#include "cws80_ui_view.h"
#include "cws80_ui_data.h"
#include "cws80_ui_nk.h"
#include "cws80_ui.h"
#include "ui/detail/nkw_button.h"
#include "ui/detail/nkw_slider.h"
#include "ui/detail/nkw_select.h"
#include "ui/detail/nkw_led.h"
#include "ui/detail/nkw_placeholder.h"
#include "ui/detail/nkg_util.h"
#include "ui/detail/nk_layout.h"
#include "ui/detail/nk_draw.h"
#include "ui/detail/nk_essential.h"
#include "ui/detail/nkw_piano.h"
#include "ui/detail/ui_helpers_native.h"
#include "ui/detail/device/dev_graphics.h"
#include "cws/component/env.h"
#include "utility/arithmetic.h"
#include "utility/debug.h"
#include "utility/c++std/optional.h"
#include "utility/c++std/string_view.h"
#include <fmt/ostream.h>
#include <algorithm>
#include <string>
#include <array>
#include <tuple>
#include <vector>

namespace cws80 {

enum class EditorPage {
    Osc1,
    Osc2,
    Osc3,
    Lfo1,
    Lfo2,
    Lfo3,
    Env1,
    Env2,
    Env3,
    Env4,
    Dca1,
    Dca2,
    Dca3,
    Dca4,
    Filter,
    Misc,
    Count,
};

static const char *editor_pagename[] = {
    "OSC1", "OSC2", "OSC3", "LFO1", "LFO2", "LFO3", "ENV1", "ENV2",
    "ENV3", "ENV4", "DCA1", "DCA2", "DCA3", "DCA4", "FILT", "MISC",
};

//------------------------------------------------------------------------------
struct UIView::Impl {
    UIController *ctl_ = nullptr;
    GraphicsDevice *gdev_ = nullptr;
    //
    im_skin sk_slider_;
    im_skin sk_knob_;
    im_texture tex_button_;
    im_texture tex_tiny_button_;
    im_skin sk_led_;
    im_texture tex_logo_;
    im_texture tex_diagram_;
    im_texture tex_background_;
    im_texture tex_background2_;
    im_piano_data piano_data_;
    //
    EditorPage page_current_ = EditorPage::Osc1;
    std::vector<im_slider_data> parameter_slider_data_{Param::num_params};
    //
    cxx::optional<uint> edited_parameter_;  // parameter number
    //
    void initialize_resources();
    static gsl::span<const FontRequest> font_requests();
    //
    void create_main_button_row(im_rectf bounds);
    void create_page(EditorPage page, im_rectf bounds);
    void create_page_osc(uint nth, im_rectf bounds);
    void create_page_dca(uint nth, im_rectf bounds);
    void create_page_filt(im_rectf bounds);
    void create_page_env(uint nth, im_rectf bounds);
    void create_page_lfo(uint nth, im_rectf bounds);
    void create_page_dca4(im_rectf bounds);
    void create_page_misc(im_rectf bounds);
    bool slider_for_parameter(uint idx, const im_skin &skin, const char *label = nullptr);
    bool selector_for_parameter(uint idx, gsl::span<const std::string> items,
                                const char *label = nullptr);
    bool selector_for_modulator(uint idx, const char *label = nullptr);
    bool selector_for_lfowave(uint idx, const char *label = nullptr);
    bool button_for_parameter(uint idx, const im_texture &skin,
                              const im_skin &ledskin = {}, const char *label = nullptr);
    bool labeled_button(im_rectf bounds, const im_texture &skin, const char *label);
    //
    void draw_diagram(im_rectf box);
    void draw_envelope(im_rectf box, const Program::Env &env, f32 Td, f32 Tsus,
                       const nk_color &col);
    void draw_envelopes(const im_rectf box[4], const Program::Env env[4],
                        const nk_color col[4]);
    void draw_wave(im_rectf box, const Program::Osc &osc, const nk_color &col);
    void draw_waves(const im_rectf box[3], const Program::Osc osc[3],
                    const nk_color col[3]);
    //
    std::string led_status_for_page(EditorPage page) const;
};

UIView::UIView(UIController &ctl, GraphicsDevice &gdev)
    : P(new Impl)
{
    P->ctl_ = &ctl;
    P->gdev_ = &gdev;

    uint w = UI::width();
    uint h = UI::height();

    NkScreen &screen = ctl.screen();
    screen.init(gdev, w, h, P->font_requests());
    P->initialize_resources();
}

UIView::~UIView()
{
}

void UIView::draw(bool interactible)
{
    UIController &ctl = *P->ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();
    //nk_user_font *font = screen.font(0);
    nk_user_font *font_led = screen.font(1);
    nk_user_font *font_emph = screen.font(2);
    uint w = UI::width();
    uint h = UI::height();
    EditorPage page = P->page_current_;
    P->edited_parameter_ = {};

    //
    int windowflags = 0;
    windowflags |= NK_WINDOW_NO_SCROLLBAR;
    windowflags |= interactible ? NK_WINDOW_REMOVE_ROM : NK_WINDOW_NOT_INTERACTIVE;

    im_rectf wbox(0, 0, (f32)w, (f32)h);

    // debug("NkBegin");
    if (nk_begin(ctx, "Window", wbox, windowflags)) {
        ims_push_scissor(ctx, wbox);

        im_layout_fixed_begin(ctx);
        im_rectf cbox = wbox;
        im_rectf statusbox = cbox.take_from_bottom(20);

        im_texture &tex_background = P->tex_background_;
        im_texture &tex_background2 = P->tex_background2_;

        ims_draw_tiled_image(ctx, cbox, tex_background, nk_rgb(255, 255, 255));

        im_texture &tex_logo = P->tex_logo_;
        ims_draw_image(ctx,
                       cbox.resized(im_pointf(tex_logo->w, tex_logo->h)).off_by({6, 4}),
                       tex_logo, nk_rgb(255, 255, 255));

        P->create_main_button_row(cbox.take_from_top(50).chop_from_top(6).chop_from_right(4));

        {
            ctl.led_message(P->led_status_for_page(page));

            im_rectf toprow = cbox.take_from_top(tex_logo->h).chop_from_top(15);

            // im_rectf ledbox = toprow.from_right(550).from_top(55).off_by({-10, 6});
            im_rectf ledbox = toprow.from_top(55).reduced({80, 0});
            ims_fill_rect(ctx, ledbox, 5.0, nk_rgb(0x20, 0x20, 0x20));
            ims_stroke_rect(ctx, ledbox, 5.0, 1.0, nk_rgb(0x80, 0x80, 0x80));
            nk_style_push_font(ctx, font_led);
            ims_draw_led_screen(ctx, ledbox.reduced(6), ctl.led_message(), {1, -1});
            nk_style_pop_font(ctx);

            cbox.chop_from_top(20);
        }

        {
            im_texture &tex_diagram = P->tex_diagram_;
            im_rectf row = cbox.take_from_top(tex_diagram->h);
            row.chop_from_right(40);
            im_rectf modrect = row.take_from_right(170);
            im_rectf diagrect = row.from_hcenter(tex_diagram->w);
            P->draw_diagram(diagrect);

            {
                // ims_stroke_rect(ctx, modrect, 0, 1, nk_rgba(0xff, 0xff, 0xff, 0xff));

                modrect.chop_from_left(4);
                im_rectf btnrow1 = modrect.from_top(row.h / 2);
                im_rectf btnrow2 = modrect.from_bottom(row.h / 2);

                EditorPage pages1[] = {EditorPage::Env1, EditorPage::Env2,
                                       EditorPage::Env3, EditorPage::Env4};
                EditorPage pages2[] = {EditorPage::Lfo1, EditorPage::Lfo2,
                                       EditorPage::Lfo3, EditorPage::Misc};

                f32 btnw = 40;
                f32 btnspc = 4;
                for (EditorPage page : pages1) {
                    if (P->labeled_button(btnrow1.take_from_left(btnw), P->tex_tiny_button_,
                                          editor_pagename[(uint)page]))
                        P->page_current_ = page;
                    btnrow1.chop_from_left(btnspc);
                }
                for (EditorPage page : pages2) {
                    if (P->labeled_button(btnrow2.take_from_left(btnw), P->tex_tiny_button_,
                                          editor_pagename[(uint)page]))
                        P->page_current_ = page;
                    btnrow2.chop_from_left(btnspc);
                }
            }

            cbox.chop_from_top(10);
        }

        {
            im_rectf mainbox = cbox.take_from_top(130).reduced({12, 0});
            const char *title = editor_pagename[(uint)page];
            nk_style_push_font(ctx, font_emph);
            ims_draw_tiled_image(ctx, mainbox, tex_background2, nk_rgb(255, 255, 255));
            ims_draw_title_box_alt(ctx, mainbox, title);
            nk_style_pop_font(ctx);
            P->create_page(page, mainbox.reduced(8));
            cbox.chop_from_top(10);
        }

        if (false) {
            im_rectf row = cbox.take_from_top(70).chop_from_top(10).reduced({60, 0});
            im_layout_fixed_set(ctx, row);
            im_piano_data &piano_data = P->piano_data_;
            if (im_piano(ctx, piano_data))
                ctl.send_piano_events(piano_data.keyevents);
        }

        const std::string &statustext = ctl.status_message();
        if (!statustext.empty()) {
            ims_fill_rect(ctx, statusbox, 5.0, nk_rgb(0x20, 0x20, 0x20));
            im_layout_fixed_set(ctx, statusbox);
            nk_label_colored(ctx, statustext.c_str(), NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE,
                             nk_rgb(0xff, 0xbd, 0x24));
        }

        im_layout_fixed_end(ctx);
        nk_end(ctx);
    }

    ctl.currently_editing_parameter(P->edited_parameter_);
}

void UIView::Impl::initialize_resources()
{
    GraphicsDevice &gdev = *gdev_;
    im_image img;

    img.load_from_memory(ui::sk_slider, 4);
    sk_slider_ = im_load_skin(gdev, img, skin_orientation::Horizontal, 64);

    img.load_from_memory(ui::sk_knob, 4);
    sk_knob_ = im_load_skin(gdev, img, skin_orientation::Vertical, 64);

    img.load_from_memory(ui::sk_button, 4);
    tex_button_ = gdev.load_texture(img);
    img.load_from_memory(ui::sk_tiny_button, 4);
    tex_tiny_button_ = gdev.load_texture(img);

    sk_led_.textures.resize(2);
    img.load_from_memory(ui::sk_led_off, 4);
    sk_led_.textures[0] = gdev.load_texture(img);
    img.load_from_memory(ui::sk_led_on, 4);
    sk_led_.textures[1] = gdev.load_texture(img);

    img.load_from_memory(ui::im_logo, 4);
    tex_logo_ = gdev.load_texture(img);
    img.load_from_memory(ui::im_diagram, 4);
    tex_diagram_ = gdev.load_texture(img);
    img.load_from_memory(ui::im_background, 4);
    tex_background_ = gdev.load_texture(img);
    img.load_from_memory(ui::im_background2, 4);
    tex_background2_ = gdev.load_texture(img);
}

gsl::span<const FontRequest> UIView::Impl::font_requests()
{
    static const FontRequest fontreqs[] = {
        FontRequest::Memory(13, ui::fnt_proggy_clean, sizeof(ui::fnt_proggy_clean)),
        FontRequest::Memory(20, ui::fnt_seg14_data, sizeof(ui::fnt_seg14_data)),
        FontRequest::Memory(21, ui::fnt_scp_black_italic_data,
                            sizeof(ui::fnt_scp_black_italic_data)),
    };
    return fontreqs;
}

void UIView::Impl::create_main_button_row(im_rectf bounds)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();
    NativeUI &native = ctl.native_ui();
    const Program &pgm = ctl.program();
    uint prognum = ctl.program_number();
    uint banknum = ctl.bank_number();
    char bankletter = 'A' + banknum;
    gsl::span<const std::string> prognames = ctl.program_names();

