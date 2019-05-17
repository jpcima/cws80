#pragma once
#include "utility/scope_guard.h"
#include "utility/types.h"
#include <gsl/gsl>
#include <cfenv>
#include <cassert>

#define scoped_fesetround(mode)                                  \
    int SCOPE_GUARD_PP_UNIQUE(_rounding_mode) = fegetround();    \
    int SCOPE_GUARD_PP_UNIQUE(_rounding_ret) = fesetround(mode); \
    (void)SCOPE_GUARD_PP_UNIQUE(_rounding_ret);                  \
    assert(SCOPE_GUARD_PP_UNIQUE(_rounding_ret) == 0);           \
    SCOPE(exit) { fesetround(SCOPE_GUARD_PP_UNIQUE(_rounding_mode)); }

template <class T> inline T square(T a) { return a * a; }

template <class T> inline T cube(T a) { return a * a * a; }

template <class T> inline T clamp(T x, T min, T max)
{
    x = (x < min) ? min : x;
    x = (x > max) ? max : x;
    return x;
}

// linear interpolator
template <class R> R itp_linear(const R y[], R mu)
{
    return y[0] * (1 - mu) + y[1] * mu;
}

// Catmull-Rom interpolator
template <class R> R itp_catmull(const R y[], R mu)
{
    R mu2 = mu * mu;
    R mu3 = mu2 * mu;
    R a[] = {-R(0.5) * y[0] + R(1.5) * y[1] - R(1.5) * y[2] + R(0.5) * y[3],
             y[0] - R(2.5) * y[1] + R(2) * y[2] - R(0.5) * y[3],
             -R(0.5) * y[0] + R(0.5) * y[2], y[1]};
    return a[0] * mu3 + a[1] * mu2 + a[2] * mu + a[3];
}

//------------------------------------------------------------------------------
// Fixed point routines
//   suffix 'xN', N = bit size of the integer part
//   prefix 'l' = 64-bit operation, 32-bit otherwise
//
//   'fx' number to fixed
//   'ix' integer part, 'rx' fractional part
//   'ffx' fixed to f32, 'dfx' fixed to f64

// number to Q32,32
template <class R> inline constexpr i64 lfx32(R x)
{
    return i64(x * (i64)4294967296);
}
// number to Q16,16
template <class R> inline constexpr i32 fx16(R x)
{
    return i32(x * (i32)65536);
}
// number to Q16,48
template <class R> inline constexpr i64 lfx16(R x)
{
    return i64(x * (i64)281474976710656);
}
// number to Q8,24
template <class R> inline constexpr i32 fx8(R x)
{
    return i32(x * (i32)16777216);
}
// number to Q8,56
template <class R> inline constexpr i64 lfx8(R x)
{
    return i64(x * (i64)72057594037927936);
}

// Q32,32 integer part
inline constexpr i64 lix32(i64 x) { return x / 4294967296; }
inline constexpr u64 lix32(u64 x) { return x / 4294967296; }
template <class T> T lix32(T) = delete;
// Q16,16 integer part
inline constexpr i32 ix16(i32 x) { return x / 65536; }
inline constexpr u32 ix16(u32 x) { return x / 65536; }
template <class T> T ix16(T) = delete;
// Q16,48 integer part
inline constexpr i64 lix16(i64 x) { return x / 281474976710656; }
inline constexpr u64 lix16(u64 x) { return x / 281474976710656; }
template <class T> T lix16(T) = delete;
// Q8,24 integer part
inline constexpr i32 ix8(i32 x) { return x / 16777216; }
inline constexpr u32 ix8(u32 x) { return x / 16777216; }
template <class T> T ix8(T) = delete;
// Q8,56 integer part
inline constexpr i64 lix8(i64 x) { return x / 72057594037927936; }
inline constexpr u64 lix8(u64 x) { return x / 72057594037927936; }
template <class T> T lix8(T) = delete;

// Q32,32 fractional part
inline constexpr u64 lrx32(i64 x) { return x & 4294967295; }
inline constexpr u64 lrx32(u64 x) { return x & 4294967295; }
template <class T> T lrx32(T) = delete;
// Q16,16 fractional part
inline constexpr u32 rx16(i32 x) { return x & 65535; }
inline constexpr u32 rx16(u32 x) { return x & 65535; }
template <class T> T rx16(T) = delete;
// Q16,48 fractional part
inline constexpr u64 lrx16(i64 x) { return x & 281474976710655; }
inline constexpr u64 lrx16(u64 x) { return x & 281474976710655; }
template <class T> T lrx16(T) = delete;
// Q8,24 fractional part
inline constexpr u32 rx8(i32 x) { return x & 16777215; }
inline constexpr u32 rx8(u32 x) { return x & 16777215; }
template <class T> T rx8(T) = delete;
// Q8,56 fractional part
inline constexpr u64 lrx8(i64 x) { return x & 72057594037927935; }
inline constexpr u64 lrx8(u64 x) { return x & 72057594037927935; }
template <class T> T lrx8(T) = delete;

// Q32,32 number to real
inline constexpr f32 lffx32(i64 x) { return x / 4294967296.0f; }
inline constexpr f32 lffx32(u64 x) { return x / 4294967296.0f; }
template <class T> f32 lffx32(T x) = delete;
inline constexpr f64 ldfx32(i64 x) { return x / 4294967296.0; }
inline constexpr f64 ldfx32(u64 x) { return x / 4294967296.0; }
template <class T> f64 ldfx32(T x) = delete;
// Q16,16 number to real
inline constexpr f32 ffx16(i32 x) { return x / 65536.0f; }
inline constexpr f32 ffx16(u32 x) { return x / 65536.0f; }
template <class T> f32 ffx16(T x) = delete;
inline constexpr f64 dfx16(i32 x) { return x / 65536.0; }
inline constexpr f64 dfx16(u32 x) { return x / 65536.0; }
template <class T> f64 dfx16(T x) = delete;
// Q16,48 number to real
inline constexpr f32 lffx16(i64 x) { return x / 281474976710656.0f; }
inline constexpr f32 lffx16(u64 x) { return x / 281474976710656.0f; }
template <class T> f32 lffx16(T x) = delete;
inline constexpr f64 ldfx16(i64 x) { return x / 281474976710656.0; }
inline constexpr f64 ldfx16(u64 x) { return x / 281474976710656.0; }
template <class T> f64 ldfx16(T x) = delete;
// Q8,24 number to real
inline constexpr f32 ffx8(i32 x) { return x / 16777216.0f; }
inline constexpr f32 ffx8(u32 x) { return x / 16777216.0f; }
template <class T> f32 ffx8(T x) = delete;
inline constexpr f64 dfx8(i32 x) { return x / 16777216.0; }
inline constexpr f64 dfx8(u32 x) { return x / 16777216.0; }
template <class T> f64 dfx8(T x) = delete;
// Q8,56 number to real
inline constexpr f32 lffx8(i64 x) { return x / 72057594037927936.0f; }
inline constexpr f32 lffx8(u64 x) { return x / 72057594037927936.0f; }
template <class T> f32 lffx8(T x) = delete;
inline constexpr f64 ldfx8(i64 x) { return x / 72057594037927936.0; }
inline constexpr f64 ldfx8(u64 x) { return x / 72057594037927936.0; }
template <class T> f64 ldfx8(T x) = delete;
