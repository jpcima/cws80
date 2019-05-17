#include "dev_input.h"
#include "ui/cws80_ui_controller.h"
#include "ui/cws80_ui_nk.h"

namespace cws80 {

void InputDevice::flush_events()
{
    nk_context *ctx = ctl_.screen().context();
    nk_input_begin(ctx);
    while (!evtqueue_.empty()) {
        execute_event(evtqueue_.front());
        evtqueue_.pop_front();
    }
    evtqueue_ = std::move(evtdefer_);
    nk_input_end(ctx);
}

void InputDevice::add(const InputEvent &evt)
{
    evtqueue_.push_back(evt);
}

void InputDevice::add_deferred(const InputEvent &evt)
{
    evtdefer_.push_back(evt);
}

void InputDevice::execute_event(const InputEvent &evt)
{
    nk_context *ctx = ctl_.screen().context();
    switch (evt.type) {
    case InputEventType::Character:
        nk_input_unicode(ctx, evt.character);
        break;
    case InputEventType::KeyPress:
    case InputEventType::KeyRelease:
        nk_input_key(ctx, evt.key, evt.type == InputEventType::KeyPress);
        break;
    case InputEventType::ButtonPress:
    case InputEventType::ButtonRelease:
        nk_input_button(ctx, evt.button.button, evt.button.x, evt.button.y,
                        evt.type == InputEventType::ButtonPress);
        break;
    case InputEventType::Motion:
        nk_input_motion(ctx, evt.motion.x, evt.motion.y);
        break;
    case InputEventType::Scroll:
        nk_input_scroll(ctx, nk_vec2((f32)evt.scroll.dx, (f32)evt.scroll.dy));
        break;
    }
}

nk_keys InputDevice::generic_translate_key(u32 key, bool special)
{
    nk_keys nk = NK_KEY_NONE;
    if (!special) {
        switch (key) {
        case '\b':
            nk = NK_KEY_BACKSPACE;
            break;
        case '\t':
            nk = NK_KEY_TAB;
            break;
        case '\r':
            nk = NK_KEY_ENTER;
            break;
        case 127:
            nk = NK_KEY_DEL;
            break;
        default:
            break;
        }
    }
    else {
        switch ((Key)key) {
        case Key::Left:
            nk = NK_KEY_LEFT;
            break;
        case Key::Up:
            nk = NK_KEY_UP;
            break;
        case Key::Right:
            nk = NK_KEY_RIGHT;
            break;
        case Key::Down:
            nk = NK_KEY_DOWN;
            break;
        case Key::PageUp:
            nk = NK_KEY_SCROLL_UP;
            break;
        case Key::PageDown:
            nk = NK_KEY_SCROLL_DOWN;
            break;
        case Key::Home:
            nk = NK_KEY_TEXT_LINE_START;
            break;
        case Key::End:
            nk = NK_KEY_TEXT_LINE_END;
            break;
        case Key::Insert:
            nk = NK_KEY_TEXT_INSERT_MODE;
            break;
        case Key::Shift:
            nk = NK_KEY_SHIFT;
            break;
        case Key::Ctrl:
            nk = NK_KEY_CTRL;
            break;
        default:
            break;
        }
    }
    return nk;
}

void InputDevice::generic_key_down(u32 key, bool special, int mods)
{
    UIController &ctl = ctl_;
    if (!special)
        ctl.debug_input_fmt("Key pressed: {} mods {}", key, mods);
    else
        ctl.debug_input_fmt("Special key pressed: {:x} mods {}", key, mods);

    // TODO mods
    InputEvent evt;
    evt.type = InputEventType::KeyPress;
    evt.key = generic_translate_key(key, special);
    if (evt.key != NK_KEY_NONE) {
        add(evt);
    }
    else if (!special) {
        evt.type = InputEventType::Character;
        evt.character = key;
        add(evt);
    }
}

void InputDevice::generic_key_up(u32 key, bool special, int mods)
{
    UIController &ctl = ctl_;
    if (!special)
        ctl.debug_input_fmt("Key released: {} mods {}", key, mods);
    else
        ctl.debug_input_fmt("Special key released: {:x} mods {}", key, mods);

    // TODO mods
    InputEvent evt;
    evt.type = InputEventType::KeyRelease;
    evt.key = generic_translate_key(key, special);
    if (evt.key != NK_KEY_NONE)
        add_deferred(evt);
}

void InputDevice::generic_mouse_down(MButton btn, int x, int y, int mods)
{
    UIController &ctl = ctl_;
    ctl.debug_input_fmt("Button pressed: {} mods {} x {} y {}", (int)btn, mods, x, y);

    InputEvent evt;
    evt.type = InputEventType::ButtonPress;
    evt.button.x = x;
    evt.button.y = y;
    evt.button.button = (nk_buttons)((uint)btn - 1);
    add(evt);
}

void InputDevice::generic_mouse_up(MButton btn, int x, int y, int mods)
{
    UIController &ctl = ctl_;
    ctl.debug_input_fmt("Button released: {} mods {} x {} y {}", (int)btn, mods, x, y);

    InputEvent evt;
    evt.type = InputEventType::ButtonRelease;
    evt.button.x = x;
    evt.button.y = y;
    evt.button.button = (nk_buttons)((uint)btn - 1);
    add_deferred(evt);
}

void InputDevice::generic_mouse_move(int x, int y)
{
    UIController &ctl = ctl_;
    ctl.debug_input_fmt("Moved: x {} y {}", x, y);

    InputEvent evt;
    evt.type = InputEventType::Motion;
    evt.motion.x = x;
    evt.motion.y = y;
    add(evt);
}

void InputDevice::generic_mouse_wheel(f64 delta, int mods)
{
    (void)mods;

    UIController &ctl = ctl_;
    ctl.debug_input_fmt("Scroll {}", delta);

    InputEvent evt;
    evt.type = InputEventType::Scroll;
    evt.scroll.dx = 0;
    evt.scroll.dy = delta;
    add(evt);
}

void InputDevice::generic_focus_in()
{
    UIController &ctl = ctl_;
    ctl.debug_input("Focus in");
}

void InputDevice::generic_focus_out()
{
    UIController &ctl = ctl_;
    ctl.debug_input("Focus out");
}

}  // namespace cws80
