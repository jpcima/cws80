#include "cws80_program.h"
#include "cws80_data.h"
#include <algorithm>
#include <cstring>

namespace cws80 {

constexpr uint Program::SQ8L_patch_size;

/*  SQ8L patch from SQ80 factory instruments */
static const u8 SQ8L_base_patch[Program::SQ8L_patch_size] = {
         //   $0   $1   $2   $3   $4   $5   $6   $7   $8   $9   $A   $B   $C   $D   $E   $F
/* $000: */ 0x02,0x00,0x00,0x00,0x09,0x53,0x51,0x38,0x4C,0x2E,0x45,0x44,0x49,0x54,0x00,0x00,
/* $010: */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
/* $020: */ 0x00,0x06,0x42,0x45,0x4C,0x54,0x4F,0x4D,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* $030: */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* $040: */ 0x00,0xFF,0x00,0x00,0x2F,0x00,0x00,0x00,0x02,0x04,0x00,0x04,0x00,0x00,0x00,0x36,
/* $050: */ 0x01,0x0C,0x00,0x18,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x05,0x44,0x00,0xFF,0xFF,
/* $060: */ 0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x3F,0x01,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0x00,
/* $070: */ 0x00,0xFF,0x00,0x00,0x2F,0x00,0x01,0x00,0x01,0xFF,0xFF,0x00,0x00,0x00,0x00,0x34,
/* $080: */ 0x01,0x0C,0x00,0x13,0xFF,0xFF,0x00,0x00,0x00,0x00,0xCE,0x00,0x00,0x00,0x1E,0x00,
/* $090: */ 0x00,0x11,0x1B,0x1C,0x09,0x00,0x00,0x00,0x3F,0x32,0x2D,0x00,0x00,0x00,0x00,0x32,
/* $0A0: */ 0x3F,0x0C,0x09,0x00,0x00,0x00,0x1A,0x3F,0x3F,0x00,0x00,0x3B,0x23,0x2C,0x3F,0x57,
/* $0B0: */ 0x00,0x00,0x00,0x00,0x2A,0x3D,0x2B,0x00,0x16,0x3F,0x26,0x1D,0x3F,0x5F,0x09,0x00,
/* $0C0: */ 0x00,0x14,0xFF,0x01,0x00,0x00,0x14,0x1F,0x00,0x00,0x3F,0xFF,0xFF,0x00,0x00,0x00,
/* $0D0: */ 0x00,0x16,0xFF,0x01,0x00,0x00,0x01,0x26,0x11,0x00,0x3F,0xFF,0xFF,0x00,0x00,0x00,
/* $0E0: */ 0x00,0x06,0xFF,0x01,0x00,0x3F,0x00,0x3F,0xFF,0xFF,0x3F,0xFF,0xFF,0x00,0x00,0x00,
/* $0F0: */ 0x00,0x18,0xFF,0x00,0x00,0x00,0x3F,0x3F,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0x00,0x00,
/* $100: */ 0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,
/* $110: */ 0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,
/* $120: */ 0xFF,0x00,0xFF,0xFF,0x00,0x2F,0x03,0x1A,0x00,0x06,0x00,0x3F,0x06,0x00,0x3F,0x00,
/* $130: */ 0xFF,0xFF,0x00,0x7F,0x00,0x00,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,
/* $140: */ 0x00,0x00,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* $150: */ 0x00,0x33,0x00,0xFF,0xFF,0x00,0x02,0x00,0x06,0x00,0x04,0x00,0x00,0x00,0x00,0x00,
/* $160: */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
/* $170: */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
/* $180: */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
/* $190: */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* $1A0: */ 0x00,0x04,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x00,0x02,
/* $1B0: */ 0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x78,
/* $1C0: */ 0x02,0x00,0x20,0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,
/* $1D0: */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* $1E0: */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,
/* $1F0: */ 0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,
/* $200: */ 0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,
/* $210: */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* $220: */ 0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,
/* $230: */ 0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0xFF,0x00
};

void Program::save_SQ8L(u8 *data)
{
    memcpy(data, SQ8L_base_patch, SQ8L_patch_size);

    uint namelen = std::min(15u, name_length());
    data[0x21] = namelen;
    memset(&data[0x22], 0, 15);
    memcpy(&data[0x22], NAME, namelen);

    auto write_mod =
        [](u8 *data, u32 off, i32 mod) {
            data[off] = (mod != (uint)Mod::OFF) ? mod : 0xff;
            data[off + 1] = (mod != (uint)Mod::OFF) ? 0 : 0xff;
        };

    for (uint osc = 0; osc < 3; ++osc) {
        uint d_off = osc * 24;
        uint p_off = osc * (P_Osc1_OCT - P_Osc0_OCT);
        data[0x41 + d_off] = get_parameter(p_off + P_Osc0_OCT);
        data[0x42 + d_off] = get_parameter(p_off + P_Osc0_SEMI);
        data[0x43 + d_off] = get_parameter(p_off + P_Osc0_FINE);
        data[0x44 + d_off] = get_parameter(p_off + P_Osc0_WAVEFORM);
        write_mod(data, 0x46 + d_off, get_parameter(p_off + P_Osc0_FMSRC1));
        data[0x48 + d_off] = get_parameter(p_off + P_Osc0_FCMODAMT1);
        write_mod(data, 0x49 + d_off, get_parameter(p_off + P_Osc0_FMSRC2));
        data[0x4B + d_off] = get_parameter(p_off + P_Osc0_FCMODAMT2);
        data[0x4F + d_off] = get_parameter(p_off + P_Osc0_DCALEVEL);
        data[0x50 + d_off] = get_parameter(p_off + P_Osc0_DCAENABLE);
        write_mod(data, 0x51 + d_off, get_parameter(p_off + P_Osc0_AMSRC1));
        data[0x53 + d_off] = get_parameter(p_off + P_Osc0_AMAMT1);
        write_mod(data, 0x54 + d_off, get_parameter(p_off + P_Osc0_AMSRC2));
        data[0x56 + d_off] = get_parameter(p_off + P_Osc0_AMAMT2);
    }

    for (uint env = 0; env < 4; ++env) {
        uint d_off = env * 14;
        uint p_off = env * (P_Env1_LE - P_Env0_LE);
        data[0x8A + d_off] = get_parameter(p_off + P_Env0_L1);
        data[0x8B + d_off] = get_parameter(p_off + P_Env0_L2);
        data[0x8C + d_off] = get_parameter(p_off + P_Env0_L3);
        data[0x8E + d_off] = get_parameter(p_off + P_Env0_LV) | (get_parameter(p_off + P_Env0_LE) << 6);
        data[0x8F + d_off] = get_parameter(p_off + P_Env0_T1V);
        data[0x90 + d_off] = get_parameter(p_off + P_Env0_T1);
        data[0x91 + d_off] = get_parameter(p_off + P_Env0_T2);
        data[0x92 + d_off] = get_parameter(p_off + P_Env0_T3);
        data[0x93 + d_off] = get_parameter(p_off + P_Env0_T4) | (get_parameter(p_off + P_Env0_R2) << 6);
        data[0x94 + d_off] = get_parameter(p_off + P_Env0_TK);
    }

    for (uint lfo = 0; lfo < 3; ++lfo) {
        uint d_off = lfo * 16;
        uint p_off = lfo * (P_Lfo1_FREQ - P_Lfo0_FREQ);
        data[0xC1 + d_off] = get_parameter(p_off + P_Lfo0_FREQ);
        data[0xC2 + d_off] = get_parameter(p_off + P_Lfo0_RESET) ? 0 : 0xff;
        data[0xC3 + d_off] = get_parameter(p_off + P_Lfo0_HUMAN);
        data[0xC4 + d_off] = get_parameter(p_off + P_Lfo0_WAV);
        data[0xC5 + d_off] = get_parameter(p_off + P_Lfo0_L1);
        data[0xC6 + d_off] = get_parameter(p_off + P_Lfo0_DELAY);
        data[0xC7 + d_off] = get_parameter(p_off + P_Lfo0_L2);
        write_mod(data, 0xC8 + d_off, get_parameter(p_off + P_Lfo0_MOD));
    }

    data[0x125] = get_parameter(P_Misc_FLTFC);
    data[0x126] = get_parameter(P_Misc_Q);
    data[0x127] = get_parameter(P_Misc_KEYBD);
    write_mod(data, 0x129, get_parameter(P_Misc_FCSRC1));
    data[0x12B] = get_parameter(P_Misc_FCMODAMT1);
    write_mod(data, 0x12C, get_parameter(P_Misc_FCSRC2));
    data[0x12E] = get_parameter(P_Misc_FCMODAMT1);
    data[0x151] = get_parameter(P_Misc_DCA4MODAMT);
    data[0x152] = get_parameter(P_Misc_PAN);
    write_mod(data, 0x156, get_parameter(P_Misc_PANMODSRC));
    data[0x158] = get_parameter(P_Misc_PANMODAMT);
    data[0x18F] = get_parameter(P_Misc_SYNC);
    data[0x190] = get_parameter(P_Misc_AM);
    data[0x191] = get_parameter(P_Misc_MONO);
    data[0x192] = get_parameter(P_Misc_GLIDE);
    data[0x193] = get_parameter(P_Misc_VC);
    data[0x194] = get_parameter(P_Misc_ENV);
    data[0x195] = get_parameter(P_Misc_OSC);
    data[0x196] = get_parameter(P_Misc_CYCLE);
}

}  // namespace cws80