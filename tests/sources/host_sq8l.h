#pragma once
#include "utility/load_library.h"
#include "utility/dynarray.h"
#include "utility/types.h"
#include <vestige.h>
#include <memory>

struct Vst_ERect {
    int16_t top, left, bottom, right;
};

class Host_SQ8L {
public:
    Host_SQ8L(const char *dllname, f32 srate, u32 bsize);

    f32 sample_rate() const noexcept { return srate_; }
    u32 buffer_size() const noexcept { return bsize_; }
    u32 num_programs() const noexcept { return aeff_->numPrograms; }

    u32 get_current_program();
    void set_current_program(u32 program);
    std::string get_current_program_name();
    dynarray<u8> get_current_program_chunk();
    bool send_midi(const u8 *msg, size_t len);
    bool send_sysex(const u8 *msg, size_t len);

    void get_editor_rect(Vst_ERect **rect);
    void open_editor(void *window);
    void close_editor();
    void idle_editor();

    void process(f32 *out, u32 nframes);

    explicit operator bool() const noexcept { return aeff_ != nullptr; }

private:
    static intptr_t host_callback(
        AEffect *aeff, i32 opcode, i32 index, intptr_t value, void *ptr, f32 opt);

private:
    struct AEffect_deleter { void operator()(AEffect *x) const noexcept { x->dispatcher(x, effClose, 0, 0, nullptr, 0); } };

    f32 srate_ = 0;
    u32 bsize_ = 0;
    std::unique_ptr<f32> temp_;

    Dl_Handle_U handle_;
    std::unique_ptr<AEffect, AEffect_deleter> aeff_;

private:
    Host_SQ8L(const Host_SQ8L &) = delete;
    Host_SQ8L &operator=(const Host_SQ8L &) = delete;
};
