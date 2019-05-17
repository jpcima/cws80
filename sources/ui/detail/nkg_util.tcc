#include "nkg_util.h"

namespace cws80 {

template <class T>
std::vector<im_rect<T>> hsubdiv(const im_rect<T> &r, uint n, f64 spacing) {
  std::vector<im_rect<T>> v(n);
  f64 sptotal = spacing * (n - 1);
  f64 w = (r.w - sptotal) / n;
  f64 xoff = (r.w - (w * n + sptotal)) / 2;
  for (uint i = 0; i < n; ++i)
    v[i] = im_rect<T>(
        (T)(r.x + xoff + (w + spacing) * i), (T)r.y,
        (T)w, (T)r.h);
  return v;
}

template <class T>
std::vector<im_rect<T>> vsubdiv(const im_rect<T> &r, uint n, f64 spacing) {
  std::vector<im_rect<T>> v(n);
  f64 sptotal = spacing * (n - 1);
  f64 h = (r.h - sptotal) / n;
  f64 yoff = (r.h - (h * n + sptotal)) / 2;
  for (uint i = 0; i < n; ++i)
    v[i] = im_rect<T>(
        (T)r.x, (T)(r.y + yoff + (h + spacing) * i),
        (T)r.w, (T)h);
  return v;
}

template <class T>
im_rect<T> centered_subrect(const im_rect<T> &rect, const im_point<T> &size) {
  return rect
      .off_by(im_point<T>((rect.w - size.x) / 2, (rect.h - size.y) / 2))
      .resized(size);
}

}  // namespace cws80
