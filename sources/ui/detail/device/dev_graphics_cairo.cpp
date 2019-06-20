#include "dev_graphics_cairo.h"
#include "ui/cws80_ui_controller.h"
#include "ui/cws80_ui_nk.h"
#include "utility/scope_guard.h"
#include "utility/arithmetic.h"
#include "utility/c++std/optional.h"
#include <cairo/cairo.h>
#include <fmt/format.h>
#include <unordered_set>
#include <type_traits>
#include <cassert>

namespace cws80 {

///
struct Font {
    struct nk_user_font nk {};
    void *userdata = nullptr;
};

struct GraphicsDevice_Cairo::Impl {
    GraphicsDevice_Cairo *Q = nullptr;

    std::unordered_set<cairo_surface_t *> textures_;
    std::vector<nk_font *> font_;
    cxx::optional<nk_font_atlas> atlas_{};
    uint atlas_width_ = 0;
    uint atlas_height_ = 0;
    nk_draw_null_texture null_{};

    void load_fonts(gsl::span<const FontRequest> fontreqs, const nk_rune range[]);
};

GraphicsDevice_Cairo::GraphicsDevice_Cairo(UIController &ctl)
    : GraphicsDevice(ctl),
      P(new Impl)
{
    P->Q = this;
}

GraphicsDevice_Cairo::~GraphicsDevice_Cairo()
{
    cleanup();
}

void GraphicsDevice_Cairo::setup_context()
{
}

void GraphicsDevice_Cairo::initialize(gsl::span<const FontRequest> fontreqs, const nk_rune range[])
{
    P->load_fonts(fontreqs, range);
}

void GraphicsDevice_Cairo::cleanup()
{
    P->font_.clear();
    if (P->atlas_) {
        nk_font_atlas_clear(&*P->atlas_);
        P->atlas_.reset();
    }
    while (!P->textures_.empty()) {
        cairo_surface_t *img = *P->textures_.begin();
        cairo_surface_destroy(img);
        P->textures_.erase(P->textures_.begin());
    }
}

im_texture GraphicsDevice_Cairo::load_texture(const u8 *data, uint w, uint h, uint channels)
{
    bool success = false;
    cairo_surface_t *img = nullptr;

    SCOPE(exit) { if (img && !success) cairo_surface_destroy(img); };

    switch (channels) {
    case 1: {
        if (!(img = cairo_image_surface_create(CAIRO_FORMAT_A8, w, h)))
            throw std::runtime_error("cannot create cairo image");

        cairo_surface_flush(img);
        u8 *pixels = cairo_image_surface_get_data(img);
        uint stride = cairo_image_surface_get_stride(img);

        for (uint x = 0; x < w; ++x)
            for (uint y = 0; y < h; ++y)
                pixels[x + y * stride] = data[x + y * w];
        break;
    }
    case 3: {  // must convert RGB -> ARGB and byteswap
        if (!(img = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h)))
            throw std::runtime_error("cannot create cairo image");

        cairo_surface_flush(img);
        u8 *pixels = cairo_image_surface_get_data(img);
        uint stride = cairo_image_surface_get_stride(img);

        for (uint x = 0; x < w; ++x) {
            for (uint y = 0; y < h; ++y) {
                const u8 *src = &data[(x + y * w) * 3];
                u32 *pix = (u32 *)&pixels[x * 4 + y * stride];

                u32 r = src[0];
                u32 g = src[1];
                u32 b = src[2];

                *pix = (r << 16) | (g << 8) | (b << 0) | (0xff << 24);
            }
        }
        break;
    }
    case 4: {  // must convert RGBA -> ARGB and byteswap
        if (!(img = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h)))
            throw std::runtime_error("cannot create cairo image");

        cairo_surface_flush(img);
        u8 *pixels = cairo_image_surface_get_data(img);
        uint stride = cairo_image_surface_get_stride(img);

        for (uint x = 0; x < w; ++x) {
            for (uint y = 0; y < h; ++y) {
                const u8 *src = &data[(x + y * w) * 4];
                u32 *pix = (u32 *)&pixels[x * 4 + y * stride];

                u32 r = src[0];
                u32 g = src[1];
                u32 b = src[2];
                u32 a = src[3];
                // pre-multiply alpha
                r = (r * a) >> 8;
                g = (g * a) >> 8;
                b = (b * a) >> 8;

                *pix = (r << 16) | (g << 8) | (b << 0) | (a << 24);
            }
        }
        break;
    }
    default:
        throw std::runtime_error("unsupported image channel count");
    }

    cairo_surface_mark_dirty(img);

    im_texture tex = nk_image_ptr(img);
    tex->w = w;
    tex->h = h;
    tex->region[2] = w;
    tex->region[3] = h;

    assert(P->textures_.find(img) == P->textures_.end());
    P->textures_.insert(img);

    success = true;
    return tex;
}

