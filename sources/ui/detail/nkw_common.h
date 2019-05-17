#pragma once
#include "nki_image.h"
#include "nk_essential.h"
#include "utility/types.h"
#include <fmt/format.h>

namespace cws80 {

class GraphicsDevice;

//
void im_widget_state_reset(nk_flags *s);

struct im_slider_data {
    f32 min = 0, max = 1;
    uint steps = 20;
    bool clicked = false;
};

struct im_skin {
    explicit operator bool() const { return !textures.empty(); }
    std::vector<im_texture> textures;
};

enum class skin_orientation { Horizontal, Vertical };

im_skin im_load_skin(GraphicsDevice &gdev, const im_image &img,
                     skin_orientation ori, uint steps);

template <class... A>
void im_label(struct nk_context *ctx, nk_flags flags, const char *fmt, const A &... as)
{
    nk_label(ctx, fmt::format(fmt, as...).c_str(), flags);
}

}  // namespace cws80
