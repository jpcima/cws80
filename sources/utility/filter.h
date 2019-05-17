#pragma once
#include "utility/arithmetic.h"
#include "utility/types.h"
#include <memory>

template <class S> struct basic_fir_fx {
    basic_fir_fx() {}
    explicit basic_fir_fx(uint taps)
        : n_(taps)
        , h_(new S[2 * taps]())
    {
    }

    void reset();
    void in(S x);
    template <class C> S out(const C *coef) const;

    uint i_ = 0, n_ = 0;
    std::unique_ptr<S[]> h_;
};

//------------------------------------------------------------------------------
template <class S> struct fir16;  // Q16,16
template <class S> struct fir16l;  // Q16,48
template <class S> struct fir32l;  // Q32,32
template <class S> struct realfir;  // real

//------------------------------------------------------------------------------
template <class S> inline void basic_fir_fx<S>::reset()
{
    S *h = h_.get();
    for (uint i = 0, n = 2 * n_; i < n; ++i)
        h[i] = 0;
}

template <class S> inline void basic_fir_fx<S>::in(S x)
{
    uint n = n_;
    uint i = (i_ - 1) % n;
    h_[i] = x;
    h_[i + n] = x;
    i_ = i;
}

//------------------------------------------------------------------------------
template <class S> struct fir16 : public basic_fir_fx<S> {
    using basic_fir_fx<S>::basic_fir_fx;
    template <class C> S out(const C *coef) const;
};

template <class S>
template <class C>
inline S fir16<S>::out(const C *__restrict coef) const
{
    i32 sum = 0;
    uint i = this->i_;
    const uint n = this->n_;
    const S *__restrict h = this->h_.get();
#pragma omp simd reduction(+ : sum)
    for (uint j = 0; j < n; ++j)
        sum += (i32)coef[j] * (i32)h[i + j];
    return (S)ix16(sum);
}

//------------------------------------------------------------------------------
template <class S> struct fir16l : public basic_fir_fx<S> {
    using basic_fir_fx<S>::basic_fir_fx;
    template <class C> S out(const C *coef) const;
};

template <class S>
template <class C>
inline S fir16l<S>::out(const C *__restrict coef) const
{
    i32 sum = 0;
    uint i = this->i_;
    const uint n = this->n_;
    const S *__restrict h = this->h_.get();
#pragma omp simd reduction(+ : sum)
    for (uint j = 0; j < n; ++j)
        sum += (i32)coef[j] * (i32)h[i + j];
    return (S)ix16(sum);
}

//------------------------------------------------------------------------------
template <class S> struct fir32l : public basic_fir_fx<S> {
    using basic_fir_fx<S>::basic_fir_fx;
    template <class C> S out(const C *coef) const;
};

template <class S>
template <class C>
inline S fir32l<S>::out(const C *__restrict coef) const
{
    i64 sum = 0;
    uint i = this->i_;
    const uint n = this->n_;
    const S *__restrict h = this->h_.get();
#pragma omp simd reduction(+ : sum)
    for (uint j = 0; j < n; ++j)
        sum += (i64)coef[j] * (i64)h[i + j];
    return (S)lix32(sum);
}

//------------------------------------------------------------------------------
template <class S> struct realfir : public basic_fir_fx<S> {
    using basic_fir_fx<S>::basic_fir_fx;
    template <class C> S out(const C *coef) const;
};

template <class S>
template <class C>
inline S realfir<S>::out(const C *__restrict coef) const
{
    S sum = 0;
    uint i = this->i_;
    const uint n = this->n_;
    const S *__restrict h = this->h_.get();
#pragma omp simd reduction(+ : sum)
    for (uint j = 0; j < n; ++j)
        sum += coef[j] * h[i + j];
    return (S)sum;
}
