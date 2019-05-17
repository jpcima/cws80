#pragma once
#include "ui/cws80_ui_controller.h"
#include "cws/cws80_messages.h"
#include "utility/types.h"
#include <memory>

namespace cws80 {

class UIMaster;
class NativeUI;
class GraphicsDevice;

//
class UI : public UIController {
public:
    explicit UI(UIMaster &master);
    virtual ~UI();

    static uint width();
    static uint height();
    static const char *title();

    void initialize(GraphicsDevice &gdev, NativeUI &nat);
    bool update();
    void render_display();
    void update_display();

    void receive_notification(const Notification::T &ntf);

private:
    struct Impl;
    std::unique_ptr<Impl> P;

    //----------------------------------------------------------------------------
public:
    NkScreen &screen() const override;
    NativeUI &native_ui() const override;
    const Program &program() const override;
    uint bank_number() const override;
    uint program_number() const override;
    gsl::span<const std::string> program_names() const override;
    i32 get_parameter(uint idx) const override;
    bool set_parameter(uint idx, i32 val) override;
    f32 get_f32_parameter(uint idx) const override;
    bool set_f32_parameter(uint idx, f32 val) override;
    void currently_editing_parameter(cxx::optional<uint> idx) override;
    void request_next_bank() override;
    void request_bank(uint num) override;
    void request_program(uint num) override;
    void request_rename_program(cxx::string_view name) override;
    void request_init_program() override;
    void request_write_program() override;
    void dlg_rename_program() override;
    void dlg_load_bank() override;
    void dlg_save_bank() override;
    void send_piano_events(const i8 events[128]) override;
    const std::string &led_message() override;
    void led_message(std::string msg) override;
    void led_priority_message(f64 timeout, std::string msg) override;
    const std::string &status_message() override;
    void status_message(std::string msg) override;
};

}  // namespace cws80
