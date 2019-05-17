#pragma once
#include "nkg_point.h"
#include "nkg_rect.h"
#include <vector>

namespace cws80 {

// split a rectangle into N subdivisions with the given spacing in between
template <class T>
std::vector<im_rect<T>> hsubdiv(const im_rect<T> &r, uint n, f64 spacing);
template <class T>
std::vector<im_rect<T>> vsubdiv(const im_rect<T> &r, uint n, f64 spacing);

template <class T>
im_rect<T> centered_subrect(const im_rect<T> &rect, const im_point<T> &size);

}  // namespace cws80

#include "nkg_util.tcc"
