#include "nk_draw.h"
#include "utility/dynarray.h"
#include <memory>
#include <assert.h>

namespace cws80 {

void im_draw_point(nk_command_buffer *out, f32 x, f32 y, nk_color color)
{
    nk_stroke_line(out, x, y, x + 1, y + 1, 1, color);
}

void im_fill_polygon(nk_command_buffer *out, gsl::span<const im_pointf> points, nk_color color)
{
    uint np = points.size();
    dynarray<f32> pbuf(2 * np);
    for (size_t i = 0; i < np; ++i) {
        pbuf[2 * i] = points[i].x;
        pbuf[2 * i + 1] = points[i].y;
    }
    nk_fill_polygon(out, pbuf.data(), np, color);
}

void im_draw_image(nk_command_buffer *out, im_rectf rect, const im_texture &tex, nk_color color)
{
    nk_draw_image(out, rect, tex.get(), color);
}

void im_draw_tiled_image(nk_command_buffer *out, im_rectf rect,
                         const im_texture &tex, nk_color color)
{
    im_rectf old_clip = out->clip;
    nk_push_scissor(out, rect);
    f32 bgw = tex->w;
    f32 bgh = tex->h;
    for (f32 x = rect.x; x < rect.x + rect.w; x += bgw)
        for (f32 y = rect.y; y < rect.y + rect.h; y += bgh)
            nk_draw_image(out, im_rectf(x, y, bgw, bgh), &*tex, color);
    nk_push_scissor(out, old_clip);
}

void im_draw_title_box(nk_context *ctx, im_rectf bounds, cxx::string_view title,
                       im_rectf *content_bounds)
{
    assert(ctx);
    assert(ctx->style.font);

    nk_command_buffer *out = nk_window_get_canvas(ctx);
    const nk_user_font *font = ctx->style.font;

    nk_fill_rect(out, bounds, 0, nk_rgb(0x40, 0x40, 0x40));
    nk_stroke_rect(out, bounds, 0, 1, nk_rgb(0xa0, 0xa0, 0xa0));
    im_rectf rt = bounds.take_from_top(25).reduced(1);
    nk_fill_rect(out, rt.reduced(1), 0, nk_rgb(0xa0, 0x40, 0x40));
    nk_draw_text(out, rt.reduced({8, 0}), title.data(), title.size(), font,
                 nk_rgba_u32(0), nk_rgb(0xc0, 0xc0, 0xc0));

    if (content_bounds)
        *content_bounds = bounds.reduced({2, 0}).chop_from_bottom(1);
}

void im_draw_title_box_alt(nk_context *ctx, im_rectf bounds, cxx::string_view title)
{
    assert(ctx);
    assert(ctx->style.font);

    nk_command_buffer *out = nk_window_get_canvas(ctx);
    const nk_user_font *font = ctx->style.font;

    f32 textw = im_text_width(font, title);

    // nk_fill_rect(out, bounds, 0, nk_rgb(0x40, 0x40, 0x40));
    nk_stroke_rect(out, bounds, 0, 1, nk_rgb(0xa0, 0xa0, 0xa0));
    im_rectf rt = bounds.from_top(25).reduced(1).from_left(textw + 20);
    nk_fill_rect(out, rt, 0, nk_rgb(0xa0, 0x40, 0x40));
    nk_draw_text(out, rt.reduced({8, 0}), title.data(), title.size(), font,
                 nk_rgba_u32(0), nk_rgb(0xc0, 0xc0, 0xc0));
}

void im_draw_led_screen(nk_context *ctx, im_rectf bounds, cxx::string_view text, im_pointf textoff)
{
    assert(ctx);
    assert(ctx->style.font);

    nk_command_buffer *out = nk_window_get_canvas(ctx);
    const nk_user_font *font = ctx->style.font;

    cxx::string_view fill = u8"\u009f";
    // cxx::string_view under = u8"\u0332";

    f32 glyphw = im_text_width(font, fill);
    f32 glyphh = font->height;

    size_t cols = (size_t)(bounds.w / glyphw);
    size_t rows = (size_t)(bounds.h / glyphh);

    f32 yoff = textoff.y + 0.5f * (bounds.h - glyphh * rows);
    f32 xoff = textoff.x + 0.5f * (bounds.w - glyphw * cols);

    im_rectf old_clip = out->clip;
    nk_push_scissor(out, bounds);
    nk_fill_rect(out, bounds, 0, nk_rgb(0, 0, 0));

    nk_color bgcolor = nk_rgb(0x00, 0x1e, 0x15);
    nk_color fgcolor = nk_rgb(0x23, 0xff, 0xc5);
    // nk_color fgcolor = nk_rgb(0x80, 0xff, 0xdd);

    std::string emptyline;
    emptyline.reserve(cols * fill.size());
    for (size_t i = 0; i < cols; ++i)
        emptyline.append(fill.data(), fill.size());

    for (size_t i = 0; i < rows; ++i) {
        im_rectf linebounds = bounds.off_by({xoff, yoff + i * glyphh});
        nk_draw_text(out, linebounds, emptyline.data(), emptyline.size(), font,
                     nk_rgba_u32(0), bgcolor);
    }

    cxx::string_view line = text;
    for (size_t i = 0; i < rows; ++i) {
        size_t endpos = line.find('\n');
        cxx::string_view rest =
            (endpos == line.npos) ? cxx::string_view() : line.substr(endpos + 1);
        line = line.substr(0, endpos);
        im_rectf linebounds = bounds.off_by({xoff, yoff + i * glyphh});
        nk_draw_text(out, linebounds, line.data(), line.size(), font,
                     nk_rgba_u32(0), fgcolor);
        line = rest;
    }

    nk_push_scissor(out, old_clip);
}

//------------------------------------------------------------------------------
void ims_stroke_line(nk_context *ctx, f32 x0, f32 y0, f32 x1, f32 y1,
                     f32 line_thickness, nk_color color)
{
    nk_command_buffer *out = nk_window_get_canvas(ctx);
    im_pointf p0 = nk_layout_space_to_screen(ctx, im_pointf(x0, y0));
    im_pointf p1 = nk_layout_space_to_screen(ctx, im_pointf(x1, y1));
    nk_stroke_line(out, p0.x, p0.y, p1.x, p1.y, line_thickness, color);
}

void ims_stroke_rect(nk_context *ctx, im_rectf rect, f32 rounding,
                     f32 line_thickness, nk_color color)
{
    nk_command_buffer *out = nk_window_get_canvas(ctx);
    rect = nk_layout_space_rect_to_screen(ctx, rect);
    nk_stroke_rect(out, rect, rounding, line_thickness, color);
}

void ims_fill_rect(nk_context *ctx, im_rectf rect, f32 rounding, nk_color color)
{
    nk_command_buffer *out = nk_window_get_canvas(ctx);
    rect = nk_layout_space_rect_to_screen(ctx, rect);
    nk_fill_rect(out, rect, rounding, color);
}

void ims_fill_polygon(nk_context *ctx, gsl::span<const im_pointf> points, nk_color color)
{
    nk_command_buffer *out = nk_window_get_canvas(ctx);
    uint np = points.size();
    dynarray<im_pointf> pbuf(np);
    for (uint i = 0; i < np; ++i)
        pbuf[i] = nk_layout_space_to_screen(ctx, points[i]);
    gsl::span<const im_pointf> scrpoints(pbuf.data(), np);
    im_fill_polygon(out, scrpoints, color);
}

void ims_draw_image(nk_context *ctx, im_rectf rect, const im_texture &tex, nk_color color)
{
    nk_command_buffer *out = nk_window_get_canvas(ctx);
    im_draw_image(out, rect, tex, color);
}

void ims_draw_tiled_image(nk_context *ctx, im_rectf rect, const im_texture &tex, nk_color color)
{
    nk_command_buffer *out = nk_window_get_canvas(ctx);
    rect = nk_layout_space_rect_to_screen(ctx, rect);
    im_draw_tiled_image(out, rect, tex, color);
}

void ims_draw_text(nk_context *ctx, im_rectf rect, const char *text, int len,
                   const nk_user_font *fnt, nk_color bg, nk_color fg)
{
    nk_command_buffer *out = nk_window_get_canvas(ctx);
    rect = nk_layout_space_rect_to_screen(ctx, rect);
    nk_draw_text(out, rect, text, len, fnt, bg, fg);
}

void ims_push_scissor(nk_context *ctx, im_rectf rect)
{
    nk_command_buffer *out = nk_window_get_canvas(ctx);
    rect = nk_layout_space_rect_to_screen(ctx, rect);
    nk_push_scissor(out, rect);
}

void ims_draw_title_box(nk_context *ctx, im_rectf bounds,
                        cxx::string_view title, im_rectf *content_bounds)
{
    bounds = nk_layout_space_rect_to_screen(ctx, bounds);
    im_draw_title_box(ctx, bounds, title, content_bounds);
    if (content_bounds)
        *content_bounds = nk_layout_space_rect_to_local(ctx, *content_bounds);
}

void ims_draw_title_box_alt(nk_context *ctx, im_rectf bounds, cxx::string_view title)
{
    bounds = nk_layout_space_rect_to_screen(ctx, bounds);
    im_draw_title_box_alt(ctx, bounds, title);
}

void ims_draw_led_screen(nk_context *ctx, im_rectf bounds, cxx::string_view text, im_pointf textoff)
{
    bounds = nk_layout_space_rect_to_screen(ctx, bounds);
    im_draw_led_screen(ctx, bounds, text, textoff);
}

//------------------------------------------------------------------------------
f32 im_text_width(const nk_user_font *fnt, cxx::string_view text)
{
    return fnt->width(fnt->userdata, fnt->height, text.data(), text.size());
}

}  // namespace cws80
