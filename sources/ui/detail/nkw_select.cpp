#include "nkw_select.h"
#include "nk_draw.h"
#include "nk_layout.h"
#include "ui/detail/ui_helpers_native.h"
#include "utility/debug.h"
#include <fmt/format.h>

namespace cws80 {

static void im_do_select(nk_context *ctx, NativeUI &nat,
                         gsl::span<const std::string> choices, uint *value, nk_input *in,
                         nk_widget_layout_states layoutstate, const im_rectf &bounds)
{
    nk_panel *layout = nk_window_get_panel(ctx);

    bool accept_input = !(layoutstate == NK_WIDGET_ROM || layout->flags & NK_WINDOW_ROM);

    nk_flags *state = &ctx->last_widget_state;
    im_widget_state_reset(state);

    if (accept_input) {
        if (nk_input_is_mouse_click_in_rect(in, NK_BUTTON_LEFT, bounds)) {
            uint optsel = nat.select_by_menu(choices, *value);
            if (optsel != ~0u)
                *value = optsel;
        }
    }
}

static void im_select_draw(nk_context *ctx, gsl::span<const std::string> choices,
                           uint value, im_rectf bounds)
{
    nk_command_buffer *out = nk_window_get_canvas(ctx);

    const nk_user_font *font = ctx->style.font;
    assert(font);

    const std::string &text = choices.at(value);
    f32 texth = font->height;
    f32 textw = im_text_width(font, text);
    im_rectf textbounds = bounds.from_center(textw, texth);

    // nk_fill_rect(out, bounds, 0, nk_rgb(0x20, 0x20, 0x20));
    nk_fill_rect(out, bounds.from_vcenter(20), 0, nk_rgb(0x20, 0x20, 0x20));

    nk_draw_text(out, textbounds, text.c_str(), text.size(), font,
                 nk_rgba_u32(0), nk_rgba(0xc0, 0xc0, 0xc0, 0xff));
}

bool im_select(nk_context *ctx, NativeUI &nat,
               gsl::span<const std::string> choices, uint *value)
{
    assert(ctx);
    assert(ctx->current);
    assert(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return false;

    im_rectf screenbounds = ctx->current->layout->row.item;

    im_rectf bounds;
    nk_widget_layout_states layoutstate = nk_widget(&bounds, ctx);
    nk_input *in = &ctx->input;

    if (layoutstate == NK_WIDGET_INVALID)
        return false;

    size_t nchoices = choices.size();
    uint oldval = *value;

    f32 btnw = 20;
    f32 btnh = 20;
    bounds.take_from_left(btnw + 4);
    bounds.take_from_right(btnw + 4);

    im_do_select(ctx, nat, choices, value, in, layoutstate, bounds);

    im_rectf lbtnbox = screenbounds.take_from_left(btnw);
    im_rectf rbtnbox = screenbounds.take_from_right(btnw);
    lbtnbox = lbtnbox.from_vcenter(btnh);
    rbtnbox = rbtnbox.from_vcenter(btnh);

    im_layout_fixed_set(ctx, lbtnbox);
    if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_LEFT))
        if (*value > 0)
            --*value;

    im_layout_fixed_set(ctx, rbtnbox);
    if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_RIGHT))
        if (*value + 1 < nchoices)
            ++*value;

    im_select_draw(ctx, choices, *value, bounds);

    return *value != oldval;
}

}  // namespace cws80
