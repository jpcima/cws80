#pragma once
#include "cws/cws80_program.h"
#include "cws/cws80_data.h"
#if 1
#include "dsp/lpcfmoog.h"
#else
#include "dsp/biquad-exec.h"
#endif
#include "utility/types.h"

namespace cws80 {

class Vcf {
public:
    typedef Program::Misc Param;

    Vcf();
    void initialize(f64 fs, uint bs);
    void setparam(const Param *p);
    void reset();
    void generate(i16 *outp, const i16 *inp, const i8 *modps[2],
                  const i8 modamts[2], uint key, uint n);  // range -63..+63

private:
    // parameters
    const Param *param_ = nullptr;
    // sample rate
    f64 fs_ = 44100;

    // cycle number
    uint cycle_ = 0;
    // update cycle number
    uint update_cycle_ = 0;

#if 1
    dsp::lpcfmoog::fast_filter filter_;
#else
    // biquad filter cascade
    dsp::biquad<f64> filter_[2];
#endif
};

}  // namespace cws80
