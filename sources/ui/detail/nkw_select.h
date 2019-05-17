#pragma once
#include "nkw_common.h"
#include "utility/types.h"
#include <gsl/gsl>

namespace cws80 {

class NativeUI;

//
bool im_select(nk_context *ctx, NativeUI &nat,
               gsl::span<const std::string> choices, uint *value);

}  // namespace cws80
