#pragma once
#include <nuklear.h>
#include <stddef.h>

namespace cws80 {

class im_texture {
public:
    im_texture() {}
    im_texture(const struct nk_image &tex)
        : tex_(tex)
    {
    }

    struct nk_image *get() { return &tex_; }
    struct nk_image &operator*() { return tex_; }
    struct nk_image *operator->() { return &tex_; }

    const struct nk_image *get() const { return &tex_; }
    const struct nk_image &operator*() const { return tex_; }
    const struct nk_image *operator->() const { return &tex_; }

private:
    struct nk_image tex_ {
    };
};

}  // namespace cws80