void GraphicsDevice_Cairo::unload_texture(nk_handle handle)
{
    cairo_surface_t *img = (cairo_surface_t *)handle.ptr;
    cairo_surface_destroy(img);

    auto it = P->textures_.find(img);
    assert(it != P->textures_.end());
    P->textures_.erase(it);
}

void GraphicsDevice_Cairo::render(void *draw_context)
{
    NkScreen &screen = P->Q->ctl_.screen();
    nk_context *ctx = screen.context();
    cairo_t *cr = (cairo_t *)draw_context;
    const struct nk_command *cmd;

    const float k = 1.0 / 255;

    const im_rectf nr = nk_get_null_rect();
    im_rectf activeclip = nr;

    nk_foreach(cmd, ctx)
    {
        auto rounded_rectangle =
            +[](cairo_t *cr, double x, double y, double w, double h, double r)
                 {
                     double d = M_PI / 180.0;
                     cairo_new_path(cr);
                     cairo_arc(cr, x + w - r, y + r, r, -90 * d, 0 * d);
                     cairo_arc(cr, x + w - r, y + h - r, r, 0 * d, 90 * d);
                     cairo_arc(cr, x + r, y + h - r, r, 90 * d, 180 * d);
                     cairo_arc(cr, x + r, y + r, r, 180 * d, 270 * d);
                     cairo_close_path(cr);
                 };

        switch (cmd->type) {
        case NK_COMMAND_NOP: break;
        case NK_COMMAND_SCISSOR: {
            const struct nk_command_scissor *s = (const struct nk_command_scissor *)cmd;
            activeclip = nk_rect(s->x, s->y, s->w, s->h);
            cairo_reset_clip(cr);
            if (activeclip != nr) {
                cairo_rectangle(cr, activeclip.x, activeclip.y, activeclip.w, activeclip.h);
                cairo_clip(cr);
            }
        } break;
        case NK_COMMAND_LINE: {
            const struct nk_command_line *l = (const struct nk_command_line *)cmd;
            cairo_move_to(cr, l->begin.x, l->begin.y);
            cairo_line_to(cr, l->end.x, l->end.y);
            cairo_set_source_rgba(cr, k * l->color.r, k * l->color.g, k * l->color.b, k * l->color.a);
            cairo_set_line_width(cr, l->line_thickness);
            cairo_stroke(cr);
        } break;
        case NK_COMMAND_RECT: {
            const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
            if (r->rounding <= 0)
                cairo_rectangle(cr, r->x, r->y, r->w, r->h);
            else
                rounded_rectangle(cr, r->x, r->y, r->w, r->h, r->rounding);
            cairo_set_source_rgba(cr, k * r->color.r, k * r->color.g, k * r->color.b, k * r->color.a);
            cairo_set_line_width(cr, r->line_thickness);
            cairo_stroke(cr);
        } break;
        case NK_COMMAND_RECT_FILLED: {
            const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
            if (r->rounding <= 0)
                cairo_rectangle(cr, r->x, r->y, r->w, r->h);
            else
                rounded_rectangle(cr, r->x, r->y, r->w, r->h, r->rounding);
            cairo_set_source_rgba(cr, k * r->color.r, k * r->color.g, k * r->color.b, k * r->color.a);
            cairo_fill(cr);
        } break;
        case NK_COMMAND_CIRCLE: {
            //const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
            //TODO: circle
        } break;
        case NK_COMMAND_CIRCLE_FILLED: {
            //const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
            //TODO: circle filled
        } break;
        case NK_COMMAND_TRIANGLE: {
            const struct nk_command_triangle *t = (const struct nk_command_triangle *)cmd;
            cairo_move_to(cr, t->a.x, t->a.y);
            cairo_line_to(cr, t->b.x, t->b.y);
            cairo_line_to(cr, t->c.x, t->c.y);
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, k * t->color.r, k * t->color.g, k * t->color.b, k * t->color.a);
            cairo_set_line_width(cr, t->line_thickness);
            cairo_stroke(cr);
        } break;
        case NK_COMMAND_TRIANGLE_FILLED: {
            const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
            cairo_move_to(cr, t->a.x, t->a.y);
            cairo_line_to(cr, t->b.x, t->b.y);
            cairo_line_to(cr, t->c.x, t->c.y);
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, k * t->color.r, k * t->color.g, k * t->color.b, k * t->color.a);
            cairo_fill(cr);
        } break;
        case NK_COMMAND_POLYGON: {
            const struct nk_command_polygon *p = (const struct nk_command_polygon *)cmd;
            unsigned n = p->point_count;
            if (n == 0)
                break;
            cairo_move_to(cr, p->points[0].x, p->points[0].y);
            for (uint i = 1; i < n; ++i)
                cairo_line_to(cr, p->points[i].x, p->points[i].y);
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, k * p->color.r, k * p->color.g, k * p->color.b, k * p->color.a);
            cairo_set_line_width(cr, p->line_thickness);
            cairo_stroke(cr);
        } break;
        case NK_COMMAND_POLYGON_FILLED: {
            const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
            unsigned n = p->point_count;
            if (n == 0)
                break;
            cairo_move_to(cr, p->points[0].x, p->points[0].y);
            for (uint i = 1; i < n; ++i)
                cairo_line_to(cr, p->points[i].x, p->points[i].y);
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, k * p->color.r, k * p->color.g, k * p->color.b, k * p->color.a);
            cairo_fill(cr);
        } break;
        case NK_COMMAND_POLYLINE: {
            const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
            unsigned n = p->point_count;
            if (n == 0)
                break;
            cairo_move_to(cr, p->points[0].x, p->points[0].y);
            for (uint i = 1; i < n; ++i)
                cairo_line_to(cr, p->points[i].x, p->points[i].y);
            cairo_set_source_rgba(cr, k * p->color.r, k * p->color.g, k * p->color.b, k * p->color.a);
            cairo_set_line_width(cr, p->line_thickness);
            cairo_stroke(cr);
        } break;
        case NK_COMMAND_TEXT: {
            const struct nk_command_text *t = (const struct nk_command_text *)cmd;
            cairo_surface_t *tex = (cairo_surface_t *)t->font->texture.ptr;

            const struct nk_user_font *font = t->font;
            const float font_height = t->height;

            struct nk_font *nkfont = (struct nk_font *)font->userdata.ptr;
            const float scale = font_height / (nkfont->info.height * nkfont->config->oversample_v);

            uint atw = P->atlas_width_;
            uint ath = P->atlas_height_;

            struct nk_rect rect = nk_rect(t->x, t->y, t->w, t->h);
            const char *text = t->string;
            uint len = t->length;
            struct nk_color fg = t->foreground;

            float x = 0;

            nk_rune unicode = 0;
            uint glyph_len = nk_utf_decode(text, &unicode, len);

            for (uint text_len = 0; text_len < len && glyph_len;) {
                if (unicode == NK_UTF_INVALID) break;

                nk_rune next = 0;
                uint next_glyph_len = nk_utf_decode(text + text_len + glyph_len, &next, (int)len - text_len);

                struct nk_user_font_glyph g;
                font->query(font->userdata, font_height, &g, unicode,
                            (next == NK_UTF_INVALID) ? '\0' : next);

                float gx0 = atw * g.uv[0].x;
                float gy0 = ath * g.uv[0].y;
                float gx1 = atw * g.uv[1].x;
                float gy1 = ath * g.uv[1].y;
                float gw = gx1 - gx0;
                float gh = gy1 - gy0;

                cairo_matrix_t mat;
                cairo_get_matrix(cr, &mat);

                cairo_translate(cr, rect.x + x + g.offset.x, rect.y + g.offset.y);
                cairo_scale(cr, scale, scale);

                cairo_rectangle(cr, 0, 0, gw, gh);
                cairo_reset_clip(cr);
                cairo_clip_preserve(cr);

                cairo_set_source_rgba(cr, k * fg.r, k * fg.g, k * fg.b, k * fg.a);
                cairo_mask_surface(cr, tex, -gx0, -gy0);
                cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
                cairo_stroke(cr);

                cairo_set_matrix(cr, &mat);

                text_len += glyph_len;
                x += g.xadvance;
                glyph_len = next_glyph_len;
                unicode = next;
            }

            cairo_reset_clip(cr);
            if (activeclip != nr) {
                cairo_rectangle(cr, activeclip.x, activeclip.y, activeclip.w, activeclip.h);
                cairo_clip(cr);
            }
        } break;
        case NK_COMMAND_CURVE: {
            //const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
            //TODO: curve
        } break;
        case NK_COMMAND_IMAGE: {
            const struct nk_command_image *i = (const struct nk_command_image *)cmd;
            cairo_surface_t *img = (cairo_surface_t *)i->img.handle.ptr;
            cairo_matrix_t mat;
            cairo_get_matrix(cr, &mat);
            cairo_translate(cr, i->x, i->y);
            cairo_set_source_surface(cr, img, 0, 0);
            cairo_paint(cr);
            cairo_set_matrix(cr, &mat);
        } break;
        case NK_COMMAND_RECT_MULTI_COLOR:
        case NK_COMMAND_ARC:
        case NK_COMMAND_ARC_FILLED:
        default: break;
        }
    }

    nk_clear(ctx);
}

