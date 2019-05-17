#pragma once
#include "cws/cws80_program.h"
#include "cws/cws80_data.h"
#include "utility/types.h"

namespace cws80 {

class Dca4 {
public:
    typedef Program::Misc Param;

    Dca4();
    void initialize(f64 /*fs*/, uint /*bs*/) {}
    void setparam(const Param *p);
    void reset() {}
    void generate_adding(i16 *outl, i16 *outr, const i16 *in, const i8 *envp,
                         const i8 *panmodp, uint n);

private:
    // parameters
    const Param *param_ = nullptr;
};

}  // namespace cws80
