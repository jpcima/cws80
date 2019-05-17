#pragma once
#include "utility/types.h"
#include <gsl/gsl>
#include <memory>

struct nk_context;
struct nk_user_font;
struct nk_buffer;

///
namespace cws80 {

class GraphicsDevice;
struct FontRequest;

//
class NkScreen {
public:
    NkScreen();
    ~NkScreen();

    void init(GraphicsDevice &gdev, uint w, uint h, gsl::span<const FontRequest> fontreqs);
    void clear();
    bool should_render() const;
    void render();
    void update();
    nk_context *context() const;
    nk_user_font *font(uint id) const;

    uint width() const;
    uint height() const;

private:
    struct Impl;
    std::unique_ptr<Impl> P;
};

}  // namespace cws80
