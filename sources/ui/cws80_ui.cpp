#include "ui/cws80_ui.h"
#include "ui/cws80_ui_nk.h"
#include "ui/cws80_ui_view.h"
#include "ui/detail/ui_helpers_native.h"
#include "cws/cws80_program.h"
#include "cws/cws80_data.h"
#include "plugin/plug_ui_master.h"
#include "utility/path.h"
#include "utility/string.h"
#include "utility/dynarray.h"
#include "utility/debug.h"
#include <nuklear.h>
#include <fmt/format.h>
#include <gsl/gsl>
#include <algorithm>
#include <functional>
#include <utility>
#include <chrono>
#include <string>
#include <deque>
#include <thread>
#include <mutex>
#include <cstdio>

namespace cws80 {

namespace stc = std::chrono;

//------------------------------------------------------------------------------
struct Notification_Delete { void operator()(const Notification::T *x) const noexcept { NotificationTraits::free(x); } };
typedef std::unique_ptr<Notification::T, Notification_Delete> Notification_u;

//------------------------------------------------------------------------------
struct UI::Impl {
    UI *Q = nullptr;
    UIMaster *master_ = nullptr;
    std::unique_ptr<UIView> view_;
    NativeUI *native_ = nullptr;
    NkScreen screen_;
    //
    Program pgm_ = initial_program();
    uint prognum_ = 0;
    uint banknum_ = 0;
    dynarray<std::string> prognames_{4 * 128};
    std::string dir_loadbank_;
    std::string ledtext_;
    std::string ledtext_prio_;
    f64 led_timeout_ = 0;
    stc::steady_clock::time_point led_start_;
    std::string statustext_;
    std::deque<Notification_u> ntfqueue_;
    std::mutex ntfqueuesync_;
    cxx::optional<uint> edited_parameter_;
    Notification_u take_notification();
    void handle_notification(const Notification::T &ntf);
    void load_bank(const std::string &path);
    void save_bank(const std::string &path);

