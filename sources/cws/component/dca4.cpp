#include "cws/component/dca4.h"
#include "cws/component/tables.h"
#include "utility/arithmetic.h"

namespace cws80 {

static constexpr uint Pan_center_idx = (Pan_table.size() - 1) / 2;

Dca4::Dca4()
    : param_(&initial_program().misc)
{
}

void Dca4::setparam(const Param *p)
{
    param_ = p;
}

void Dca4::generate_adding(i16 *outl, i16 *outr, const i16 *in, const i8 *envp,
                           const i8 *panmodp, uint n)
{
    const Param &P = *param_;

    uint dca4modamt = P.DCA4MODAMT;  // 0..63
    int panmodamt = clamp<i8>(P.PANMODAMT, -63, +63);  // -63..63

    // PAN centered at 8 and symmetric at 0
    int pan = clamp((int)P.PAN - 8, -7, +7);  // -7..+7

    for (uint i = 0; i < n; ++i) {
        int am = envp[i] * (int)dca4modamt;  // -3969..+3969
        int dcaout = in[i] * am / 3969;

#pragma message("TODO: PAN modulation")
        (void)panmodp; (void)panmodamt;
        // panmodp[i] * panmodamt;  // -3969..+3969

        uint panidx = (int)Pan_center_idx + pan * (int)Pan_center_idx / 7;

        int panr = Pan_table[panidx];
        int panl = Pan_table[511 - panidx];

        outl[i] += ix16(dcaout * panl);
        outr[i] += ix16(dcaout * panr);
    }
}

}  // namespace cws80
