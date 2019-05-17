#pragma once
#include "nkw_common.h"
#include "utility/types.h"

namespace cws80 {

struct im_piano_data {
    uint firstoct = 2;
    uint octaves = 5;
    uint clickedkey = ~0u;
    i8 keyevents[128]{};
    u8 keystates[128]{};
};

bool im_piano(nk_context *ctx, im_piano_data &data);

}  // namespace cws80