    const f32 btnw = 50;
    const f32 btnspc = 6;
    im_rectf btnbox;

    btnbox = bounds.take_from_right(btnw + btnspc).from_left(btnw);
    if (labeled_button(btnbox, tex_button_, "BANK"))
        ctl.request_next_bank();

    std::string program_choices[128];
    for (uint i = 0; i < 128; ++i) {
        char namebuf[8];
        cxx::string_view name =
            (i == prognum) ? pgm.name(namebuf) : prognames[i + banknum * 128];
        program_choices[i] =
            fmt::format(fmt::format("{}{:03d}  {:6s}", bankletter, i + 1, name));
    }

    bounds.chop_from_right(10);
    im_rectf progbox = bounds.take_from_right(150).from_vcenter(24);
    im_layout_fixed_set(ctx, progbox);
    if (im_select(ctx, native, program_choices, &prognum))
        ctl.request_program(prognum);

    bounds.chop_from_right(10);

    btnbox = bounds.take_from_right(btnw + btnspc).from_left(btnw);
    if (labeled_button(btnbox, tex_button_, "WRITE"))
        ctl.request_write_program();

    btnbox = bounds.take_from_right(btnw + btnspc).from_left(btnw);
    if (labeled_button(btnbox, tex_button_, "INIT"))
        ctl.request_init_program();

