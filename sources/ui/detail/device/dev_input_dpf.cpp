#include "ui/detail/device/dev_input_dpf.h"

namespace cws80 {

void InputDevice_DPF::on_keyboard(const Widget::KeyboardEvent &evt)
{
    if (evt.press)
        generic_key_down(evt.key, false, evt.mod);
    else
        generic_key_up(evt.key, false, evt.mod);
}

void InputDevice_DPF::on_special(const Widget::SpecialEvent &evt)
{
    if (evt.press)
        generic_key_down(evt.key, true, evt.mod);
    else
        generic_key_up(evt.key, true, evt.mod);
}

void InputDevice_DPF::on_mouse(const Widget::MouseEvent &evt)
{
    if (evt.press)
        generic_mouse_down((MButton)evt.button, evt.pos.getX(), evt.pos.getY(), evt.mod);
    else
        generic_mouse_up((MButton)evt.button, evt.pos.getX(), evt.pos.getY(), evt.mod);
}

void InputDevice_DPF::on_motion(const Widget::MotionEvent &evt)
{
    generic_mouse_move(evt.pos.getX(), evt.pos.getY());
}

void InputDevice_DPF::on_scroll(const Widget::ScrollEvent &evt)
{
    generic_mouse_wheel(evt.delta.getY(), evt.mod);
}

}  // namespace cws80
