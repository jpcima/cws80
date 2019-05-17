#pragma once
#include "utility/types.h"
#include "utility/c++std/optional.h"
#include "utility/c++std/string_view.h"
#include <fmt/format.h>
#include <gsl/gsl>

namespace cws80 {

class NkScreen;
class NativeUI;
struct Bank;
struct Program;

///
class UIController {
public:
    virtual ~UIController() {}

    virtual NkScreen &screen() const = 0;
    virtual NativeUI &native_ui() const = 0;
    virtual const Program &program() const = 0;
    virtual uint bank_number() const = 0;
    virtual uint program_number() const = 0;
    virtual gsl::span<const std::string> program_names() const = 0;
    virtual i32 get_parameter(uint idx) const = 0;
    virtual bool set_parameter(uint idx, i32 val) = 0;
    virtual f32 get_f32_parameter(uint idx) const = 0;
    virtual bool set_f32_parameter(uint idx, f32 val) = 0;
    virtual void currently_editing_parameter(cxx::optional<uint> idx) = 0;
    virtual void request_next_bank() = 0;
    virtual void request_bank(uint num) = 0;
    virtual void request_program(uint num) = 0;
    virtual void request_rename_program(cxx::string_view name) = 0;
    virtual void request_init_program() = 0;
    virtual void request_write_program() = 0;
    virtual void dlg_rename_program() = 0;
    virtual void dlg_load_bank() = 0;
    virtual void dlg_save_bank() = 0;
    virtual void send_piano_events(const i8 events[128]) = 0;
    virtual const std::string &led_message() = 0;
    virtual void led_message(std::string msg) = 0;
    virtual void led_priority_message(f64 timeout, std::string msg) = 0;
    virtual const std::string &status_message() = 0;
    virtual void status_message(std::string msg) = 0;
    //
    template <class A, class... As>
    void led_fmt(const char *fmt, const A &x, const As &... xs)
    {
        led_message(fmt::format(fmt, x, xs...));
    }
    template <class A, class... As>
    void led_priority_fmt(f64 timeout, const char *fmt, const A &x, const As &... xs)
    {
        led_priority_message(timeout, fmt::format(fmt, x, xs...));
    }
    //
    template <class A, class... As>
    void status_fmt(const char *fmt, const A &x, const As &... xs)
    {
        status_message(fmt::format(fmt, x, xs...));
    }
    //
    enum {
        debugging_input = true,
        debugging_logic = true,
    };
    inline void debug_input(std::string str)
    {
        if (debugging_input)
            status_message(std::move(str));
    }
    inline void debug_logic(std::string str)
    {
        if (debugging_logic)
            status_message(std::move(str));
    }
    template <class A, class... As>
    void debug_input_fmt(const char *fmt, const A &x, const As &... xs)
    {
        if (debugging_input)
            status_fmt(fmt, x, xs...);
    }
    template <class A, class... As>
    void debug_logic_fmt(const char *fmt, const A &x, const As &... xs)
    {
        if (debugging_logic)
            status_fmt(fmt, x, xs...);
    }
};

}  // namespace cws80