    btnbox = bounds.take_from_right(btnw + btnspc).from_left(btnw);
    if (labeled_button(btnbox, tex_button_, "NAME"))
        ctl.dlg_rename_program();

    btnbox = bounds.take_from_right(btnw + btnspc).from_left(btnw);
    if (labeled_button(btnbox, tex_button_, "SAVE"))
        ctl.dlg_save_bank();

    btnbox = bounds.take_from_right(btnw + btnspc).from_left(btnw);
    if (labeled_button(btnbox, tex_button_, "LOAD"))
        ctl.dlg_load_bank();
}

void UIView::Impl::create_page(EditorPage page, im_rectf bounds)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();

    switch (page) {
    case EditorPage::Osc1:
    case EditorPage::Osc2:
    case EditorPage::Osc3: {
        uint nth = (uint)page - (uint)EditorPage::Osc1;
        create_page_osc(nth, bounds);
        break;
    }

    case EditorPage::Dca1:
    case EditorPage::Dca2:
    case EditorPage::Dca3: {
        uint nth = (uint)page - (uint)EditorPage::Dca1;
        create_page_dca(nth, bounds);
        break;
    }

    case EditorPage::Filter:
        create_page_filt(bounds);
        break;

    case EditorPage::Env1:
    case EditorPage::Env2:
    case EditorPage::Env3:
    case EditorPage::Env4: {
        uint nth = (uint)page - (uint)EditorPage::Env1;
        create_page_env(nth, bounds);
        break;
    }

    case EditorPage::Lfo1:
    case EditorPage::Lfo2:
    case EditorPage::Lfo3: {
        uint nth = (uint)page - (uint)EditorPage::Lfo1;
        create_page_lfo(nth, bounds);
        break;
    }

    case EditorPage::Dca4:
        create_page_dca4(bounds);
        break;

    case EditorPage::Misc:
        create_page_misc(bounds);
        break;

    default:
        im_layout_fixed_set(ctx, bounds);
        im_placeholder(ctx);
    }
}

void UIView::Impl::create_page_osc(uint nth, im_rectf bounds)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();
    const Program &pgm = ctl.program();

    const uint pdelta = nth * (P_Osc1_SEMI - P_Osc0_SEMI);

    {
        im_rectf side = bounds.take_from_right(120);
        std::vector<im_rectf> drawbox = vsubdiv(side, 3, 4);
        std::array<nk_color, 3> color;
        color.fill(nk_rgb(0xa0, 0xa0, 0xa0));
        color[nth] = nk_rgb(0x00, 0xf0, 0x00);
        for (uint i = 0; i < 3; ++i) {
            ims_stroke_rect(ctx, drawbox[i], 0, 1, nk_rgb(0xff, 0xff, 0xff));
            ims_fill_rect(ctx, drawbox[i], 0, nk_rgb(0x30, 0x30, 0x30));
            drawbox[i] = drawbox[i].reduced({0, 2});
        }
        draw_waves(drawbox.data(), pgm.oscs, color.data());
    }

    // TODO
    {
        im_rectf row1 = bounds.from_top(bounds.h / 2);
        im_rectf row2 = bounds.from_bottom(bounds.h / 2);
        row1.chop_from_left(80);

        im_layout_fixed_set(ctx, row1.take_from_left(150).from_top(40).reduced({8, 0}));
        std::string wavenames[256];
        for (uint i = 0; i < 256; ++i) {
            char namebuf[16];
            if (!*waveset_name(i, namebuf))
                sprintf(namebuf, "HIDDEN %u", i);
            wavenames[i] = namebuf;
        }
        this->selector_for_parameter(P_Osc0_WAVEFORM + pdelta, wavenames, "WAVE");

        im_layout_fixed_set(ctx, row1.take_from_left(150).from_top(40).reduced({8, 0}));
        this->selector_for_modulator(P_Osc0_FMSRC1 + pdelta, "MOD 1");
        im_layout_fixed_set(ctx, row1.take_from_left(150).from_top(40).reduced({8, 0}));
        this->selector_for_modulator(P_Osc0_FMSRC2 + pdelta, "MOD 2");

        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Osc0_OCT + pdelta, sk_knob_, "OCT");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Osc0_SEMI + pdelta, sk_knob_, "SEMI");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Osc0_FINE + pdelta, sk_knob_, "FINE");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Osc0_FCMODAMT1 + pdelta, sk_knob_, "DEPTH 1");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Osc0_FCMODAMT2 + pdelta, sk_knob_, "DEPTH 2");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->button_for_parameter(P_Misc_SYNC, tex_tiny_button_, sk_led_, "SYNC 1>2");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->button_for_parameter(P_Misc_AM, tex_tiny_button_, sk_led_, "AM 1>2");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->button_for_parameter(P_Misc_OSC, tex_tiny_button_, sk_led_, "OSC");
    }
}

