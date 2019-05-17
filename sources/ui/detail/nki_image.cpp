#include "nki_image.h"
#include <string.h>

namespace cws80 {

im_image::im_image()
{
}

im_image::~im_image()
{
}

void im_image::load(const char *filename, uint desired_channels)
{
    int w{}, h{}, n{};
    u8 *data = stbi_load(filename, &w, &h, &n, desired_channels);
    if (!data)
        throw exception("cannot load image from path");
    w_ = w;
    h_ = h;
    n_ = n;
    c_ = desired_channels;
    data_.reset(data);
}

void im_image::load_from_memory(const u8 *buffer, uint length, uint desired_channels)
{
    int w{}, h{}, n{};
    u8 *data = stbi_load_from_memory(buffer, length, &w, &h, &n, desired_channels);
    if (!data)
        throw exception("cannot load image from memory");
    w_ = w;
    h_ = h;
    n_ = n;
    c_ = desired_channels;
    data_.reset(data);
}

im_image im_image::cut(const im_recti &r) const
{
    const uint imw = w_, imh = h_;
    const uint rx = r.x, ry = r.y, rw = r.w, rh = r.h;
    const uint c = c_;

    if (r.x < 0 || r.w < 0 || r.y < 0 || r.h < 0 || rw > imw || rh > imh ||
        rx > imw - rw || ry > imh - rh)
        throw exception("cannot cut an image region with these dimensions");

    std::unique_ptr<u8[], void (*)(void *)> data{nullptr, &stbi_image_free};
    data.reset(reinterpret_cast<u8 *>(STBI_MALLOC(c * rw * rh)));

    if (!data)
        throw std::bad_alloc();

    for (uint i = 0; i < rh; ++i) {
        const u8 *src = &data_[c * ((i + ry) * imw + rx)];
        u8 *dst = &data[c * (i * rw)];
        memcpy(dst, src, c * rw);
    }

    im_image res;
    res.data_ = std::move(data);
    res.w_ = rw;
    res.h_ = rh;
    res.n_ = n_;
    res.c_ = c;
    return res;
}

const u8 *im_image::pixel(uint x, uint y) const
{
    return &data_[c_ * (x + y * w_)];
}

u8 im_image::alpha(uint x, uint y) const
{
    return (c_ != 4) ? 0xff : pixel(x, y)[3];
}

bool im_image::is_transparent_column(uint x) const
{
    for (uint y = 0, h = h_; y < h; ++y)
        if (alpha(x, y))
            return false;
    return true;
}

bool im_image::is_transparent_row(uint y) const
{
    for (uint x = 0, w = w_; x < w; ++x)
        if (alpha(x, y))
            return false;
    return true;
}

std::vector<im_image> im_image::vsplit() const
{
    std::vector<im_image> res;

    const uint imw = w_, imh = h_;
    const uint h = imw;

    const uint n = imh / h;
    res.resize(n);

    for (uint i = 0; i < n; ++i) {
        im_recti bounds(0, i * h, imw, h);
        res[i] = this->cut(bounds);
    }

    return res;
}

im_recti im_image::rect() const
{
    return im_recti(0, 0, w_, h_);
}

im_recti im_image::crop_alpha_border() const
{
    const uint w = w_, h = h_;

    if (c_ != 4)
        return im_recti(0, 0, w, h);

    uint rx = 0, ry = 0, rw = w, rh = h;
    for (uint i = 0; i < w && is_transparent_column(i); ++i) {
        ++rx;
        --rw;
    }
    for (uint i = 0; i < h && is_transparent_row(i); ++i) {
        ++ry;
        --rh;
    }
    for (uint i = w - 1; i > rx && is_transparent_column(i); --i)
        --rw;
    for (uint i = h - 1; i > ry && is_transparent_row(i); --i)
        --rh;

    return im_recti(rx, ry, rw, rh);
}

}  // namespace cws80