nk_user_font *GraphicsDevice_Cairo::get_font(uint id)
{
    std::vector<nk_font *> &fonts = P->font_;
    if (id >= fonts.size())
        return nullptr;
    return &fonts[id]->handle;
}

void GraphicsDevice_Cairo::Impl::load_fonts(gsl::span<const FontRequest> fontreqs, const nk_rune range[])
{
    bool success = false;
    nk_font_atlas &atlas = *(atlas_ = nk_font_atlas{});
    std::vector<nk_font *> &fonts = font_;

    nk_font_atlas_init_default(&atlas);
    SCOPE(exit)
    {
        if (!success) {
            nk_font_atlas_clear(&atlas);
            atlas_.reset();
            fonts.clear();
            null_ = nk_draw_null_texture{};
        };
    };

    nk_font_atlas_begin(&atlas);

    for (const FontRequest &req : fontreqs) {
        struct nk_font_config fcfg = nk_font_config(req.height);
        fcfg.range = range;
        fcfg.oversample_h = 2;
        fcfg.oversample_v = 2;

        nk_font *font = nullptr;
        switch (req.type) {
        case FontRequest::Type::Default:
            font = nk_font_atlas_add_default(&atlas, req.height, &fcfg);
            break;
        case FontRequest::Type::File:
            font = nk_font_atlas_add_from_file(&atlas, req.un.file.path, req.height, &fcfg);
            break;
        case FontRequest::Type::Memory:
            font = nk_font_atlas_add_from_memory(&atlas, (void *)req.un.memory.data,
                                                 req.un.memory.size, req.height, &fcfg);
            break;
        default:
            assert(false);
        }
        if (!font)
            throw std::runtime_error("could not load the requested font");
        fonts.push_back(font);
    }

    im_texture font_tex;
    {
        uint atw = 0, ath = 0;
        const u8 *image = (const u8 *)nk_font_atlas_bake(
            &atlas, (int *)&atw, (int *)&ath, NK_FONT_ATLAS_ALPHA8);
        if (!image)
            throw std::runtime_error("could not convert font into texture");
        font_tex = Q->load_texture(image, atw, ath, 1);
        atlas_width_ = atw;
        atlas_height_ = ath;
    }

    nk_draw_null_texture &null = null_;
    nk_font_atlas_end(&atlas, font_tex->handle, &null);

    success = true;
}

}  // namespace cws80
