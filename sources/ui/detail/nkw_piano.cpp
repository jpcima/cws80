#include "nkw_piano.h"
#include "nk_draw.h"
#include "utility/debug.h"
#include <fmt/format.h>
#include <algorithm>

namespace cws80 {

static std::array<int, 13> compute_xkey()
{
    std::array<int, 13> xkey;
    uint index = 0;
    int white, black;
    //
    xkey[index++] = white = 0;
    xkey[index++] = black = 14;
    xkey[index++] = white += 23;
    xkey[index++] = black += 28;
    xkey[index++] = white += 24;
    //
    xkey[index++] = white += 23;
    xkey[index++] = black += 41;
    xkey[index++] = white += 24;
    xkey[index++] = black += 27;
    xkey[index++] = white += 23;
    xkey[index++] = black += 27;
    xkey[index++] = white += 23;
    xkey[index++] = white += 24;
    return xkey;
}

const bool im_piano_keyblack[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};

enum { octave_width = 164 };

static im_recti im_piano_keyrect(uint key)
{
    im_recti keyrect;
    static const std::array<int, 13> xkey = compute_xkey();
    uint keymod = key % 12;
    uint keyoct = key / 12;
    bool black = im_piano_keyblack[keymod];
    keyrect.x = xkey[keymod] + octave_width * keyoct;
    keyrect.y = 0;
    int width;
    if (black) {
        width = 14;
    }
    else {
        uint k2 = keymod + 1;
        while (k2 < 12 && im_piano_keyblack[k2])
            ++k2;
        width = xkey[k2] - xkey[keymod];
    }
    keyrect.w = width;
    keyrect.h = black ? 6 : 10;
    return keyrect;
}

static im_rectf im_piano_convert_rect(const im_recti &keyrect,
                                      const im_rectf &bounds, f32 xscale, f32 yscale)
{
    im_rectf keybounds;
    keybounds.x = bounds.x + keyrect.x * xscale;
    keybounds.y = bounds.y;
    keybounds.w = keyrect.w * xscale;
    keybounds.h = keyrect.h * yscale;
    return keybounds;
}

static bool im_do_slider(nk_context *ctx, im_piano_data &data, nk_input *in,
                         nk_widget_layout_states layoutstate,
                         const im_rectf &bounds, f32 xscale, f32 yscale)
{
    nk_panel *layout = nk_window_get_panel(ctx);

    bool event = false;

    bool accept_input = !(layoutstate == NK_WIDGET_ROM || layout->flags & NK_WINDOW_ROM);

    nk_flags *state = &ctx->last_widget_state;
    im_widget_state_reset(state);

    const uint firstkey = data.firstoct * 12;
    const uint nkeys = data.octaves * 12;
    std::fill_n(data.keyevents, 128, 0);

    if (accept_input) {
        bool clicked =
            nk_input_is_mouse_click_down_in_rect(in, NK_BUTTON_LEFT, bounds, true);

        if (clicked)
            *state = NK_WIDGET_STATE_ACTIVE;

        uint keydown = ~0u;
        if (nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
            for (uint key = firstkey; keydown == ~0u && key < firstkey + nkeys; ++key) {
                if (im_piano_keyblack[key % 12]) {
                    im_rectf keybounds =
                        im_piano_convert_rect(im_piano_keyrect(key - firstkey),
                                              bounds, xscale, yscale);
                    if (nk_input_is_mouse_hovering_rect(in, keybounds))
                        keydown = key;
                }
            }
            for (uint key = firstkey; keydown == ~0u && key < firstkey + nkeys; ++key) {
                if (!im_piano_keyblack[key % 12]) {
                    im_rectf keybounds =
                        im_piano_convert_rect(im_piano_keyrect(key - firstkey),
                                              bounds, xscale, yscale);
                    if (nk_input_is_mouse_hovering_rect(in, keybounds))
                        keydown = key;
                }
            }
        }

        if (keydown != data.clickedkey) {
            if (data.clickedkey != ~0u) {
                // debug("Release {}", data.clickedkey);
                data.keyevents[data.clickedkey] = -128;
                data.keystates[data.clickedkey] = 0;
                event = true;
            }
            data.clickedkey = keydown;
            if (keydown != ~0u) {
                // debug("Press {}", keydown);
                data.keyevents[keydown] = +127;
                data.keystates[keydown] = 127;
                event = true;
            }
        }

        const bool is_hovering = nk_input_is_mouse_hovering_rect(in, bounds);
        const bool was_hovering = nk_input_is_mouse_prev_hovering_rect(in, bounds);
        if (is_hovering)
            *state = NK_WIDGET_STATE_HOVERED;
        if (*state & NK_WIDGET_STATE_HOVER && !was_hovering)
            *state |= NK_WIDGET_STATE_ENTERED;
        else if (was_hovering)
            *state |= NK_WIDGET_STATE_LEFT;
    }

    return event;
}

static void im_piano_draw(nk_context *ctx, im_piano_data &data,
                          const im_rectf &bounds, f32 xscale, f32 yscale)
{
    nk_command_buffer *out = nk_window_get_canvas(ctx);

    const uint firstkey = data.firstoct * 12;
    const uint nkeys = data.octaves * 12;

    const nk_color col_black = nk_rgba(0x00, 0x00, 0x00, 0xff);
    const nk_color col_white = nk_rgba(0xff, 0xff, 0xff, 0xff);

    for (uint key = firstkey; key < firstkey + nkeys; ++key) {
        if (!im_piano_keyblack[key % 12]) {
            im_rectf keybounds = im_piano_convert_rect(im_piano_keyrect(key - firstkey),
                                                       bounds, xscale, yscale);
            nk_stroke_rect(out, keybounds, 0, 1, col_black);
            nk_color color = col_white;
            if (data.keystates[key])
                color = nk_rgba(0x72, 0x9f, 0xcf, 0xff);
            nk_fill_rect(out, keybounds, 0, color);
        }
    }

    for (uint key = firstkey; key < firstkey + nkeys; ++key) {
        if (im_piano_keyblack[key % 12]) {
            im_rectf keybounds = im_piano_convert_rect(im_piano_keyrect(key - firstkey),
                                                       bounds, xscale, yscale);
            nk_stroke_rect(out, keybounds, 0, 1, col_black);
            nk_color color = col_black;
            if (data.keystates[key])
                color = nk_rgba(0x20, 0x4a, 0x87, 0xff);
            nk_fill_rect(out, keybounds, 0, color);
        }
    }
}

bool im_piano(nk_context *ctx, im_piano_data &data)
{
    assert(ctx);
    assert(ctx->current);
    assert(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    im_rectf bounds;
    nk_widget_layout_states state = nk_widget(&bounds, ctx);
    nk_input *in = &ctx->input;

    if (state == NK_WIDGET_INVALID)
        return 0;

    const f32 xscale = bounds.w / (data.octaves * octave_width);
    const f32 yscale = bounds.h / 10;

    bool ret = im_do_slider(ctx, data, in, state, bounds, xscale, yscale);
    im_piano_draw(ctx, data, bounds, xscale, yscale);
    return ret;
}

}  // namespace cws80
