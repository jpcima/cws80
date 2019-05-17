#pragma once
#include "nkg_rect.h"
#include "utility/types.h"
#include <nuklear.h>

namespace cws80 {

void im_layout_fixed_begin(nk_context *ctx);
void im_layout_fixed_set(nk_context *ctx, im_rectf bounds);
void im_layout_fixed_end(nk_context *ctx);

}  // namespace cws80
