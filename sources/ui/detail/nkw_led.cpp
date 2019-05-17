#include "nkw_led.h"
#include "nk_draw.h"
#include "nkg_util.h"
#include <assert.h>

namespace cws80 {

bool im_led(nk_context *ctx, const im_skin &skin, bool value)
{
    assert(ctx);
    assert(ctx->current);
    assert(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return false;

    im_rectf bounds;
    nk_widget_layout_states layoutstate = nk_widget(&bounds, ctx);

    if (layoutstate == NK_WIDGET_INVALID)
        return false;

    const im_texture &tex = skin.textures.at(value);

    im_pointf size(tex->w, tex->h);
    im_rectf rect = centered_subrect(bounds, size);

    nk_command_buffer *out = nk_window_get_canvas(ctx);
    im_rectf old_clip = out->clip;
    nk_push_scissor(out, rect.intersect(out->clip));
    im_draw_image(out, rect, tex, nk_rgb(255, 255, 255));
    nk_push_scissor(out, old_clip);

    return true;
}

}  // namespace cws80
