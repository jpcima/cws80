#include "cws/component/dca.h"
#include "utility/arithmetic.h"

#pragma message("TODO implement DCA")

namespace cws80 {

Dca::Dca()
    : param_(&initial_program().oscs[0])
{
}

void Dca::setparam(const Param *p)
{
    param_ = p;
}

void Dca::generate(i16 *outp, const i16 *inp, const i16 *amp,
                   const i8 *modps[2], const i8 modamts[2], uint n)
{
    const Param &P = *param_;

    bool enable = P.DCAENABLE;
    uint level = P.DCALEVEL;  // 0..63
    // NOTE: level=63 is full volume (outp -32767..32767)

    const i8 *mod1 = modps[0];
    const i8 *mod2 = modps[1];
    i8 modamt1 = clamp<i8>(modamts[0], -63, +63);
    i8 modamt2 = clamp<i8>(modamts[1], -63, +63);

    for (uint i = 0; i < n; ++i) {
        int mod = mod1[i] * modamt1 + mod2[i] * modamt2;  // -7938..+7938
        mod = mod * 127 / 7938;  // -127..127

#pragma message("TODO DCA AM")
        (void)amp;

        uint levelmod = clamp(2 * (int)level + mod, 0, 127);
        levelmod = enable ? levelmod : 0;

        outp[i] = inp[i] * (int)levelmod / 127;
    }
}

}  // namespace cws80
