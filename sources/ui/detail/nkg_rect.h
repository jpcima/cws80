#pragma once
#include "nkg_point.h"
#include "utility/types.h"
#include <nuklear.h>
#include <algorithm>
#include <iostream>

namespace cws80 {

template <class T> class im_rect;
typedef im_rect<f32> im_rectf;
typedef im_rect<short> im_rects;
typedef im_rect<int> im_recti;

template <class T> struct im_rect_traits;

//------------------------------------------------------------------------------
template <class T> class im_rect : public im_rect_traits<T>::storage_type {
public:
    typedef typename im_rect_traits<T>::storage_type storage_type;

    im_rect()
        : storage_type{}
    {
    }
    im_rect(storage_type s)
        : storage_type(s)
    {
    }
    im_rect(T x, T y, T w, T h)
        : storage_type{x, y, w, h}
    {
    }

    T top() const { return this->y; }
    T left() const { return this->x; }
    T bottom() const { return this->y + this->h - 1; }
    T right() const { return this->x + this->w - 1; }
    T xcenter() const { return this->x + (this->w - 1) / 2; }
    T ycenter() const { return this->y + (this->h - 1) / 2; }
    im_point<T> origin() const { return {this->x, this->y}; }
    im_point<T> size() const { return {this->w, this->h}; }
    im_rect<T> intersect(const im_rect<T> &r) const
    {
        im_rect<T> ri;
        ri.x = std::max(this->x, r.x);
        ri.y = std::max(this->y, r.y);
        ri.w = std::min(this->right(), r.right()) - ri.x + 1;
        ri.h = std::min(this->bottom(), r.bottom()) - ri.y + 1;
        return ri;
    }
    im_rect<T> unite(const im_rect<T> &r) const
    {
        im_rect<T> ri;
        ri.x = std::min(this->x, r.x);
        ri.y = std::min(this->y, r.y);
        ri.w = std::max(this->right(), r.right()) - ri.x + 1;
        ri.h = std::max(this->bottom(), r.bottom()) - ri.y + 1;
        return ri;
    }
    im_rect<T> from_top(T q) const
    {
        return im_rect<T>(this->x, this->y, this->w, q);
    }
    im_rect<T> from_bottom(T q) const
    {
        return im_rect<T>(this->x, this->y + this->h - q, this->w, q);
    }
    im_rect<T> from_left(T q) const
    {
        return im_rect<T>(this->x, this->y, q, this->h);
    }
    im_rect<T> from_right(T q) const
    {
        return im_rect<T>(this->x + this->w - q, this->y, q, this->h);
    }
    im_rect<T> take_from_top(T q)
    {
        im_rect<T> r = from_top(q);
        this->y += q;
        this->h -= q;
        return r;
    }
    im_rect<T> take_from_bottom(T q)
    {
        im_rect<T> r = from_bottom(q);
        this->h -= q;
        return r;
    }
    im_rect<T> take_from_left(T q)
    {
        im_rect<T> r = from_left(q);
        this->x += q;
        this->w -= q;
        return r;
    }
    im_rect<T> take_from_right(T q)
    {
        im_rect<T> r = from_right(q);
        this->w -= q;
        return r;
    }
    im_rect<T> &chop_from_top(T q)
    {
        take_from_top(q);
        return *this;
    }
    im_rect<T> &chop_from_bottom(T q)
    {
        take_from_bottom(q);
        return *this;
    }
    im_rect<T> &chop_from_left(T q)
    {
        take_from_left(q);
        return *this;
    }
    im_rect<T> &chop_from_right(T q)
    {
        take_from_right(q);
        return *this;
    }
    im_rect<T> from_center(T qx, T qy)
    {
        return im_rect<T>(this->x + (this->w - qx) / 2,
                          this->y + (this->h - qy) / 2, qx, qy);
    }
    im_rect<T> from_hcenter(T q)
    {
        return im_rect<T>(this->x + (this->w - q) / 2, this->y, q, this->h);
    }
    im_rect<T> from_vcenter(T q)
    {
        return im_rect<T>(this->x, this->y + (this->h - q) / 2, this->w, q);
    }
    im_rect<T> repositioned(const im_point<T> &p) const
    {
        return im_rect<T>(p.x, p.y, this->w, this->h);
    }
    im_rect<T> resized(const im_point<T> &p) const
    {
        return im_rect<T>(this->x, this->y, p.x, p.y);
    }
    im_rect<T> off_by(const im_point<T> &p) const
    {
        return im_rect<T>(this->x + p.x, this->y + p.y, this->w, this->h);
    }
    im_rect<T> expanded(const im_point<T> &p) const
    {
        return im_rect<T>(this->x - p.x, this->y - p.y, this->w + 2 * p.x,
                          this->h + 2 * p.y);
    }
    im_rect<T> expanded(T q) const { return expanded(im_point<T>(q, q)); }
    im_rect<T> reduced(const im_point<T> &p) const
    {
        return expanded(im_point<T>(-p.x, -p.y));
    }
    im_rect<T> reduced(T q) const { return expanded(-q); }
};

//------------------------------------------------------------------------------
template <class T>
bool operator==(const im_rect<T> &a, const im_rect<T> &b)
{
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

template <class T>
bool operator!=(const im_rect<T> &a, const im_rect<T> &b)
{
    return !operator==(a, b);
}

//------------------------------------------------------------------------------
template <> struct im_rect_traits<f32> {
    typedef struct nk_rect storage_type;
};

template <> struct im_rect_traits<short> {
    typedef struct nk_recti storage_type;
};

template <class T> struct im_rect_traits {
    struct storage_type {
        T x, y, w, h;
    };
};

//------------------------------------------------------------------------------
template <class T, class Ch>
std::basic_ostream<Ch> &operator<<(std::basic_ostream<Ch> &o, const im_rect<T> &r)
{
    return o << "{" << r.x << ", " << r.y << ", " << r.w << ", " << r.h << "}";
}

}  // namespace cws80
