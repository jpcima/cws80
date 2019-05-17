#include "cws/component/sat.h"
#include "cws/component/tables.h"
#include "utility/arithmetic.h"
#include <math.h>

namespace cws80 {

enum {
    Sat_tablen = 32768,
    Sat_oversample = 4,
};

void Sat::initialize(f64 fs, uint bs, Sat *other)
{
    (void)fs;
    (void)bs;

#ifdef CWS_FIXED_POINT_FIR_FILTERS
    aaflt1_ = fir32l<i32>(Sat_aa4x.size());
    aaflt2_ = fir32l<i16>(Sat_aa4x.size());
#else
    aaflt1_ = realfir<f32>(Sat_aa4x.size());
    aaflt2_ = realfir<f32>(Sat_aa4x.size());
#endif

    if (other) {
        sat_table_ = other->sat_table_;
    }
    else {
        sat_table_.reset(new i16[Sat_tablen]);
        initialize_tables();
    }
}

void Sat::generate(const i32 *inp, i16 *outp, uint n)
{
    if (false) {  // hard clip
        for (uint i = 0; i < n; ++i)
            outp[i] = clamp<i32>(inp[i], -32767, 32767);
        return;
    }

    const i16 *sat_table = sat_table_.get();

#ifdef CWS_FIXED_POINT_FIR_FILTERS
    fir32l<i32> &aaflt1 = aaflt1_;
    fir32l<i16> &aaflt2 = aaflt2_;
#else
    realfir<f32> &aaflt1 = aaflt1_;
    realfir<f32> &aaflt2 = aaflt2_;
#endif

    for (uint i = 0; i < n; ++i) {
        i32 in = inp[i];  // -98301..+98301

        i32 satout[Sat_oversample];
        for (uint o = 0; o < Sat_oversample; ++o) {
#ifdef CWS_FIXED_POINT_FIR_FILTERS
            aaflt1.in((o == 0) ? in : 0);
            i32 satin = aaflt1.out(Sat_aa4x.data());
#else
            aaflt1.in((o == 0) ? (f32)in : 0);
            i32 satin = (i32)lrint(aaflt1.out(Sat_aa4x_real.data()));
#endif

            u1 sign = satin < 0;
            i32 absin = sign ? -satin : satin;
            i16 absout = sat_table[absin / 3];
            i16 out = sign ? -absout : absout;

            aaflt2.in(out);
#ifdef CWS_FIXED_POINT_FIR_FILTERS
            satout[o] = aaflt2.out(Sat_aa4x.data());
#else
            satout[o] = (i32)aaflt2.out(Sat_aa4x_real.data());
#endif
        }

        outp[i] = satout[0];
    }
}

void Sat::initialize_tables()
{
    i16 *sat_table = sat_table_.get();

    scoped_fesetround(FE_TONEAREST);
    for (uint i = 0; i < Sat_tablen; ++i) {
        f64 r = (f64)i / (Sat_tablen - 1);
        // f64 sat = tanh(r);
        // f64 sat = (r <= 1./3.) ? (2. * r) :
        //           (r <= 2./3.) ? ((3.- square(2. - 3. * r)) / 3.) : 1.;
        f64 sat = r - cube(r) / 3;
        sat_table[i] = (i16)lrint(sat * 32767);
    }
}

}  // namespace cws80
