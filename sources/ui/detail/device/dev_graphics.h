#pragma once
#include "ui/detail/nki_image.h"
#include "ui/detail/nk_essential.h"
#include <gsl/gsl>
#include <nuklear.h>

namespace cws80 {

class UIController;
struct FontRequest;

enum class GraphicsType {
    Other,
    OpenGL,
    Cairo,
    Gdiplus,
};

//
class GraphicsDevice {
public:
    explicit GraphicsDevice(UIController &ctl)
        : ctl_(ctl)
    {
    }
    virtual ~GraphicsDevice() {}

    virtual GraphicsType type() const = 0;

    virtual void setup_context() {}
    virtual void initialize(gsl::span<const FontRequest> fontreqs) = 0;
    virtual void cleanup() = 0;

    im_texture load_texture(const im_image &img);
    virtual im_texture load_texture(const u8 *data, uint w, uint h, uint channels) = 0;
    virtual void unload_texture(nk_handle handle) = 0;

    virtual void render(void *draw_context) = 0;

    virtual nk_user_font *get_font(uint id) = 0;

protected:
    UIController &ctl_;
};

//------------------------------------------------------------------------------
struct FontRequest {
    static FontRequest Default(f32 height, const nk_rune *range);
    static FontRequest File(f32 height, const char *path, const nk_rune *range);
    static FontRequest Memory(f32 height, const void *data, size_t size, const nk_rune *range);

    enum class Type { Default, File, Memory } type;
    f32 height;
    const nk_rune *range;

    union {
        struct {
            const char *path;
        } file;
        struct {
            const void *data;
            size_t size;
        } memory;
    } un;
};

inline auto FontRequest::Default(f32 height, const nk_rune *range) -> FontRequest
{
    FontRequest req;
    req.type = FontRequest::Type::Default;
    req.height = height;
    req.range = range;
    return req;
}

inline auto FontRequest::File(f32 height, const char *path, const nk_rune *range) -> FontRequest
{
    FontRequest req;
    req.type = FontRequest::Type::File;
    req.height = height;
    req.range = range;
    req.un.file.path = path;
    return req;
}

inline auto FontRequest::Memory(f32 height, const void *data, size_t size, const nk_rune *range) -> FontRequest
{
    FontRequest req;
    req.type = FontRequest::Type::Memory;
    req.height = height;
    req.range = range;
    req.un.memory.data = data;
    req.un.memory.size = size;
    return req;
}

}  // namespace cws80
