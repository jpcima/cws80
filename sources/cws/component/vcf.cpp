#include "cws/component/vcf.h"
#include "cws/component/tables.h"
#if 1
#else
#include "dsp/biquad-design.h"
#endif
#include "utility/arithmetic.h"

#pragma message("TODO implement VCF")

namespace cws80 {

// on the KEYBD parameter (approx from spectral analysis)
//  adjusted cutoff Fc' = Fc * (1 + a * NOTE * KEYBD)
//  with a ~= 0.0002, KEYBD (-63..+63)

Vcf::Vcf()
    : param_(&initial_program().misc)
{
}

void Vcf::initialize(f64 fs, uint bs)
{
    (void)bs;
    fs_ = fs;
    const f64 interval = 3e-4;  // update interval
    update_cycle_ = (uint)lround(fs * interval);
}

void Vcf::setparam(const Param *p)
{
    param_ = p;
}

void Vcf::reset()
{
}

void Vcf::generate(i16 *outp, const i16 *inp, const i8 *modps[2],
                   const i8 modamts[2], uint key, uint n)
{
    const Param &P = *param_;
    f64 fs = fs_;

#pragma message("TODO VCF")
    // for (uint i = 0; i < n; ++i)
    //   outp[i] = inp[i];
    // return;

    const i8 *mod1 = modps[0];
    const i8 *mod2 = modps[1];
    i8 modamt1 = clamp<i8>(modamts[0], -63, +63);
    i8 modamt2 = clamp<i8>(modamts[1], -63, +63);
    uint fltfc = P.FLTFC;
    f64 q = P.Q / 31.0;  // 0..1  TODO Q range?
    // NOTE(ext): SQ80 only has positive tracking, SQ8L has both
    i8 keybd = clamp<i8>(P.KEYBD, -63, +63);

    uint cycle = cycle_;
    const uint update_cycle = update_cycle_;

#if 1
    dsp::lpcfmoog::fast_filter &filter = filter_;
#else
    dsp::biquad<f64>(&filter)[2] = filter_;
#endif

    for (uint i = 0; i < n; ++i) {
        int mod = mod1[i] * modamt1 + mod2[i] * modamt2;  // -7938..+7938
        mod = mod * 127 / 7938;  // -127..127

        uint fcidx = clamp<int>((int)fltfc + mod, 0, 127);  // TODO mod range?
        f64 fc = Vcf_freqs[fcidx] / fs;
        fc *= 1.0 + 0.0002 * key * keybd;
        fc = clamp(fc, 0.0, 0.5);

        // TODO SQ80 filter
#if 1
        const f64 scale = 32767;
        f64 out = scale * filter.tick(inp[i] * (1.0 / scale));
#else
        f64 out = filter[1].tick(filter[0].tick(inp[i]));
#endif
        // hard clip
        outp[i] = (i16)clamp<long>(lrint(out), -32768, 32767);

        if (++cycle == update_cycle) {
#if 1
            const f64 qmin = 0.2;
            const f64 qmax = 0.8;
            filter.lp(fc, q * (qmax - qmin) + qmin);  // Q range?
#else
            dsp::biquad_design dsn;
            dsn.lp(fc, sqrt(8.0 * (q + 1.0)));  // Q range?
            dsn.apply_to(filter[0]);
            dsn.apply_to(filter[1]);
#endif
        }
    }

    cycle_ = cycle;
}

}  // namespace cws80