    //
    typedef std::function<bool(const Notification::T &ntf)> async_routine;
    struct async_process {
        async_routine routine;
        stc::milliseconds timeout;
        cxx::optional<stc::steady_clock::time_point> start;
    };
    std::deque<async_process> async_;
    void async_process_exec(const Notification::T &ntf);
    bool async_process_complete() const;
    void async_process_push(async_routine routine, stc::milliseconds timeout);
};

UI::UI(UIMaster &master)
    : P(new Impl)
{
    P->Q = this;
    P->master_ = &master;
}

UI::~UI()
{
}

uint UI::width()
{
    return 720;
}

uint UI::height()
{
    return 380;
}

const char *UI::title()
{
    return "Synthesizer";
}

//------------------------------------------------------------------------------
void UI::initialize(GraphicsDevice &gdev, NativeUI &nat)
{
    P->view_.reset(new UIView(*this, gdev));
    P->native_ = &nat;
    led_priority_message(2, "** WELCOME TO CWS-80 **");
    debug_logic("Start");
}

bool UI::update()
{
    NkScreen &screen = P->screen_;

    while (Notification_u ntf = P->take_notification()) {
        P->async_process_exec(*ntf);
        P->handle_notification(*ntf);
    }

    bool interactible = true;
    if (!P->async_process_complete())
        interactible = false;

    screen.clear();
    P->view_->draw(interactible);
    return screen.should_render();
}

void UI::render_display(void *draw_context)
{
    NkScreen &screen = P->screen_;
    screen.render(draw_context);
}

void UI::update_display()
{
    NkScreen &screen = P->screen_;
    screen.update();
}

//------------------------------------------------------------------------------
void UI::receive_notification(const Notification::T &ntf)
{
    Notification_u copy(NotificationTraits::clone(ntf));
    std::lock_guard<std::mutex> guard(P->ntfqueuesync_);
    P->ntfqueue_.push_back(std::move(copy));
}

Notification_u UI::Impl::take_notification()
{
    std::lock_guard<std::mutex> guard(ntfqueuesync_);
    auto &queue = ntfqueue_;
    Notification_u res;
    if (!queue.empty()) {
        res = std::move(queue.front());
        queue.pop_front();
    }
    return res;
}

void UI::Impl::handle_notification(const Notification::T &ntf)
{
    switch (ntf.type) {
    case NotificationType::Bank: {
        auto &bankntf = static_cast<const Notification::Bank &>(ntf);
        const Bank &bank = bankntf.data;
        uint num = bankntf.num;
        if (num >= 4)
            return;  // bad message
        char namebuf[8];
        uint i = 0, n = bank.pgm_count;
        uint j = num * 128;
        for (; i < n; ++i)
            prognames_[i + j] = bank.pgm[i].name(namebuf);
        for (; i < 128; ++i)
            prognames_[i + j] = "------";
        break;
    }
    case NotificationType::Program: {
        auto &pgmntf = static_cast<const Notification::Program &>(ntf);
        if (pgmntf.bank >= 4 || pgmntf.prog >= 128)
            return;  // bad message
        banknum_ = pgmntf.bank;
        prognum_ = pgmntf.prog;
        pgm_ = pgmntf.data;
        char namebuf[8];
        prognames_[pgmntf.prog + pgmntf.bank * 128] = pgmntf.data.name(namebuf);
        break;
    }
    case NotificationType::Write:
        Q->led_priority_message(2, "** WRITE SUCCESSFUL **");
        break;
    }
}

void UI::Impl::load_bank(const std::string &path)
{
    UIMaster &master = *master_;
    cxx::optional<Bank> bank;

    FILE *fh = fopen(path.c_str(), "rb");
    if (!fh) {
        Q->status_fmt("{}", capitalize(strerror(errno)));
        return;
    }
    SCOPE(exit) { fclose(fh); };

    try {
        bank = Bank::read_sysex(fh);
        Q->status_fmt("Loaded bank: {}", path);
        Q->led_priority_message(2, "** BANK LOADED **");
    }
    catch (std::exception &ex) {
        const char *msg = ex.what();
        Q->status_fmt("{}", capitalize(msg));
    }
    if (bank) {
        Request::LoadBank req;
        req.data = *bank;
        master.emit_request(req);
    }
    dir_loadbank_ = path_directory(path.c_str());
}

void UI::Impl::save_bank(const std::string &path)
{
    UIMaster &master = *master_;
    uint num = banknum_;

    Request::GetBankData req;
    req.bank = num;
    master.emit_request(req);

    auto async_routine = [this, path, num](const Notification::T &ntf) -> bool {
        cxx::optional<Bank> optbank;
        if (ntf.type == NotificationType::Bank) {
            const auto &bnf = static_cast<const Notification::Bank &>(ntf);
            if (bnf.num == num)
                optbank = bnf.data;
        }
        if (!optbank)
            return false;

        FILE *fh = fopen(path.c_str(), "wb");
        if (!fh) {
            Q->status_fmt("{}", capitalize(strerror(errno)));
            return false;
        }
        SCOPE(exit) { fclose(fh); };

        try {
            Bank::write_sysex(fh, *optbank);
            Q->status_fmt("Saved bank: {}", path);
            Q->led_priority_message(2, "** BANK SAVED **");
        }
        catch (std::exception &ex) {
            unlink(path.c_str());
            const char *msg = ex.what();
            Q->status_fmt("{}", capitalize(msg));
        }
        return true;
    };

    async_process_push(async_routine, stc::milliseconds(1000));
}

//------------------------------------------------------------------------------
void UI::Impl::async_process_exec(const Notification::T &ntf)
{
    if (async_.empty())
        return;
    async_process &proc = async_.front();
    stc::steady_clock::time_point start;
    stc::steady_clock::time_point now = stc::steady_clock::now();
    if (proc.start) {
        start = *proc.start;
    }
    else {
        start = now;
        proc.start = start;
    }
    bool complete = proc.routine(ntf);
    ;
    if (!complete && now - start > proc.timeout) {
        Q->led_priority_message(2, "** PROTOCOL TIMEOUT **");
        complete = true;
    }
    if (complete)
        async_.pop_front();
}

bool UI::Impl::async_process_complete() const
{
    return async_.empty();
}

void UI::Impl::async_process_push(async_routine routine, stc::milliseconds timeout)
{
    async_process proc;
    proc.routine = routine;
    proc.timeout = timeout;
    async_.push_back(proc);
}

//------------------------------------------------------------------------------
NkScreen &UI::screen() const
{
    return P->screen_;
}

NativeUI &UI::native_ui() const
{
    return *P->native_;
}

const Program &UI::program() const
{
    return P->pgm_;
}

uint UI::bank_number() const
{
    return P->banknum_;
}

uint UI::program_number() const
{
    return P->prognum_;
}

gsl::span<const std::string> UI::program_names() const
{
    return P->prognames_;
}

i32 UI::get_parameter(uint idx) const
{
    const Program &pgm = P->pgm_;
    i32 min, max;
    std::tie(min, max) = Program::get_parameter_range(idx);
    return clamp(pgm.get_parameter(idx), min, max);
}

bool UI::set_parameter(uint idx, i32 val)
{
    UIMaster &master = *P->master_;
    Program &pgm = P->pgm_;
    bool change = pgm.set_parameter(idx, val);
    i32 ival = pgm.get_parameter(idx);
    master.set_parameter_automated(idx, ival);
    return change;
}

f32 UI::get_f32_parameter(uint idx) const
{
    const Program &pgm = P->pgm_;
    i32 min, max;
    std::tie(min, max) = Program::get_parameter_range(idx);
    i32 value = clamp(pgm.get_parameter(idx), min, max);
    return (f32)(value - min) / (max - min);
}

bool UI::set_f32_parameter(uint idx, f32 val)
{
    UIMaster &master = *P->master_;
    Program &pgm = P->pgm_;
    bool change = pgm.set_f32_parameter(idx, val);
    i32 ival = pgm.get_parameter(idx);
    master.set_parameter_automated(idx, ival);
    return change;
}

void UI::currently_editing_parameter(cxx::optional<uint> idx)
{
    UIMaster &master = *P->master_;
    cxx::optional<uint> oldidx = P->edited_parameter_;
    if (idx != oldidx) {
        if (oldidx)
            master.end_edit(*oldidx);
        if (idx)
            master.begin_edit(*idx);
        P->edited_parameter_ = idx;
    }
}

void UI::request_next_bank()
{
    request_bank((P->banknum_ + 1) % 4);
}

void UI::request_bank(uint num)
{
    UIMaster &master = *P->master_;
    Request::SetBank req;
    req.bank = num;
    master.emit_request(req);
}

void UI::request_program(uint num)
{
    UIMaster &master = *P->master_;
    Request::SetProgram req;
    req.prog = num;
    master.emit_request(req);
}

void UI::request_rename_program(cxx::string_view name)
{
    UIMaster &master = *P->master_;
    Request::RenameProgram req;
    uint n = std::min<size_t>(name.size(), 6);
    for (uint i = 0; i < n; ++i)
        req.name[i] = name[i];
    for (uint i = n; i < 6; ++i)
        req.name[i] = ' ';
    master.emit_request(req);
}

void UI::request_init_program()
{
    UIMaster &master = *P->master_;
    Request::InitProgram req;
    master.emit_request(req);
}

void UI::request_write_program()
{
    UIMaster &master = *P->master_;
    Request::WriteProgram req;
    master.emit_request(req);
}

void UI::dlg_rename_program()
{
    char namebuf[8];
    cxx::optional<std::string> name =
        P->native_->edit_line("Program name", P->pgm_.name(namebuf));
    if (name)
        request_rename_program(*name);
}

void UI::dlg_load_bank()
{
    static const FileChooserFilter filters[] = {
        {"Bank files", {"*.syx", "*.mdx"}},
    };
    std::string path = P->native_->choose_file(filters, P->dir_loadbank_,
                                               "Load Bank", FileChooserMode::Open);
    if (!path.empty())
        P->load_bank(path);
}

void UI::dlg_save_bank()
{
    static const FileChooserFilter filters[] = {
        {"Bank files", {"*.syx", "*.mdx"}},
    };
    std::string path = P->native_->choose_file(filters, P->dir_loadbank_,
                                               "Save Bank", FileChooserMode::Save);
    if (!path.empty())
        P->save_bank(path);
}

void UI::send_piano_events(const i8 events[128])
{
    UIMaster &master = *P->master_;
    for (u8 key = 0; key < 127; ++key) {
        i8 event = events[key];
        if (event > 0) {
            Request::NoteOn noteon;
            noteon.key = key;
            noteon.velocity = event;
            master.emit_request(noteon);
        }
        else if (event < 0) {
            Request::NoteOff noteoff;
            noteoff.key = key;
            noteoff.velocity = -(event + 1);
            master.emit_request(noteoff);
        }
    }
}

const std::string &UI::led_message()
{
    if (P->led_timeout_ > 0) {
        uint millis = (uint)(1e3 * P->led_timeout_);
        stc::steady_clock::time_point now = stc::steady_clock::now();
        if (now - P->led_start_ < stc::milliseconds(millis))
            return P->ledtext_prio_;
        P->led_timeout_ = 0;
    }
    return P->ledtext_;
}

void UI::led_message(std::string msg)
{
    P->ledtext_ = std::move(msg);
}

void UI::led_priority_message(f64 timeout, std::string msg)
{
    P->ledtext_prio_ = std::move(msg);
    P->led_timeout_ = timeout;
    P->led_start_ = stc::steady_clock::now();
}

const std::string &UI::status_message()
{
    return P->statustext_;
}

void UI::status_message(std::string msg)
{
    P->statustext_ = std::move(msg);
}

}  // namespace cws80
