#include "nk_layout.h"

namespace cws80 {

void im_layout_fixed_begin(nk_context *ctx)
{
    nk_layout_space_begin(ctx, NK_STATIC, 0, std::numeric_limits<int>::max());
}

void im_layout_fixed_set(nk_context *ctx, im_rectf bounds)
{
    nk_layout_space_push(ctx, bounds);
}

void im_layout_fixed_end(nk_context *ctx)
{
    nk_layout_space_end(ctx);
}

}  // namespace cws80
