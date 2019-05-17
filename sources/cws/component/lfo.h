#pragma once
#include "cws/cws80_program.h"
#include "cws/cws80_data.h"
#include "utility/types.h"
#include <boost/shared_ptr.hpp>
#include <memory>
#include <random>

namespace cws80 {

class Lfo {
public:
    typedef Program::Lfo Param;

    Lfo();
    void initialize(f64 fs, uint bs, Lfo *other);
    void setparam(const Param *p);
    void reset();
    void generate(i8 *outp, const i8 *const mod, uint n);  // range -63..+63
    static uint freqidx(f32 f);

private:
    // parameters
    const Param *param_ = nullptr;
    // Q8,24 phase
    u32 phase_ = 0;
    // noise generator
    std::minstd_rand noisernd_;
    // Q8,24 phase increments
    boost::shared_ptr<u32[]> lfo_phi_;
    //
    void initialize_tables(f64 fs);
};

}  // namespace cws80
