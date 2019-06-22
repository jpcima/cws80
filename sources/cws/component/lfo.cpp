#include "cws/component/lfo.h"
#include "cws/component/tables.h"
#include "utility/arithmetic.h"
#include <map>
#include <memory>
#include <mutex>
#include <math.h>

#pragma message("TODO implement LFO: L1, L2, DELAY, MOD")

namespace cws80 {

///
struct LfoConstant {
    u32 lfo_phi[64];
    explicit LfoConstant(f64 fs);
};
static std::map<f64, std::unique_ptr<LfoConstant>> Lfo_const;
static std::mutex Lfo_const_mutex;

///
static volatile bool Lfo_wavetables_init = false;
static std::mutex Lfo_wavetables_mutex;
static void Lfo_generate_waves();
static i8 Lfo_tri[256];

///
Lfo::Lfo()
    : param_(&initial_program().lfos[0])
{
    Lfo_generate_waves();
}

void Lfo::initialize(f64 fs, uint bs)
{
    (void)bs;

    std::lock_guard<std::mutex> lock(Lfo_const_mutex);
    std::unique_ptr<LfoConstant> &constant = Lfo_const[fs];
    if (!constant) constant.reset(new LfoConstant(fs));
    lfo_phi_ = constant->lfo_phi;
}

void Lfo::setparam(const Param *p)
{
    param_ = p;
}

void Lfo::reset()
{
    phase_ = 0;
}

void Lfo::generate(i8 *outp, const i8 *mod, uint n)
{
    u32 phase = phase_;

    const Param &P = *param_;
    auto &noisernd = noisernd_;
#pragma message("TODO: mods (case of self modulation?)")
    (void)mod; // mod: add to LFO depth

    u32 phi = lfo_phi_[P.FREQ];

    switch ((LfoWave)P.WAV) {
    case LfoWave::TRI: {
        for (uint i = 0; i < n; ++i) {
            outp[i] = Lfo_tri[ix8(phase)];
            phase += phi;
        }
        break;
    }
    case LfoWave::SAW: {
        for (uint i = 0; i < n; ++i) {
            uint saw = 255 - ix8(phase);
            outp[i] = (int)(saw * 254 / 255) - 127;
            phase += phi;
        }
        break;
    }
    case LfoWave::SQR: {
        for (uint i = 0; i < n; ++i) {
            outp[i] = (phase < (u32)fx8(128)) ? 0 : 127;
            phase += phi;
        }
        break;
    }
    case LfoWave::NOI: {
        for (uint i = 0; i < n; ++i) {
            uint noi = noisernd() % 255;
            outp[i] = (int)noi - 127;
            phase += phi;
        }
        break;
    }
    }

    /* outp : -127..+127 */

    for (uint i = 0; i < n; ++i) {
        outp[i] /= 2;
    }

    phase_ = phase;
}

uint Lfo::freqidx(f32 f)
{
    uint i = 0;
    while (i < 64 - 1 && f > Lfo_freqs[i + 1])
        ++i;
    f32 f0 = Lfo_freqs[i];
    f32 f1 = Lfo_freqs[i + 1];
    return (f - f0 < f1 - f) ? i : (i + 1);
}

LfoConstant::LfoConstant(f64 fs)
{
    scoped_fesetround(FE_TONEAREST);
    for (uint i = 0; i < 64; ++i)
        lfo_phi[i] = fx8(256 * Lfo_freqs[i] / fs);
}

static void Lfo_generate_waves()
{
    if (Lfo_wavetables_init)
        return;

    std::lock_guard<std::mutex> lock(Lfo_wavetables_mutex);
    if (Lfo_wavetables_init)
        return;

    scoped_fesetround(FE_TONEAREST);

    for (uint i = 0; i < 256; ++i) {
        int n = clamp((int)i, 0, 64) - clamp((int)i - 64, 0, 128) +
                clamp((int)i - 192, 0, 63);
        f64 val = n * 127. / 64.;
        Lfo_tri[i] = (i8)lrint(val);
    }
}

}  // namespace cws80
