#pragma once
#include "cws/cws80_program.h"
#include "cws/cws80_data.h"
#include "utility/types.h"
#include <boost/shared_ptr.hpp>
#include <memory>

namespace cws80 {

struct Env {
    typedef Program::Env Param;

    enum class State { Off, Atk, Dcy, At2, Sus, Rel };

    Env();
    void initialize(f64 fs, uint bs, const Env *other);
    void setparam(const Param *p);
    void reset();
    void trigger(uint vel);
    void release(uint vel);
    State state() const;
    bool running() const;
    void generate(i8 *outp, uint n);  // range -63..+63
    static f32 timeval(uint i);  // range 0..63
    static uint timeidx(f32 t);
    static const char *nameof(State s);

private:
    // parameters
    const Param *param_ = nullptr;
    // which state it's currently in
    State state_ = State::Off;
    // levels in Q8, 24 units
    i32 l1_ = 0, l2_ = 0, l3_ = 0;
    // slopes in Q8,24 units per sample
    i32 r1_ = 0, r2_ = 0, r3_ = 0, r4_ = 0;
    // current level
    i32 l_ = 0;
    // whether key has been released yet
    bool rel_ = false;
    // Q16,16 envelope times normalized to sample rate
    boost::shared_ptr<u32[]> times_;
    //
    void initialize_tables(f64 fs);
};

}  // namespace cws80
