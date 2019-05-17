#pragma once
#include "ui/detail/device/dev_input.h"
#include "utility/types.h"
#include "Widget.hpp"

namespace cws80 {

class InputDevice_DPF : public InputDevice {
public:
    using InputDevice::InputDevice;

    void on_keyboard(const Widget::KeyboardEvent &evt);
    void on_special(const Widget::SpecialEvent &evt);
    void on_mouse(const Widget::MouseEvent &evt);
    void on_motion(const Widget::MotionEvent &evt);
    void on_scroll(const Widget::ScrollEvent &evt);

private:
    u32 keychars_[256] = {};
};

}  // namespace cws80
