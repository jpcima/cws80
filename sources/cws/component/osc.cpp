#include "cws/component/osc.h"
#include "utility/arithmetic.h"
#include <map>
#include <memory>
#include <mutex>
#include <math.h>

#pragma message("TODO implement OSC")

namespace cws80 {

// resolution per interval of a semitone
static constexpr uint osc_phi_oversample = 8;
static constexpr uint osc_phi_tablen = 128 * osc_phi_oversample;

///
struct OscConstant {
    u32 osc_phi[128 * osc_phi_oversample];
    explicit OscConstant(f64 fs);
};
static std::map<f64, std::unique_ptr<OscConstant>> Osc_const;
static std::mutex Osc_const_mutex;

///
Osc::Osc()
    : param_(&initial_program().oscs[0])
{
}

void Osc::initialize(f64 fs, uint bs)
{
    (void)bs;

    std::lock_guard<std::mutex> lock(Osc_const_mutex);
    std::unique_ptr<OscConstant> &constant = Osc_const[fs];
    if (!constant) constant.reset(new OscConstant(fs));
    osc_phi_ = constant->osc_phi;
}

void Osc::setparam(const Param *p)
{
    param_ = p;
}

void Osc::reset()
{
    phase_ = 0;
}

void Osc::generate(i16 *outp, const i8 *syncinp, i8 *syncoutp,
                   const i8 *modps[2], const i8 modamts[2], uint key, uint n)
{
    const Param &P = *param_;

    uint waveform = P.WAVEFORM;

    const i8 *mod1 = modps[0];
    const i8 *mod2 = modps[1];
    i8 modamt1 = clamp<i8>(modamts[0], -63, +63);
    i8 modamt2 = clamp<i8>(modamts[1], -63, +63);

    Waveset waveset = waveset_by_id(waveform);
    u8 wavenum = waveset.wavenum[16 * key / 128];

    Wave wave = wave_by_id(wavenum);
    Sample sample = wave_sample(wave);
    // bool oneshot = wave_oneshot(wavenum);

    u32 phase = phase_;
    const u32 *osc_phi = osc_phi_;

    for (uint i = 0; i < n; ++i) {
        int mod = mod1[i] * modamt1 + mod2[i] * modamt2;  // -7938..+7938
        mod = mod * 127 / 7938;  // -127..127

        uint pitch = key * osc_phi_oversample;
#pragma message("TODO OSC semi/fine (wave)")
#pragma message("TODO OSC semi/fine (program)")
#pragma message("TODO OSC pitch mods")

        u32 phaseinc = osc_phi[clamp<uint>(pitch, 0, osc_phi_tablen - 1)];

        bool syncd = syncinp[i] > 0;
        u32 oldphase = phase;
        phase = syncd ? 0 : (phase + phaseinc);  // aliased sync
        bool wrapd = syncd | (phase < oldphase);
        syncoutp[i] = wrapd;

        int out;
        if (false) {  // no interpolation
            uint index = phase >> (32 - sample.log2length);
            out = (int)sample.data[index] * 65534 / 255 - 32767;
        }
        else {  // linear interpolation
            uint length = 1 << sample.log2length;
            uint shift = 32 - sample.log2length;

            u32 i0 = phase >> shift;
            u32 i1 = (i0 < length - 1) ? (i0 + 1) : 0;

            int s0 = (int)sample.data[i0] * 65534 / 255 - 32767;
            int s1 = (int)sample.data[i1] * 65534 / 255 - 32767;

            uint frac = (phase >> (shift - 16)) & 65535;
            out = ix16(s1 * (i32)frac + s0 * (i32)(65536 - frac));
        }

        syncoutp[i] = wrapd;
        outp[i] = out;
    }

    phase_ = phase;
}

OscConstant::OscConstant(f64 fs)
{
    for (uint i = 0; i < 128; ++i) {
        for (uint j = 0, o = osc_phi_oversample; j < o; ++j) {
            uint idx = j + i * o;
            f64 key = i + (f64)j / o;
            f64 freq = 440 * exp2((key - 69) / 12);
            f64 rate = freq / fs;
            osc_phi[idx] = (u32)(rate * UINT32_MAX);
        }
    }
}

}  // namespace cws80
