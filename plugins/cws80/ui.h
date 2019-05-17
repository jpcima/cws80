#pragma once
#include "DistrhoUI.hpp"
#include "ui/cws80_ui.h"
#include "ui/detail/device/dev_graphics_gl.h"
#include "ui/detail/device/dev_input_dpf.h"
#include "ui/detail/ui_helpers_native.h"
#include "plugin/plug_ui_master.h"
#include "ring_buffer.h"
#include <list>
#include <memory>

class SynthUI : public UI, public cws80::UIMaster {
public:
    SynthUI();

    std::shared_ptr<Ring_Buffer> notifications_in_;
    std::shared_ptr<Ring_Buffer> requests_out_;

protected:
    void parameterChanged(u32 index, float value) override;
    void onDisplay() override;
    void uiIdle() override;
    bool onKeyboard(const KeyboardEvent &evt) override;
    bool onSpecial(const SpecialEvent &evt) override;
    bool onMouse(const MouseEvent &evt) override;
    bool onMotion(const MotionEvent &evt) override;
    bool onScroll(const ScrollEvent &evt) override;

private:
    void initDevice();
    void flushRequests();

    // -------------------------------------------------------------------------------------------------------
protected:
    // implement UIMaster
    void emit_request(const cws80::Request::T &req) override;
    void set_parameter_automated(uint idx, i32 val) override;
    void begin_edit(uint idx) override;
    void end_edit(uint idx) override;

private:
    bool init_device_ = false;
    std::unique_ptr<cws80::GraphicsDevice_GL> gdev_;
    std::unique_ptr<cws80::InputDevice_DPF> idev_;
    std::unique_ptr<cws80::NativeUI> nat_;
    cws80::UI ui_;
    std::list<std::unique_ptr<cws80::Request::T>> req_queue_;

private:
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthUI)
};
