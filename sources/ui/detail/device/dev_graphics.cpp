#include "dev_graphics.h"

namespace cws80 {

im_texture GraphicsDevice::load_texture(const im_image &img)
{
    const u8 *data = img.data();
    uint w = img.width();
    uint h = img.height();
    uint channels = img.channels();
    return load_texture(data, w, h, channels);
}

}  // namespace cws80
