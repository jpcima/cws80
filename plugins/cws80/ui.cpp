#include "ui.h"
#include "plugin.h"
#include "ui/cws80_ui.h"
#include "ui/detail/ui_helpers_native.h"
#include "plugin/plug_ui_master.h"
#include "utility/debug.h"
#include "Window.hpp"
#include <memory>

SynthUI::SynthUI()
    : UI(cws80::UI::width(), cws80::UI::height()),
      ui_(*this)
{
    cws80::UI &ui = ui_;
    cws80::InputDevice_DPF *idev = new cws80::InputDevice_DPF(ui);
    idev_.reset(idev);

    SynthPlugin *fx = static_cast<SynthPlugin *>(getPluginInstancePointer());
    notifications_in_.reset(new Ring_Buffer(65536));
    requests_out_.reset(new Ring_Buffer(65536));
    fx->requests_in_ = requests_out_;
    fx->notifications_out_ = notifications_in_;
}

void SynthUI::parameterChanged(u32 index, float value)
{
#warning TODO parameterChanged
    (void)index; (void)value;
}

void SynthUI::onDisplay()
{
    if (!init_device_) {
        initDevice();
        init_device_ = true;
    }

#if defined(DGL_OPENGL)
    void *draw_context = nullptr;
#elif defined(DGL_CAIRO)
    void *draw_context = this->getParentWindow().getGraphicsContext().cairo;
#endif

    cws80::UI &ui = ui_;
    ui.render_display(draw_context);
}

void SynthUI::uiIdle()
{
    cws80::UI &ui = ui_;
    Ring_Buffer &notifications_in = *notifications_in_;
    cws80::InputDevice_DPF &idev = *idev_;

    flushRequests();

    cws80::Notification::T hdr;
    if (notifications_in.peek(hdr)) {
        cws80::NotificationTraits tr(hdr.type);
        size_t size = tr.size();
        alignas(cws80::Notification::T) u8 data[tr.max_size()];
        if (notifications_in.get(data, size))
            ui.receive_notification(*reinterpret_cast<cws80::Notification::T *>(data));
    }

    idev.flush_events();

    if (init_device_) {
        if (ui.update())
            repaint();
    }
}

bool SynthUI::onKeyboard(const KeyboardEvent &evt)
{
    cws80::InputDevice_DPF &idev = *idev_;
    idev.on_keyboard(evt);
    return true;
}

bool SynthUI::onSpecial(const SpecialEvent &evt)
{
    cws80::InputDevice_DPF &idev = *idev_;
    idev.on_special(evt);
    return true;
}

bool SynthUI::onMouse(const MouseEvent &evt)
{
    cws80::InputDevice_DPF &idev = *idev_;
    idev.on_mouse(evt);
    return true;
}

bool SynthUI::onMotion(const MotionEvent &evt)
{
    cws80::InputDevice_DPF &idev = *idev_;
    idev.on_motion(evt);
    return true;
}

bool SynthUI::onScroll(const ScrollEvent &evt)
{
    cws80::InputDevice_DPF &idev = *idev_;
    idev.on_scroll(evt);
    return true;
}

void SynthUI::initDevice()
{
    cws80::UI &ui = ui_;
    cws80::GraphicsDevice_DPF *gdev = new cws80::GraphicsDevice_DPF(ui);
    gdev_.reset(gdev);
    cws80::NativeUI *nat = cws80::NativeUI::create();
    nat_.reset(nat);
    ui.initialize(*gdev, *nat);
}

void SynthUI::flushRequests()
{
    Ring_Buffer &requests_out = *requests_out_;
    std::list<std::unique_ptr<cws80::Request::T>> &req_queue = req_queue_;

    while (!req_queue.empty()) {
        const cws80::Request::T &req = *req_queue.front();
        cws80::RequestTraits tr(req.type);
        size_t size = tr.size();
        const u8 *data = reinterpret_cast<const u8 *>(&req);
        if (!requests_out.put(data, size))
            break;
        req_queue.pop_front();
    }
}

// implement UIMaster
void SynthUI::emit_request(const cws80::Request::T &req)
{
    Ring_Buffer &requests_out = *requests_out_;
    std::list<std::unique_ptr<cws80::Request::T>> &req_queue = req_queue_;

    cws80::RequestTraits tr(req.type);
    size_t size = tr.size();
    const u8 *data = reinterpret_cast<const u8 *>(&req);

    //debug("Send request {}", size);

    flushRequests();
    if (!req_queue.empty() || !requests_out.put(data, size))
        req_queue.emplace_back(tr.clone(req));
}

void SynthUI::set_parameter_automated(uint idx, i32 val)
{
    setParameterValue(idx, val);
}

void SynthUI::begin_edit(uint idx)
{
    editParameter(idx, true);
}

void SynthUI::end_edit(uint idx)
{
    editParameter(idx, false);
}

START_NAMESPACE_DISTRHO

UI *createUI()
{
    SynthUI *ui = new SynthUI;
    debug("New UI with size {}x{}", ui->getWidth(), ui->getHeight());
    return ui;
}

END_NAMESPACE_DISTRHO