void UIView::Impl::create_page_dca(uint nth, im_rectf bounds)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();
    const Program &pgm = ctl.program();

    const uint pdelta = nth * (P_Osc1_SEMI - P_Osc0_SEMI);

    {
        im_rectf side = bounds.take_from_right(120);
        std::vector<im_rectf> drawbox = vsubdiv(side, 3, 4);
        std::array<nk_color, 3> color;
        color.fill(nk_rgb(0xa0, 0xa0, 0xa0));
        color[nth] = nk_rgb(0x00, 0xf0, 0x00);
        for (uint i = 0; i < 3; ++i) {
            ims_stroke_rect(ctx, drawbox[i], 0, 1, nk_rgb(0xff, 0xff, 0xff));
            ims_fill_rect(ctx, drawbox[i], 0, nk_rgb(0x30, 0x30, 0x30));
            drawbox[i] = drawbox[i].reduced({0, 2});
        }
        draw_waves(drawbox.data(), pgm.oscs, color.data());
    }

    {
        im_rectf row1 = bounds.from_top(bounds.h / 2);
        im_rectf row2 = bounds.from_bottom(bounds.h / 2);
        row1.chop_from_left(80);

        im_layout_fixed_set(ctx, row1.take_from_left(150).from_top(40).reduced({8, 0}));
        this->selector_for_modulator(P_Osc0_AMSRC1 + pdelta, "MOD 1");
        im_layout_fixed_set(ctx, row1.take_from_left(150).from_top(40).reduced({8, 0}));
        this->selector_for_modulator(P_Osc0_AMSRC2 + pdelta, "MOD 2");

        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->button_for_parameter(P_Osc0_DCAENABLE + pdelta, tex_tiny_button_,
                                   sk_led_, "ENABLE");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Osc0_DCALEVEL + pdelta, sk_knob_, "LEVEL");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Osc0_AMAMT1 + pdelta, sk_knob_, "DEPTH 1");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Osc0_AMAMT2 + pdelta, sk_knob_, "DEPTH 2");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->button_for_parameter(P_Misc_AM, tex_tiny_button_, sk_led_, "AM 1>2");
    }
}

void UIView::Impl::create_page_filt(im_rectf bounds)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();

    {
        im_rectf row1 = bounds.from_top(bounds.h / 2);
        im_rectf row2 = bounds.from_bottom(bounds.h / 2);
        row1.chop_from_left(80);

        im_layout_fixed_set(ctx, row1.take_from_left(150).from_top(40).reduced({8, 0}));
        this->selector_for_modulator(P_Misc_FCSRC1, "MOD 1");
        im_layout_fixed_set(ctx, row1.take_from_left(150).from_top(40).reduced({8, 0}));
        this->selector_for_modulator(P_Misc_FCSRC2, "MOD 2");

        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Misc_FLTFC, sk_knob_, "FREQ");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Misc_Q, sk_knob_, "Q");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Misc_KEYBD, sk_knob_, "KEYBD");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Misc_FCMODAMT1, sk_knob_, "DEPTH 1");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Misc_FCMODAMT2, sk_knob_, "DEPTH 2");
    }
}

