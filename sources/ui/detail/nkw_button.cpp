#include "nkw_button.h"
#include "nk_draw.h"
#include "utility/arithmetic.h"
#include <fmt/format.h>
#include <assert.h>

namespace cws80 {

struct im_button_dims {
    im_rectf rect, image, input;
};

im_button_dims im_button_get_dims(im_rectf bounds, const im_texture &tex)
{
    im_button_dims dims;
    uint texw = tex->w, texh = tex->h;
    dims.rect = bounds;
    dims.image = bounds.from_center((f32)texw, (f32)texh);
    dims.input = bounds.intersect(dims.image);
    return dims;
}

static bool im_do_button(nk_context *ctx, nk_input *in,
                         nk_widget_layout_states layoutstate, const im_button_dims &dims)
{
    nk_panel *layout = nk_window_get_panel(ctx);

    bool clicked = false;

    bool accept_input = !(layoutstate == NK_WIDGET_ROM || layout->flags & NK_WINDOW_ROM);

    nk_flags *state = &ctx->last_widget_state;
    im_widget_state_reset(state);

    if (accept_input) {
        if (nk_input_is_mouse_click_down_in_rect(in, NK_BUTTON_LEFT, dims.input, true)) {
            *state = NK_WIDGET_STATE_ACTIVE;
            clicked = true;
        }

        const bool is_hovering = nk_input_is_mouse_hovering_rect(in, dims.input);
        const bool was_hovering = nk_input_is_mouse_prev_hovering_rect(in, dims.input);
        if (is_hovering)
            *state = NK_WIDGET_STATE_HOVERED;
        if (*state & NK_WIDGET_STATE_HOVER && !was_hovering)
            *state |= NK_WIDGET_STATE_ENTERED;
        else if (was_hovering)
            *state |= NK_WIDGET_STATE_LEFT;
    }

    return clicked;
}

static void im_button_draw(nk_context *ctx, const im_texture &tex, const im_button_dims &dims)
{
    nk_command_buffer *out = nk_window_get_canvas(ctx);

    im_rectf old_clip = out->clip;
    nk_push_scissor(out, dims.rect.intersect(out->clip));
    nk_color drawcolor = nk_rgba(255, 255, 255, 255);
    im_draw_image(out, dims.image, tex, drawcolor);
    nk_push_scissor(out, old_clip);
}

bool im_button(nk_context *ctx, const im_texture &skin)
{
    assert(ctx);
    assert(ctx->current);
    assert(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return false;

    im_rectf bounds;
    nk_widget_layout_states layoutstate = nk_widget(&bounds, ctx);
    nk_input *in = &ctx->input;

    if (layoutstate == NK_WIDGET_INVALID)
        return false;

    im_button_dims dims = im_button_get_dims(bounds, skin);
    bool ret = im_do_button(ctx, in, layoutstate, dims);
    im_button_draw(ctx, skin, dims);

    return ret;
}

}  // namespace cws80
