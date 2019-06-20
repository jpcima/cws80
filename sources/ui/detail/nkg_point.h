#pragma once
#include "utility/types.h"
#include <nuklear.h>
#include <iosfwd>

namespace cws80 {

template <class T> class im_point;
typedef im_point<f32> im_pointf;
typedef im_point<short> im_points;
typedef im_point<int> im_pointi;

template <class T> struct im_point_traits;

//------------------------------------------------------------------------------
template <class T> class im_point : public im_point_traits<T>::storage_type {
public:
    typedef typename im_point_traits<T>::storage_type storage_type;

    im_point()
        : storage_type{}
    {
    }
    im_point(storage_type s)
        : storage_type(s)
    {
    }
    im_point(T x, T y)
        : storage_type{x, y}
    {
    }
};

//------------------------------------------------------------------------------
template <class T>
bool operator==(const im_point<T> &a, const im_point<T> &b)
{
    return a.x == b.x && a.y == b.y;
}

template <class T>
bool operator!=(const im_point<T> &a, const im_point<T> &b)
{
    return !operator==(a, b);
}

//------------------------------------------------------------------------------
template <> struct im_point_traits<f32> {
    typedef struct nk_vec2 storage_type;
};

template <> struct im_point_traits<short> {
    typedef struct nk_vec2i storage_type;
};

template <class T> struct im_point_traits {
    struct storage_type {
        T x, y;
    };
};

//------------------------------------------------------------------------------
template <class T, class Ch>
std::basic_ostream<Ch> &operator<<(std::basic_ostream<Ch> &o, const im_point<T> &p)
{
    return o << "{" << p.x << ", " << p.y << "}";
}

}  // namespace cws80
