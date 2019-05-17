#pragma once
#include <nuklear.h>
#include "utility/types.h"

namespace cws80 {

enum class InputEventType {
    Character,
    KeyPress,
    KeyRelease,
    ButtonPress,
    ButtonRelease,
    Motion,
    Scroll,
};

struct InputEvent {
    InputEventType type;
    union {
        u32 character;
        nk_keys key;
        struct {
            int x, y;
            nk_buttons button;
        } button;
        struct {
            int x, y;
        } motion;
        struct {
            f64 dx, dy;
        } scroll;
    };
};

enum class Kmod {
    Shift = 1,
    Control = 2,
    Alt = 4,
    Super = 8,
};

enum class Key {
    F1 = 0xE000,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    Left,
    Up,
    Right,
    Down,
    PageUp,
    PageDown,
    Home,
    End,
    Insert,
    Shift,
    Ctrl,
    Alt,
    Super
};

enum class MButton {
    Left = 1,
    Middle = 2,
    Right = 3,
};

}  // namespace cws80
