#include "nkw_placeholder.h"
#include "nkw_common.h"
#include <assert.h>

namespace cws80 {

int im_placeholder(nk_context *ctx)
{
    assert(ctx);
    assert(ctx->current);
    assert(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    im_rectf bounds;
    nk_widget_layout_states state = nk_widget(&bounds, ctx);
    if (state == NK_WIDGET_INVALID)
        return 0;

    // draw
    nk_command_buffer *out = nk_window_get_canvas(ctx);
    // im_rectf old_clip = out->clip;
    // nk_push_scissor(out, bounds.intersect(out->clip));
    nk_color linecolor = nk_rgb(0xa0, 0, 0);
    nk_stroke_rect(out, bounds, 0.f, 1.f, linecolor);
    nk_stroke_line(out, bounds.x, bounds.y, bounds.right(), bounds.bottom(), 1.f, linecolor);
    nk_stroke_line(out, bounds.x, bounds.bottom(), bounds.right(), bounds.top(),
                   1.f, linecolor);
    // nk_push_scissor(out, old_clip);

    return 1;
}

}  // namespace cws80