void UIView::Impl::create_page_env(uint nth, im_rectf bounds)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();
    const Program &pgm = ctl.program();

    const uint pdelta = nth * (P_Env1_L1 - P_Env0_L1);

    im_rectf side = bounds.take_from_right(250);
    // std::vector<im_rectf> drawbox = vsubdiv(side, 4, 4);

    std::vector<im_rectf> drawbox;
    drawbox.reserve(4);
    for (im_rectf row : vsubdiv(side, 2, 12))
        for (im_rectf col : hsubdiv(row, 2, 6))
            drawbox.push_back(col);

    std::array<nk_color, 4> color;
    color.fill(nk_rgb(0xa0, 0xa0, 0xa0));
    color[nth] = nk_rgb(0x00, 0xf0, 0x00);

    for (uint i = 0; i < 4; ++i) {
        ims_stroke_rect(ctx, drawbox[i], 0, 1, nk_rgb(0xff, 0xff, 0xff));
        ims_fill_rect(ctx, drawbox[i], 0, nk_rgb(0x30, 0x30, 0x30));
        drawbox[i] = drawbox[i].reduced({0, 2});
    }
    draw_envelopes(drawbox.data(), pgm.envs, color.data());

    {
        im_rectf row1 = bounds.from_top(bounds.h / 2);
        im_rectf row2 = bounds.from_bottom(bounds.h / 2);
        row1.chop_from_left(80);

        f32 elw = 60;

        im_layout_fixed_set(ctx, row1.take_from_left(elw).from_vcenter(60));
        this->slider_for_parameter(P_Env0_L1 + pdelta, sk_knob_, "L1");
        im_layout_fixed_set(ctx, row1.take_from_left(elw).from_vcenter(60));
        this->slider_for_parameter(P_Env0_L2 + pdelta, sk_knob_, "L2");
        im_layout_fixed_set(ctx, row1.take_from_left(elw).from_vcenter(60));
        this->slider_for_parameter(P_Env0_L3 + pdelta, sk_knob_, "L3");
        im_layout_fixed_set(ctx, row1.take_from_left(elw).from_vcenter(60));
        this->slider_for_parameter(P_Env0_LV + pdelta, sk_knob_, "LV");
        im_layout_fixed_set(ctx, row1.take_from_left(elw).from_vcenter(60));
        this->slider_for_parameter(P_Env0_T1V + pdelta, sk_knob_, "T1V");

        im_layout_fixed_set(ctx, row2.take_from_left(elw).from_vcenter(60));
        this->slider_for_parameter(P_Env0_T1 + pdelta, sk_knob_, "T1");
        im_layout_fixed_set(ctx, row2.take_from_left(elw).from_vcenter(60));
        this->slider_for_parameter(P_Env0_T2 + pdelta, sk_knob_, "T2");
        im_layout_fixed_set(ctx, row2.take_from_left(elw).from_vcenter(60));
        this->slider_for_parameter(P_Env0_T3 + pdelta, sk_knob_, "T3");
        im_layout_fixed_set(ctx, row2.take_from_left(elw).from_vcenter(60));
        this->slider_for_parameter(P_Env0_T4 + pdelta, sk_knob_, "T4");
        im_layout_fixed_set(ctx, row2.take_from_left(elw).from_vcenter(60));
        this->slider_for_parameter(P_Env0_TK + pdelta, sk_knob_, "TK");
        im_layout_fixed_set(ctx, row2.take_from_left(elw).from_vcenter(60));
        this->button_for_parameter(P_Misc_ENV, tex_tiny_button_, sk_led_, "ENV");
        im_layout_fixed_set(ctx, row2.take_from_left(elw).from_vcenter(60));
        this->button_for_parameter(P_Misc_CYCLE, tex_tiny_button_, sk_led_, "CYCLE");
    }
}

void UIView::Impl::create_page_lfo(uint nth, im_rectf bounds)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();

    const uint pdelta = nth * (P_Lfo1_L1 - P_Lfo0_L1);

    {
        im_rectf row1 = bounds.from_top(bounds.h / 2);
        im_rectf row2 = bounds.from_bottom(bounds.h / 2);
        row1.chop_from_left(80);

        im_layout_fixed_set(ctx, row1.take_from_left(150).from_top(40).reduced({8, 0}));
        this->selector_for_modulator(P_Lfo0_MOD + pdelta, "MOD");
        im_layout_fixed_set(ctx, row1.take_from_left(150).from_top(40).reduced({8, 0}));
        this->selector_for_lfowave(P_Lfo0_WAV + pdelta, "WAVE");

        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Lfo0_FREQ + pdelta, sk_knob_, "FREQ");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Lfo0_L1 + pdelta, sk_knob_, "L1");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Lfo0_L2 + pdelta, sk_knob_, "L2");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Lfo0_DELAY + pdelta, sk_knob_, "DELAY");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->button_for_parameter(P_Lfo0_RESET + pdelta, tex_tiny_button_, sk_led_, "RESET");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->button_for_parameter(P_Lfo0_HUMAN + pdelta, tex_tiny_button_, sk_led_, "HUMAN");
    }
}

void UIView::Impl::create_page_dca4(im_rectf bounds)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();
    const Program &pgm = ctl.program();

    {
        im_rectf side = bounds.take_from_right(120);
        std::vector<im_rectf> drawbox = vsubdiv(side, 3, 4);
        std::array<nk_color, 3> color;
        color.fill(nk_rgb(0x00, 0xf0, 0x00));
        for (uint i = 0; i < 3; ++i) {
            ims_stroke_rect(ctx, drawbox[i], 0, 1, nk_rgb(0xff, 0xff, 0xff));
            ims_fill_rect(ctx, drawbox[i], 0, nk_rgb(0x30, 0x30, 0x30));
            drawbox[i] = drawbox[i].reduced({0, 2});
        }
        draw_waves(drawbox.data(), pgm.oscs, color.data());
    }

    {
        im_rectf row1 = bounds.from_top(bounds.h / 2);
        im_rectf row2 = bounds.from_bottom(bounds.h / 2);
        row1.chop_from_left(80);

        im_layout_fixed_set(ctx, row1.take_from_left(150).from_top(40).reduced({8, 0}));
        this->selector_for_modulator(P_Misc_PANMODSRC, "PAN MOD");

        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Misc_DCA4MODAMT, sk_knob_, "ENV4 DEPTH");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Misc_PAN, sk_knob_, "PAN");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Misc_PANMODAMT, sk_knob_, "PAN DEPTH");
    }
}

void UIView::Impl::create_page_misc(im_rectf bounds)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();

    {
        im_rectf row1 = bounds.from_top(bounds.h / 2);
        im_rectf row2 = bounds.from_bottom(bounds.h / 2);
        row1.chop_from_left(80);

        im_layout_fixed_set(ctx, row1.take_from_left(70).from_vcenter(60));
        this->button_for_parameter(P_Misc_SYNC, tex_tiny_button_, sk_led_, "SYNC 1>2");
        im_layout_fixed_set(ctx, row1.take_from_left(70).from_vcenter(60));
        this->button_for_parameter(P_Misc_AM, tex_tiny_button_, sk_led_, "AM 1>2");
        im_layout_fixed_set(ctx, row1.take_from_left(70).from_vcenter(60));
        this->button_for_parameter(P_Misc_MONO, tex_tiny_button_, sk_led_, "MONO");

        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->slider_for_parameter(P_Misc_GLIDE, sk_knob_, "GLIDE");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->button_for_parameter(P_Misc_VC, tex_tiny_button_, sk_led_, "VC");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->button_for_parameter(P_Misc_ENV, tex_tiny_button_, sk_led_, "ENV");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->button_for_parameter(P_Misc_OSC, tex_tiny_button_, sk_led_, "OSC");
        im_layout_fixed_set(ctx, row2.take_from_left(70).from_vcenter(60));
        this->button_for_parameter(P_Misc_CYCLE, tex_tiny_button_, sk_led_, "CYCLE");
    }
}

