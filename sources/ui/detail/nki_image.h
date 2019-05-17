#pragma once
#include "nkg_point.h"
#include "nkg_rect.h"
#include "utility/types.h"
#include <stb_image.h>
#include <gsl/gsl>
#include <memory>
#include <vector>
#include <stdlib.h>

namespace cws80 {

class im_image {
public:
    im_image();
    ~im_image();

    void load(const char *filename, uint desired_channels);
    void load_from_memory(const u8 *buffer, uint length, uint desired_channels);
    void load_from_memory(gsl::span<const u8> memory, uint desired_channels)
    {
        load_from_memory(memory.data(), memory.size(), desired_channels);
    }

    const u8 *pixel(uint x, uint y) const;
    u8 alpha(uint x, uint y) const;

    bool is_transparent_column(uint x) const;
    bool is_transparent_row(uint y) const;

    im_image cut(const im_recti &r) const;
    std::vector<im_image> hsplit() const;
    std::vector<im_image> vsplit() const;

    im_recti rect() const;
    im_recti crop_alpha_border() const;

    const u8 *data() const { return data_.get(); }
    uint width() const { return w_; }
    uint height() const { return h_; }
    uint channels() const { return c_; }

    im_image(im_image &&other) = default;
    im_image &operator=(im_image &&other) = default;

private:
    uint w_{}, h_{}, n_{}, c_{};
    std::unique_ptr<u8[], void (*)(void *)> data_{nullptr, &stbi_image_free};

public:
    class exception : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };
};

}  // namespace cws80
