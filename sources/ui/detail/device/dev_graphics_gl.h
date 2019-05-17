#pragma once
#include "dev_graphics.h"
#include <memory>

typedef int GLint;
typedef uint GLuint;

//
namespace cws80 {

class GraphicsDevice_GL : public GraphicsDevice {
public:
    explicit GraphicsDevice_GL(UIController &ctl);
    ~GraphicsDevice_GL();

    inline GraphicsType type() const override { return GraphicsType::OpenGL; }

    void setup_context() override;
    void initialize(gsl::span<const FontRequest> fontreqs, const nk_rune range[]) override;
    void cleanup() override;

    im_texture load_texture(const u8 *data, uint w, uint h, uint channels) override;
    void unload_texture(nk_handle handle) override;

    void render() override;

    nk_user_font *get_font(uint id) override;

private:
    struct Impl;
    std::unique_ptr<Impl> P;
};

}  // namespace cws80