bool UIView::Impl::slider_for_parameter(uint idx, const im_skin &skin, const char *label)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();
    assert(ctx->current);
    assert(ctx->current->layout);
    nk_panel *layout = ctx->current->layout;

    if (label) {
        const nk_user_font *font = ctx->style.font;
        im_rectf rect = layout->row.item;
        im_rectf bottom = rect.take_from_bottom(font->height);
        f32 textw = im_text_width(font, label);
        im_rectf textbounds = bottom.from_hcenter(textw);
        ims_fill_rect(ctx, textbounds.expanded({4, 0}), 0, nk_rgba(0x30, 0x30, 0x30, 0xff));
        im_layout_fixed_set(ctx, bottom);
        nk_label(ctx, label, NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);
        im_layout_fixed_set(ctx, rect);
    }

    bool change = false;
    f32 val = ctl.get_f32_parameter(idx);
    if (im_slider(ctx, skin, &val, parameter_slider_data_.at(idx))) {
        edited_parameter_ = idx;
        ctl.currently_editing_parameter(idx);
        change = ctl.set_f32_parameter(idx, val);
    }
    return change;
}

bool UIView::Impl::selector_for_parameter(uint idx, gsl::span<const std::string> items,
                                          const char *label)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    NativeUI &nat = ctl.native_ui();
    nk_context *ctx = screen.context();
    assert(ctx->current);
    assert(ctx->current->layout);
    nk_panel *layout = ctx->current->layout;

    if (label) {
        const nk_user_font *font = ctx->style.font;
        im_rectf rect = layout->row.item;
        im_rectf bottom = rect.take_from_bottom(font->height);
        f32 textw = im_text_width(font, label);
        im_rectf textbounds = bottom.from_hcenter(textw);
        ims_fill_rect(ctx, textbounds.expanded({4, 0}), 0, nk_rgba(0x30, 0x30, 0x30, 0xff));
        im_layout_fixed_set(ctx, bottom);
        nk_label(ctx, label, NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);
        im_layout_fixed_set(ctx, rect);
    }

    bool change = false;
    uint val = ctl.get_parameter(idx);
    if (im_select(ctx, nat, items, &val)) {
        edited_parameter_ = idx;
        ctl.currently_editing_parameter(idx);
        change = ctl.set_parameter(idx, val);
    }
    return change;
}

bool UIView::Impl::selector_for_modulator(uint idx, const char *label)
{
    static const std::string modnames[16] = {"LFO1",  "LFO2",  "LFO3",  "ENV1",
                                             "ENV2",  "ENV3",  "ENV4",  "VEL",
                                             "VEL2",  "KYBD",  "KYBD2", "WHEEL",
                                             "PEDAL", "XCTRL", "PRESS", "OFF"};
    return selector_for_parameter(idx, modnames, label);
}

bool UIView::Impl::selector_for_lfowave(uint idx, const char *label)
{
    static const std::string lfowavenames[4] = {
        "TRI",
        "SAW",
        "SQR",
        "NOI",
    };
    return selector_for_parameter(idx, lfowavenames, label);
}

bool UIView::Impl::button_for_parameter(uint idx, const im_texture &skin,
                                        const im_skin &ledskin, const char *label)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();
    assert(ctx->current);
    assert(ctx->current->layout);
    nk_panel *layout = ctx->current->layout;

    im_rectf ledrect;
    if (ledskin) {
        im_rectf rect = layout->row.item;
        ledrect = rect.take_from_top(20);
        im_layout_fixed_set(ctx, rect);
    }

    if (label) {
        const nk_user_font *font = ctx->style.font;
        im_rectf rect = layout->row.item;
        im_rectf bottom = rect.take_from_bottom(font->height);
        f32 textw = im_text_width(font, label);
        im_rectf textbounds = bottom.from_hcenter(textw);
        ims_fill_rect(ctx, textbounds.expanded({4, 0}), 0, nk_rgba(0x30, 0x30, 0x30, 0xff));
        im_layout_fixed_set(ctx, bottom);
        nk_label(ctx, label, NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);
        im_layout_fixed_set(ctx, rect);
    }

    bool change = false;
    i32 val = ctl.get_parameter(idx);
    i32 min, max;
    std::tie(min, max) = Program::get_parameter_range(idx);

    if (im_button(ctx, skin)) {
        edited_parameter_ = idx;
        ctl.currently_editing_parameter(idx);
        change = ctl.set_parameter(idx, (val == min) ? max : min);
        val = ctl.get_parameter(idx);
    }

    if (ledskin) {
        im_layout_fixed_set(ctx, ledrect);
        im_led(ctx, ledskin, val != min);
    }
    return change;
}

bool UIView::Impl::labeled_button(im_rectf bounds, const im_texture &skin, const char *label)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();
    const nk_user_font *font = ctx->style.font;
    assert(font);
    f32 texth = font->height;
    f32 btnw = skin->w;
    f32 btnh = skin->h;
    f32 yoff = (bounds.h - btnh - texth) / 2;
    im_layout_fixed_set(ctx, im_rectf(bounds.x + (bounds.w - btnw) / 2,
                                      bounds.y + yoff, btnw, btnh));
    bool value = im_button(ctx, skin);
    im_layout_fixed_set(ctx, im_rectf(bounds.x, bounds.y + yoff + btnh, bounds.w, texth));
    nk_label(ctx, label, NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_TOP);
    return value;
}

