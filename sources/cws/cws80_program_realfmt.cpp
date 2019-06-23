#include "cws80_program.h"
#include "utility/arithmetic.h"

namespace cws80 {

i32 Program::get_parameter(uint idx) const
{
    const Program &x = *this;

#define PARAMETER_CASE(index, cat, name, gett, sett, min, max) \
    case index:                                                \
        return (gett);

    switch (idx) {
        EACH_PARAMETER(PARAMETER_CASE)
    default:
        return 0;
    }

#undef PARAMETER_CASE
}

bool Program::set_parameter(uint idx, i32 v)
{
    Program &x = *this;

#define PARAMETER_CASE(index, cat, name, gett, sett, min, max) \
    case index: {                                              \
        bool eq = (i32)(gett) == v;                            \
        (sett);                                                \
        return !eq;                                            \
    }

    switch (idx) {
        EACH_PARAMETER(PARAMETER_CASE)
    default:
        return false;
    }

#undef PARAMETER_CASE
}

std::pair<i32, i32> Program::get_parameter_range(uint idx)
{
#define PARAMETER_CASE(index, cat, name, gett, sett, min, max) \
    case index:                                                \
        return {min, max};

    switch (idx) {
        EACH_PARAMETER(PARAMETER_CASE)
    default:
        return {};
    }

#undef PARAMETER_CASE
}

const char *Program::get_parameter_name(uint idx)
{
#define PARAMETER_CASE(index, cat, name, gett, sett, min, max) \
    case index:                                                \
        return #cat "." #name;

    switch (idx) {
        EACH_PARAMETER(PARAMETER_CASE)
    default:
        return "";
    }

#undef PARAMETER_CASE
}

const char *Program::get_parameter_short_name(uint idx)
{
#define PARAMETER_CASE(index, cat, name, gett, sett, min, max) \
    case index:                                                \
        return #name;

    switch (idx) {
        EACH_PARAMETER(PARAMETER_CASE)
    default:
        return "";
    }

#undef PARAMETER_CASE
}

i32 Program::clamp_parameter_value(uint idx, i32 val)
{
    std::pair<i32, i32> range = get_parameter_range(idx);
    return clamp(val, range.first, range.second);
}

bool Program::set_u7_parameter(uint idx, int val7)
{
    std::pair<i32, i32> range = get_parameter_range(idx);
    i32 min = range.first, max = range.second;
    i32 val = clamp(min + val7 * (max - min) / 127, min, max);
    return set_parameter(idx, val);
}

bool Program::set_f32_parameter(uint idx, f32 valf)
{
    std::pair<i32, i32> range = get_parameter_range(idx);
    i32 min = range.first, max = range.second;
    i32 val = clamp((i32)(min + valf * (max - min)), min, max);
    return set_parameter(idx, val);
}

bool Program::apply_nrpn(int nrpn, int val7)
{
    uint idx7 = ~0u;

    switch (nrpn) {
#define ENV(I)                              \
    case 0 + 10 * (I - 1):                  \
        idx7 = P_Env##I##_L1;               \
        break;                              \
    case 1 + 10 * (I - 1):                  \
        idx7 = P_Env##I##_L2;               \
        break;                              \
    case 2 + 10 * (I - 1):                  \
        idx7 = P_Env##I##_L3;               \
        break;                              \
    case 3 + 10 * (I - 1):                  \
        idx7 = P_Env##I##_T1;               \
        break;                              \
    case 4 + 10 * (I - 1):                  \
        idx7 = P_Env##I##_T2;               \
        break;                              \
    case 5 + 10 * (I - 1):                  \
        idx7 = P_Env##I##_T3;               \
        break;                              \
    case 6 + 10 * (I - 1):                  \
        idx7 = P_Env##I##_T4;               \
        break;                              \
    case 7 + 10 * (I - 1):                  \
        idx7 = P_Env##I##_LV;               \
        break;                              \
    case 8 + 10 * (I - 1):                  \
        idx7 = P_Env##I##_T1V;              \
        break;                              \
    case 9 + 10 * (I - 1):                  \
        idx7 = P_Env##I##_TK;               \
        break;
#define LFO(I)                              \
    case 40 + 8 * (I - 1):                  \
        idx7 = P_Lfo##I##_FREQ;             \
        break;                              \
    case 41 + 8 * (I - 1):                  \
        idx7 = P_Lfo##I##_RESET;            \
        break;                              \
    case 42 + 8 * (I - 1):                  \
        idx7 = P_Lfo##I##_HUMAN;            \
        break;                              \
    case 43 + 8 * (I - 1):                  \
        idx7 = P_Lfo##I##_WAV;              \
        break;                              \
    case 44 + 8 * (I - 1):                  \
        idx7 = P_Lfo##I##_L1;               \
        break;                              \
    case 45 + 8 * (I - 1):                  \
        idx7 = P_Lfo##I##_DELAY;            \
        break;                              \
    case 46 + 8 * (I - 1):                  \
        idx7 = P_Lfo##I##_L2;               \
        break;                              \
    case 47 + 8 * (I - 1):                  \
        idx7 = P_Lfo##I##_MOD;              \
        break;
#define OSC(I)                              \
    case 64 + 8 * (I - 1):                  \
        idx7 = P_Osc##I##_OCT;              \
        break;                              \
    case 65 + 8 * (I - 1):                  \
        idx7 = P_Osc##I##_SEMI;             \
        break;                              \
    case 66 + 8 * (I - 1):                  \
        idx7 = P_Osc##I##_FINE;             \
        break;                              \
    case 67 + 8 * (I - 1):                  \
        idx7 = P_Osc##I##_WAVEFORM;         \
        break;                              \
    case 68 + 8 * (I - 1):                  \
        idx7 = P_Osc##I##_FMSRC1;           \
        break;                              \
    case 69 + 8 * (I - 1):                  \
        idx7 = P_Osc##I##_FCMODAMT1;        \
        break;                              \
    case 70 + 8 * (I - 1):                  \
        idx7 = P_Osc##I##_FMSRC2;           \
        break;                              \
    case 71 + 8 * (I - 1):                  \
        idx7 = P_Osc##I##_FCMODAMT2;        \
        break;                              \
    case 88 + 6 * (I - 1):                  \
        idx7 = P_Osc##I##_DCALEVEL;         \
        break;                              \
    case 89 + 6 * (I - 1):                  \
        idx7 = P_Osc##I##_DCAENABLE;        \
        break;                              \
    case 90 + 6 * (I - 1):                  \
        idx7 = P_Osc##I##_AMSRC1;           \
        break;                              \
    case 91 + 6 * (I - 1):                  \
        idx7 = P_Osc##I##_AMAMT1;           \
        break;                              \
    case 92 + 6 * (I - 1):                  \
        idx7 = P_Osc##I##_AMSRC2;           \
        break;                              \
    case 93 + 6 * (I - 1):                  \
        idx7 = P_Osc##I##_AMAMT2;           \
        break;

        ENV(1);
        ENV(2);
        ENV(3);
        ENV(4);
        LFO(1);
        LFO(2);
        LFO(3);
        OSC(1);
        OSC(2);
        OSC(3);

#undef ENV
#undef LFO
#undef OSC

    case 106:
        idx7 = P_Misc_DCA4MODAMT;
        break;
    case 107:
        idx7 = P_Misc_PAN;
        break;
    case 108:
        idx7 = P_Misc_PANMODSRC;
        break;
    case 109:
        idx7 = P_Misc_PANMODAMT;
        break;

    case 110:
        idx7 = P_Misc_FLTFC;
        break;
    case 111:
        idx7 = P_Misc_Q;
        break;
    case 112:
        idx7 = P_Misc_KEYBD;
        break;
    case 113:
        idx7 = P_Misc_FCSRC1;
        break;
    case 114:
        idx7 = P_Misc_FCMODAMT1;
        break;
    case 115:
        idx7 = P_Misc_FCSRC2;
        break;
    case 116:
        idx7 = P_Misc_FCMODAMT2;
        break;

    case 117:
        idx7 = P_Misc_AM;
        break;
    case 118:
        idx7 = P_Misc_GLIDE;
        break;
    case 119:
        idx7 = P_Misc_MONO;
        break;
    case 120:
        idx7 = P_Misc_SYNC;
        break;
    case 121:
        idx7 = P_Misc_VC;
        break;
    case 122:
        idx7 = P_Misc_ENV;
        break;
    case 123:
        idx7 = P_Misc_OSC;
        break;
    case 124:
        idx7 = P_Misc_CYCLE;
        break;

    case 125:
        idx7 = P_Misc_LAYER;
        break;
    case 126:
        idx7 = P_Misc_LAYERPRG;
        break;
    case 127:
        idx7 = P_Misc_SPLITLAYER;
        break;
    case 128:
        idx7 = P_Misc_SPLITLAYERPRG;
        break;
    case 129:
        idx7 = P_Misc_SPLITDIR;
        break;
    case 130:
        idx7 = P_Misc_SPLITPRG;
        break;
    case 131:
        idx7 = P_Misc_SPLITPOINT;
        break;

    default:
        break;
    }

    if (idx7 == ~0u)
        return false;

    return set_u7_parameter(idx7, val7);
}

}  // namespace cws80
