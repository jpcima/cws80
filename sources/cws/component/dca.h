#pragma once
#include "cws/cws80_program.h"
#include "cws/cws80_data.h"
#include "utility/types.h"

namespace cws80 {

class Dca {
public:
    typedef Program::Osc Param;

    Dca();
    void initialize(f64 /*fs*/, uint /*bs*/) {}
    void setparam(const Param *p);
    void reset() {}
    void generate(i16 *outp, const i16 *inp, const i16 *amp, const i8 *modps[2],
                  const i8 modamts[2], uint n);

private:
    // parameters
    const Param *param_ = nullptr;
};

}  // namespace cws80