void UIView::Impl::draw_diagram(im_rectf box)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();
    im_texture &tex_diagram = tex_diagram_;

    ims_draw_image(ctx, box, tex_diagram, nk_rgb(255, 255, 255));

    im_rectf btnbox{0, 0, 40, 30};
    f32 hoffset = 16;
    f32 yoffset = 4;
    f32 hspacing = 110;
    f32 vspacing = 30;

    btnbox.y = box.y + yoffset;
    btnbox.x = box.x + hoffset;
    for (unsigned i = 0; i < 3; ++i) {
        EditorPage page = (EditorPage)((uint)EditorPage::Osc1 + i);
        if (labeled_button(btnbox, tex_tiny_button_, editor_pagename[(uint)page]))
            page_current_ = page;
        btnbox.y += vspacing;
    }

    btnbox.y = box.y + yoffset;
    btnbox.x = box.x + hoffset + hspacing;
    for (unsigned i = 0; i < 3; ++i) {
        EditorPage page = (EditorPage)((uint)EditorPage::Dca1 + i);
        if (labeled_button(btnbox, tex_tiny_button_, editor_pagename[(uint)page]))
            page_current_ = page;
        btnbox.y += vspacing;
    }

    btnbox.y = box.y + yoffset + vspacing;
    btnbox.x = box.x + hoffset + hspacing * 2;
    {
        EditorPage page = EditorPage::Filter;
        if (labeled_button(btnbox, tex_tiny_button_, editor_pagename[(uint)page]))
            page_current_ = page;
    }

    btnbox.y = box.y + yoffset + vspacing;
    btnbox.x = box.x + hoffset + hspacing * 3;
    {
        EditorPage page = EditorPage::Dca4;
        if (labeled_button(btnbox, tex_tiny_button_, editor_pagename[(uint)page]))
            page_current_ = page;
    }
}

void UIView::Impl::draw_envelope(im_rectf box, const Program::Env &env, f32 Td,
                                 f32 Tsus, const nk_color &col)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();
    nk_command_buffer *out = nk_window_get_canvas(ctx);
    im_rectf old_clip = out->clip;

    ims_push_scissor(ctx, box);
    // ims_fill_rect(ctx, box, 0, nk_rgb(0, 0, 0));

    f32 x = box.x, y = box.y;
    f32 w = box.w, h = box.h;

    f32 L1 = env.L1 / 64.0f;
    f32 L2 = env.L2 / 64.0f;
    f32 L3 = env.L3 / 64.0f;
    f32 T1 = Env::timeval(env.T1);
    f32 T2 = Env::timeval(env.T2);
    f32 T3 = Env::timeval(env.T3);
    f32 T4 = Env::timeval(env.T4);

    // time values
    std::array<im_pointf, 6> V{{
        {0, 0},
        {T1, L1},
        {T1 + T2, L2},
        {T1 + T2 + T3, L3},
        {T1 + T2 + T3 + Tsus, L3},
        {T1 + T2 + T3 + Tsus + T4, 0},
    }};

    // points
    std::array<im_pointf, 6> P;
    for (uint i = 0; i < P.size(); ++i) {
        P[i].x = (Td == 0) ? x : (x + (V[i].x / Td) * (w - 1));
        P[i].y = y + (1 - ((V[i].y + 1) / 2)) * (h - 1);
    }

    // ims_fill_polygon(ctx, P, nk_rgb(0, 0, 255));

    for (im_pointf p : P) {
        im_rectf r = im_rectf(p.x, p.y, 0, 0).expanded(3);
        ims_stroke_rect(ctx, r, 0, 1, nk_rgb(0xfc, 0xe9, 0x4f));
    }

    for (uint i = 0; i < P.size() - 1; ++i)
        ims_stroke_line(ctx, P[i].x, P[i].y, P[i + 1].x, P[i + 1].y, 1, col);
    ims_stroke_line(ctx, P[5].x, P[5].y, x + w - 1, P[0].y, 1, col);

    nk_push_scissor(out, old_clip);
}

void UIView::Impl::draw_envelopes(const im_rectf box[4],
                                  const Program::Env env[4], const nk_color col[4])
{
    f32 Td = 0;
    for (uint i = 0; i < 4; ++i) {
        f32 T1 = Env::timeval(env[i].T1);
        f32 T2 = Env::timeval(env[i].T2);
        f32 T3 = Env::timeval(env[i].T3);
        f32 T4 = Env::timeval(env[i].T4);
        Td = std::max(Td, T1 + T2 + T3 + T4);
    }
    f32 Tsus = Td / 3;
    Td += Tsus;
    for (uint i = 0; i < 4; ++i)
        draw_envelope(box[i], env[i], Td, Tsus, col[i]);
}

void UIView::Impl::draw_wave(im_rectf box, const Program::Osc &osc, const nk_color &col)
{
    UIController &ctl = *ctl_;
    NkScreen &screen = ctl.screen();
    nk_context *ctx = screen.context();
    nk_command_buffer *out = nk_window_get_canvas(ctx);
    im_rectf old_clip = out->clip;

    ims_push_scissor(ctx, box);
    // ims_fill_rect(ctx, box, 0, nk_rgb(0, 0, 0));

    f32 x = box.x, y = box.y;
    f32 w = box.w, h = box.h;

    Waveset waveset = waveset_by_id(osc.WAVEFORM);
    Wave wave = wave_by_id(waveset.wavenum[0]);
    Sample sample = wave_sample(wave);

    im_pointf p1;
    for (uint i = 0; i < w; ++i) {
        f32 r = i / (w - 1);
        f32 val = sample.data[uint(r * sample.length())] / 255.0f;
        im_pointf p2(x + i, y + (1 - val) * (h - 1));
        if (i > 0)
            ims_stroke_line(ctx, p1.x, p1.y, p2.x, p2.y, 1, col);
        p1 = p2;
    }

    nk_push_scissor(out, old_clip);
}

