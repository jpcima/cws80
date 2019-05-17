#include "nkw_slider.h"
#include "nk_draw.h"
#include "utility/arithmetic.h"
#include <fmt/format.h>
#include <assert.h>

namespace cws80 {

struct im_slider_dims {
    im_rectf rect, image, input;
};

im_slider_dims im_slider_get_dims(im_rectf bounds, const im_skin &skin)
{
    im_slider_dims dims;
    const im_texture &tex = skin.textures.at(0);
    uint texw = tex->w, texh = tex->h;
    dims.rect = bounds;
    dims.image = bounds.from_center((f32)texw, (f32)texh);
    dims.input = bounds.intersect(dims.image);
    return dims;
}

static f32 im_slider_value_from_pos(f32 y, const im_slider_data &data,
                                    const im_slider_dims &dims)
{
    f32 ratio = (y - dims.input.y) / dims.input.h;
    f32 value = data.max - (data.min + ratio * (data.max - data.min));
    return clamp(value, data.min, data.max);
}

static bool im_do_slider(nk_context *ctx, f32 *value, im_slider_data &data,
                         nk_input *in, nk_widget_layout_states layoutstate,
                         const im_slider_dims &dims)
{
    nk_panel *layout = nk_window_get_panel(ctx);

    const f32 orig_value = *value;
    f32 new_value = orig_value;
    //bool was_clicked = data.clicked;
    data.clicked = false;

    bool accept_input = !(layoutstate == NK_WIDGET_ROM || layout->flags & NK_WINDOW_ROM);

    nk_flags *state = &ctx->last_widget_state;
    im_widget_state_reset(state);

    if (accept_input) {
        if (nk_input_is_mouse_down(in, NK_BUTTON_LEFT) &&
            nk_input_has_mouse_click_down_in_rect(in, NK_BUTTON_LEFT, dims.input, true)) {
            *state = NK_WIDGET_STATE_ACTIVE;
            data.clicked = true;
            new_value = im_slider_value_from_pos(in->mouse.pos.y, data, dims);
        }
        else if (in->mouse.scroll_delta.y) {
            new_value += in->mouse.scroll_delta.y / data.steps;
            new_value = clamp(new_value, data.min, data.max);
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
    else {
#pragma message("TODO what did I mean by this")
        // if (was_clicked && nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
        //   data.clicked = true;
        //   new_value = im_slider_value_from_pos(in->mouse.pos.y, data, dims);
        // }
    }

    *value = new_value;
    return new_value != orig_value;
}

static void im_slider_draw(nk_context *ctx, const im_skin &skin, f32 value,
                           const im_slider_data &data, const im_slider_dims &dims)
{
    nk_command_buffer *out = nk_window_get_canvas(ctx);

    f32 r = clamp((value - data.min) / (data.max - data.min), 0.0f, 1.0f);
    size_t index = (size_t)(r * (skin.textures.size() - 1));

    const im_texture &tex = skin.textures[index];
    im_rectf old_clip = out->clip;
    nk_push_scissor(out, dims.rect.intersect(out->clip));
    nk_color drawcolor = nk_rgba(255, 255, 255, 255);
    im_draw_image(out, dims.image, tex, drawcolor);
    nk_push_scissor(out, old_clip);
}

bool im_slider(nk_context *ctx, const im_skin &skin, f32 *value, im_slider_data &data)
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

    im_slider_dims dims = im_slider_get_dims(bounds, skin);
    bool ret = im_do_slider(ctx, value, data, in, layoutstate, dims);
    im_slider_draw(ctx, skin, *value, data, dims);

    return ret;
}

}  // namespace cws80
