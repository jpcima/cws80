#pragma once
#include "dev_input_common.h"
#include "utility/types.h"
#include <deque>

namespace cws80 {

class UIController;

//
class InputDevice {
public:
    explicit InputDevice(UIController &ctl)
        : ctl_(ctl)
    {
    }
    virtual ~InputDevice() {}

public:
    void flush_events();

private:
    void add(const InputEvent &evt);
    void add_deferred(const InputEvent &evt);
    void execute_event(const InputEvent &evt);
    UIController &ctl_;
    std::deque<InputEvent> evtqueue_;
    std::deque<InputEvent> evtdefer_;

protected:
    static nk_keys generic_translate_key(u32 key, bool special);
    void generic_key_down(u32 key, bool special, int mods);
    void generic_key_up(u32 key, bool special, int mods);
    void generic_mouse_down(MButton btn, int x, int y, int mods);
    void generic_mouse_up(MButton btn, int x, int y, int mods);
    void generic_mouse_move(int x, int y);
    void generic_mouse_wheel(f64 delta, int mods);
    void generic_focus_in();
    void generic_focus_out();
};

}  // namespace cws80
