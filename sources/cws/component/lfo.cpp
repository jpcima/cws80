#include "cws/component/lfo.h"
#include "cws/component/tables.h"
#include "utility/arithmetic.h"
#include <math.h>

#pragma message("TODO implement LFO: L1, L2, DELAY, MOD")

namespace cws80 {

static bool Lfo_tables_init = false;
static i8 Lfo_tri[256];
static void Lfo_generate_tri();

Lfo::Lfo()
    : param_(&initial_program().lfos[0])
{
    if (!Lfo_tables_init) {
        Lfo_generate_tri();
        Lfo_tables_init = true;
    }
}

void Lfo::initialize(f64 fs, uint bs, Lfo *other)
{
    (void)bs;

    if (other) {
        lfo_phi_ = other->lfo_phi_;
    }
    else {
        lfo_phi_.reset(new u32[64]);
        initialize_tables(fs);
    }
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

void Lfo::initialize_tables(f64 fs)
{
    u32 *lfo_phi = lfo_phi_.get();

    scoped_fesetround(FE_TONEAREST);
    for (uint i = 0; i < 64; ++i)
        lfo_phi[i] = fx8(256 * Lfo_freqs[i] / fs);
}

static void Lfo_generate_tri()
{
    i8 *tri = Lfo_tri;
    scoped_fesetround(FE_TONEAREST);

    for (uint i = 0; i < 256; ++i) {
        int n = clamp((int)i, 0, 64) - clamp((int)i - 64, 0, 128) +
                clamp((int)i - 192, 0, 63);
        f64 val = n * 127. / 64.;
        tri[i] = (i8)lrint(val);
    }
}

}  // namespace cws80
