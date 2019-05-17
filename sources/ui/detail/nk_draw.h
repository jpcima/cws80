#pragma once
#include "nkg_point.h"
#include "nkg_rect.h"
#include "nk_essential.h"
#include "utility/types.h"
#include "utility/c++std/string_view.h"
#include <nuklear.h>
#include <gsl/gsl>

namespace cws80 {

// drawing
void im_draw_point(nk_command_buffer *out, f32 x, f32 y, nk_color color);
void im_fill_polygon(nk_command_buffer *out, gsl::span<const im_pointf> points, nk_color color);
void im_draw_image(nk_command_buffer *out, im_rectf rect, const im_texture &tex,
                   nk_color color);
void im_draw_tiled_image(nk_command_buffer *out, im_rectf rect,
                         const im_texture &tex, nk_color color);
void im_draw_title_box(nk_context *ctx, im_rectf bounds, cxx::string_view title,
                       im_rectf *content_bounds = nullptr);
void im_draw_title_box_alt(nk_context *ctx, im_rectf bounds, cxx::string_view title);
void im_draw_led_screen(nk_context *ctx, im_rectf bounds, cxx::string_view text,
                        im_pointf textoff);

// drawing in screen coordinates
void ims_stroke_line(nk_context *ctx, f32 x0, f32 y0, f32 x1, f32 y1,
                     f32 line_thickness, nk_color color);
void ims_stroke_rect(nk_context *ctx, im_rectf rect, f32 rounding,
                     f32 line_thickness, nk_color color);
void ims_fill_rect(nk_context *ctx, im_rectf rect, f32 rounding, nk_color color);
void ims_fill_polygon(nk_context *ctx, gsl::span<const im_pointf> points, nk_color color);
void ims_draw_image(nk_context *ctx, im_rectf rect, const im_texture &tex, nk_color color);
void ims_draw_tiled_image(nk_context *ctx, im_rectf rect, const im_texture &tex,
                          nk_color color);
void ims_draw_text(nk_context *ctx, im_rectf rect, const char *text, int len,
                   const nk_user_font *fnt, nk_color bg, nk_color fg);
void ims_push_scissor(nk_context *ctx, im_rectf rect);
//
void ims_draw_title_box(nk_context *ctx, im_rectf bounds, cxx::string_view title,
                        im_rectf *content_bounds = nullptr);
void ims_draw_title_box_alt(nk_context *ctx, im_rectf bounds, cxx::string_view title);
void ims_draw_led_screen(nk_context *ctx, im_rectf bounds,
                         cxx::string_view text, im_pointf textoff);

// text
f32 im_text_width(const nk_user_font *fnt, cxx::string_view text);

}  // namespace cws80
