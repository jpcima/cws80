#pragma once
#include "utility/filter.h"
#include "utility/types.h"
#include <memory>

namespace cws80 {

class Sat {
public:
    void initialize(f64 fs, uint bs);
    void reset() {}
    void generate(const i32 *inp, i16 *outp, uint n);

private:
    // saturation function
    i16 *sat_table_ = nullptr;
#ifdef CWS_FIXED_POINT_FIR_FILTERS
    // upsampling antialias filter
    fir32l<i32> aaflt1_;
    // downsampling antialias filter
    fir32l<i16> aaflt2_;
#else
    // upsampling antialias filter
    realfir<f32> aaflt1_;
    // downsampling antialias filter
    realfir<f32> aaflt2_;
#endif
};

}  // namespace cws80