void UIView::Impl::draw_waves(const im_rectf box[3], const Program::Osc osc[3],
                              const nk_color col[3])
{
    for (uint i = 0; i < 3; ++i)
        draw_wave(box[i], osc[i], col[i]);
}

std::string UIView::Impl::led_status_for_page(EditorPage page) const
{
    UIController &ctl = *ctl_;
    const Program &pgm = ctl.program();
    const char *pagename = editor_pagename[(uint)page];
    std::string status;

    switch (page) {
    case EditorPage::Osc1:
    case EditorPage::Osc2:
    case EditorPage::Osc3: {
        uint nth = (uint)page - (uint)EditorPage::Osc1;
        const Program::Osc &pm = pgm.oscs[nth];

        int oct = pm.OCT();
        int semi = pm.SEMI();
        int fine = pm.FINE;
        char wave[16];
        waveset_name(pm.WAVEFORM, wave);
        if (!wave[0])
            sprintf(wave, "%u", pm.WAVEFORM);
        const char *mod1 = modulator_name((Mod)pm.FMSRC1);
        const char *mod2 = modulator_name((Mod)pm.FMSRC2);

        status = fmt::format(
            "OSC{}  OCT={:+02d}  SEMI={:+03d}  FINE={:+03d}  WAVE={}\n"
            "      MODS={:5s}  * {:+03d}      {:5s}  * {:+03d}",
            nth + 1, oct, semi, fine, wave, mod1, pm.FCMODAMT1, mod2, pm.FCMODAMT2);

        break;
    }

    case EditorPage::Env1:
    case EditorPage::Env2:
    case EditorPage::Env3:
    case EditorPage::Env4: {
        uint nth = (uint)page - (uint)EditorPage::Env1;
        const Program::Env &pm = pgm.envs[nth];

        status =
            fmt::format("ENV{}  L1={:+03d}  L2={:+03d}  L3={:+03d}  LV={:02d}\n"
                        "      T1={:02d}  T2={:02d}  T3={:02d}  T4={:02d}  "
                        "T1V={:02d}  TK={:02d}",
                        nth + 1, pm.L1, pm.L2, pm.L3, pm.LV, pm.T1, pm.T2,
                        pm.T3, pm.T4, pm.T1V, pm.TK);

        break;
    }

    case EditorPage::Lfo1:
    case EditorPage::Lfo2:
    case EditorPage::Lfo3: {
        uint nth = (uint)page - (uint)EditorPage::Lfo1;
        const Program::Lfo &pm = pgm.lfos[nth];
        const char *wave = lfo_wave_name((LfoWave)pm.WAV);
        const char *mod = modulator_name((Mod)pm.MOD());

        status =
            fmt::format("LFO{}  FREQ={:02d}  RESET={:3s}  HUMAN={:3s}  "
                        "WAV={:3s}\n"
                        "      L1={:02d}  DELAY={:02d}  L2={:02d}  MOD={:5s}",
                        nth + 1, pm.FREQ, pm.RESET ? "ON" : "OFF",
                        pm.HUMAN ? "ON" : "OFF", wave, pm.L1, pm.DELAY, pm.L2, mod);

        break;
    }

    case EditorPage::Dca1:
    case EditorPage::Dca2:
    case EditorPage::Dca3: {
        uint nth = (uint)page - (uint)EditorPage::Dca1;
        const Program::Osc &pm = pgm.oscs[nth];
        const char *mod1 = modulator_name((Mod)pm.AMSRC1);
        const char *mod2 = modulator_name((Mod)pm.AMSRC2);

        status =
            fmt::format("DCA{}  LEVEL={:2d}  OUTPUT={:3s}\n"
                        "      MODS={:5s}  * {:+03d}      {:5s}  * {:+03d}",
                        nth + 1, pm.DCALEVEL, pm.DCAENABLE ? "ON" : "OFF", mod1,
                        pm.AMAMT1, mod2, pm.AMAMT2);

        break;
    }

    case EditorPage::Dca4: {
        const Program::Misc &pm = pgm.misc;
        const char *pmod = modulator_name((Mod)pm.PANMODSRC);

        status = fmt::format("DCA4  ENV4  * {:02d}  PAN={:02d}\n"
                             "      MODS={:5s}  * {:+03d}",
                             pm.DCA4MODAMT, pm.PAN, pmod, pm.PANMODAMT);

        break;
    }

    case EditorPage::Filter: {
        const Program::Misc &pm = pgm.misc;
        const char *mod1 = modulator_name((Mod)pm.FCSRC1);
        const char *mod2 = modulator_name((Mod)pm.FCSRC2);

        status =
            fmt::format("FILT  FREQ={:03d}  Q={:02d}  KEYBD={:02d}\n"
                        "      MODS={:5s}  * {:+03d}      {:5s}  * {:+03d}",
                        pm.FLTFC, pm.Q, pm.KEYBD, mod1, pm.FCMODAMT1, mod2, pm.FCMODAMT2);

        break;
    }

    case EditorPage::Misc: {
        const Program::Misc &pm = pgm.misc;

        status = fmt::format(
            "MISC  SYNC={:01d}   AM={:01d}   MONO={:01d}   GLIDE={:02d}\n"
            "        VC={:01d}  ENV={:01d}    OSC={:01d}   CYCLE={:01d}",
            pm.SYNC, pm.AM, pm.MONO, pm.GLIDE, pm.VC, pm.ENV, pm.OSC, pm.CYCLE);

        break;
    }

    default:
        status = fmt::format("TODO: status for {}", pagename);
    }

    return status;
}

}  // namespace cws80
