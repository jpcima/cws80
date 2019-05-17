#include "cws/component/env.h"
#include "cws/component/tables.h"
#include "utility/arithmetic.h"
#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <assert.h>

#define debug(fmt, ...)                          \
    do {                                         \
        fprintf(stderr, fmt "\n", ##__VA_ARGS__) \
    } while (0)
#undef debug

#pragma message("Env: implement SQ80 parameters LE, 2R")

namespace cws80 {

Env::Env()
    : param_(&initial_program().envs[0])
{
}

void Env::initialize(f64 fs, uint bs, const Env *other)
{
    (void)bs;

    if (other) {
        times_ = other->times_;
    }
    else {
        times_.reset(new u32[64]);
        initialize_tables(fs);
    }
}

void Env::setparam(const Param *p)
{
    param_ = p;
}

void Env::reset()
{
#ifdef debug
    if (state_ != State::Off)
        debug("%s -> %s", nameof(state_), nameof(State::Off));
#endif

    state_ = State::Off;
    rel_ = false;
    l_ = 0;
}

void Env::trigger(uint vel)
{
    // TODO parameters TK T1V
    (void)vel;

    const Param &param = *param_;
    const u32 *times = times_.get();

    // times in samples
    u32 t1 = std::max(times[param.T1], 1u);
    u32 t2 = std::max(times[param.T2], 1u);
    u32 t3 = std::max(times[param.T3], 1u);
    u32 t4 = std::max(times[param.T4], 1u);

    // levels in Q8,24 units
    i32 l1 = fx8(clamp<i8>(param.L1, -63, +63));
    i32 l2 = fx8(clamp<i8>(param.L2, -63, +63));
    i32 l3 = fx8(clamp<i8>(param.L3, -63, +63));

    // slopes in Q8,24 units per sample
    i32 r1 = l1 / (i32)t1;
    i32 r2 = (l2 - l1) / (i32)t2;
    i32 r3 = (l3 - l2) / (i32)t3;
    i32 r4 = -l3 / (i32)t4;

#ifdef debug
    if (state_ != State::Atk)
        debug("%s -> %s", nameof(state_), nameof(State::Atk));
#endif

    state_ = State::Atk;

    rel_ = false;
    l1_ = l1;
    l2_ = l2;
    l3_ = l3;
    r1_ = r1;
    r2_ = r2;
    r3_ = r3;
    r4_ = r4;
}

void Env::release(uint vel)
{
    (void)vel;

    rel_ = true;
}

auto Env::state() const -> State
{
    return state_;
}

bool Env::running() const
{
    return state_ != State::Off;
}

void Env::generate(i8 *outp, uint n)
{
    State state = state_;
    i32 l = l_;

    const i32 l1 = l1_;
    const i32 l2 = l2_;
    const i32 l3 = l3_;
    const i32 r1 = r1_;
    const i32 r2 = r2_;
    const i32 r3 = r3_;
    const i32 r4 = r4_;

    auto enter = [&state](State newstate) {
#ifdef debug
        if (newstate != state)
            debug("%s -> %s", nameof(state), nameof(newstate));
#endif
        state = newstate;
    };

    if (rel_)
        enter(State::Rel);

    for (uint i = 0; i < n; ++i) {
        switch (state) {
        case State::Off:
            l = 0;
            break;
        case State::Atk:
            if ((r1 < 0) ? (l > l1) : (l < l1)) {
                enter(State::Atk);
                l += r1;
                l = (r1 < 0) ? ((l < l1) ? l1 : l) : ((l > l1) ? l1 : l);
                break;
            }
            // fall through
        case State::Dcy:
            if ((r2 < 0) ? (l > l2) : (l < l2)) {
                enter(State::Dcy);
                l += r2;
                l = (r2 < 0) ? ((l < l2) ? l2 : l) : ((l > l2) ? l2 : l);
                break;
            }
            // fall through
        case State::At2:
            if ((r3 < 0) ? (l > l3) : (l < l3)) {
                enter(State::At2);
                l += r3;
                l = (r3 < 0) ? ((l < l3) ? l3 : l) : ((l > l3) ? l3 : l);
                break;
            }
            // fall through
        case State::Sus: {
            enter(State::Sus);
            l = l3;
            break;
        }
        case State::Rel: {
            enter(State::Rel);
            l += r4;
            l = (r4 < 0) ? ((l < 0) ? 0 : l) : ((l > 0) ? 0 : l);
            if (l == 0)
                enter(State::Off);
            break;
        }
        }
        outp[i] = ix8(l);
    }

    l_ = l;
    state_ = state;
}

f32 Env::timeval(uint i)
{
    assert(i < 64);
    return Env_times[i];
}

uint Env::timeidx(f32 t)
{
    uint i = 0;
    while (i < 64 - 1 && t > Env_times[i + 1])
        ++i;
    f32 t0 = Env_times[i];
    f32 t1 = Env_times[i + 1];
    return (t - t0 < t1 - t) ? i : (i + 1);
}

const char *Env::nameof(State s)
{
    static const char *names[] = {"Off", "Atk", "Dcy", "At2", "Sus", "Rel"};
    return names[(uint)s];
}

void Env::initialize_tables(f64 fs)
{
    u32 *times = times_.get();

    scoped_fesetround(FE_TONEAREST);
    for (uint i = 0; i < 64; ++i)
        times[i] = lrint(Env_times[i] * fs);
}

}  // namespace cws80
