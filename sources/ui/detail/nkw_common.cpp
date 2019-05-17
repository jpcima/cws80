#include "nkw_common.h"
#include "ui/detail/device/dev_graphics.h"
#include "utility/dynarray.h"
#include <memory>

namespace cws80 {

void im_widget_state_reset(nk_flags *s)
{
    *s = NK_WIDGET_STATE_INACTIVE | (*s & NK_WIDGET_STATE_MODIFIED);
}

im_skin im_load_skin(GraphicsDevice &gdev, const im_image &img, skin_orientation ori, uint steps)
{
    im_skin skin;
    if (steps < 1)
        return skin;

    dynarray<im_image> imgs(steps);

    uint w = img.width();
    uint h = img.height();
    bool horiz = ori == skin_orientation::Horizontal;

    if (steps <= 0 || (horiz && w % steps != 0) || (!horiz && h % steps != 0))
        throw std::logic_error("cannot slice picture with this many divisions");

    w = horiz ? (w / steps) : w;
    h = horiz ? h : (h / steps);

    for (uint i = 0; i < steps; ++i) {
        im_recti rect((horiz ? (w * i) : 0), (horiz ? 0 : (h * i)), w, h);
        imgs[i] = img.cut(rect);
    }

    im_recti rect = imgs[0].crop_alpha_border();
    for (uint i = 1; i < steps; ++i)
        rect = rect.unite(imgs[i].crop_alpha_border());

    uint rw = rect.w;
    uint rh = rect.h;
    if (rw != w || rh != h)
        for (uint i = 0; i < steps; ++i)
            imgs[i] = imgs[i].cut(rect);

    skin.textures.resize(steps);
    for (uint i = 0; i < steps; ++i)
        skin.textures[i] = gdev.load_texture(imgs[i]);
    return skin;
}

}  // namespace cws80
