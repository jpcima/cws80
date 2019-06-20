#pragma once
#include "dev_graphics.h"
#include "utility/types.h"
#include <memory>

//
namespace cws80 {

class GraphicsDevice_Cairo : public GraphicsDevice {
public:
    explicit GraphicsDevice_Cairo(UIController &ctl);
    ~GraphicsDevice_Cairo();

    inline GraphicsType type() const override { return GraphicsType::Cairo; }

    void setup_context() override;
    void initialize(gsl::span<const FontRequest> fontreqs, const nk_rune range[]) override;
    void cleanup() override;

    im_texture load_texture(const u8 *data, uint w, uint h, uint channels) override;
    void unload_texture(nk_handle handle) override;

    void render(void *draw_context) override;

    nk_user_font *get_font(uint id) override;

private:
    struct Impl;
    std::unique_ptr<Impl> P;
};

}  // namespace cws80
