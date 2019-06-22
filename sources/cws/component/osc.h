#pragma once
#include "cws/cws80_program.h"
#include "cws/cws80_data.h"
#include "utility/types.h"
#include <memory>

namespace cws80 {

class Osc {
public:
    typedef Program::Osc Param;

    Osc();
    void initialize(f64 fs, uint bs);
    void setparam(const Param *p);
    void reset();
    void generate(i16 *outp, const i8 *syncinp, i8 *syncoutp,
                  const i8 *modps[2], const i8 modamts[2], uint key, uint n);
    // range -63..+63

private:
    // parameters
    const Param *param_ = nullptr;
    // phase
    u32 phase_ = 0;
    // phase increments normalized to fs
    u32 *osc_phi_ = nullptr;
};

}  // namespace cws80
