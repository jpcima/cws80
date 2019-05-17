#pragma once
#include "cws80_program_parameters.h"
#include "utility/arithmetic.h"
#include "utility/types.h"
#include "utility/c++std/string_view.h"
#include <algorithm>
#include <string>
#include <utility>
#include <stdio.h>

namespace cws80 {

#pragma pack(push, 1)
struct Program {
    enum { data_length = 102 };

    char NAME[6];

    struct Env {
        struct {
            struct { i8 : 1; i8 L1 : 7; };
            struct { i8 : 1; i8 L2 : 7; };
            struct { i8 : 1; i8 L3 : 7; };
            struct { u8 T1 : 6; i8 : 2; };
            struct { u8 T2 : 6; i8 : 2; };
            struct { u8 T3 : 6; i8 : 2; };
            struct { u8 T4 : 6; i8 : 1; u1 R2 : 1; };
            struct { u1 LE : 1; i8 : 1; u8 LV : 6; };
            struct { u8 T1V : 6; i8 : 2; };
            struct { u8 TK : 6; i8 : 2; };
        };
    };

    struct Lfo {
        struct { u8 FREQ : 6; u8 WAV : 2; };
        struct { u8 L1 : 6; u8 MODlo : 2; };
        struct { u8 L2 : 6; u8 MODhi : 2; };
        struct { u8 DELAY : 6; u1 HUMAN : 1; u1 RESET : 1; };
        u8 MOD() const { return MODhi | (MODlo << 2); }
        void MOD(u8 x) { MODhi = x >> 2; MODlo = x; }
    };

    struct Osc {
        struct { u8 OCTSEMI : 7; i8 : 1; };
        struct { i8 : 3; u8 FINE : 5; };
        struct { u8 FMSRC1 : 4; u8 FMSRC2 : 4; };
        struct { i8 : 1; i8 FCMODAMT1 : 7; };
        struct { i8 : 1; i8 FCMODAMT2 : 7; };
        u8 WAVEFORM;
        struct { i8 : 1; u8 DCALEVEL : 6; u1 DCAENABLE : 1; };
        struct { u8 AMSRC1 : 4; u8 AMSRC2 : 4; };
        struct { i8 : 1; i8 AMAMT1 : 7; };
        struct { i8 : 1; i8 AMAMT2 : 7; };
        i8 OCT() const { return std::min(OCTSEMI / 12 - 3, 5); }
        u8 SEMI() const { return OCTSEMI % 12; }
        void OCT(i8 x) { OCTSEMI = clamp<i8>(x + 3, 0, 8) * 12 + SEMI(); }
        void SEMI(u8 x) { OCTSEMI = OCT() * 12 + std::min<u8>(x, 11); }
    };

    struct Misc {
        struct { i8 : 1; u8 DCA4MODAMT : 6; u1 AM : 1; };
        struct { u8 FLTFC : 7; u1 SYNC : 1; };
        struct { u8 Q : 5; i8 : 3; };
        struct { u8 FCSRC1 : 4; u8 FCSRC2 : 4; };
        struct { i8 FCMODAMT1 : 7; u1 VC /* aka ROTATE */ : 1; };
        struct { i8 FCMODAMT2 : 7; u1 MONO : 1; };
        struct { i8 : 1; u8 KEYBD /* aka FCMODAMT3 */ : 6; u1 ENV : 1; };
        struct { u8 GLIDE : 6; i8 : 1; u1 OSC : 1; };
        struct { u8 SPLITPOINT : 7; u1 SPLITDIR : 1; };
        struct { u8 LAYERPRG : 7; u1 LAYER : 1; };
        struct { u8 SPLITPRG : 7; u1 SPLIT : 1; };
        struct { u8 SPLITLAYERPRG : 7; u1 SPLITLAYER : 1; };
        struct { u8 PANMODSRC : 4; u8 PAN : 4; };
        struct { i8 PANMODAMT : 7; u1 CYCLE : 1; };
    };

    Env envs[Param::env_count];
    Lfo lfos[Param::lfo_count];
    Osc oscs[Param::osc_count];
    Misc misc;

    uint name_length() const;
    char *name(char namebuf[8]) const;
    bool rename(cxx::string_view name);
    static Program load(const u8 *data, size_t length);

    std::string to_string() const;
    static Program from_string(const std::string &string);

    i32 get_parameter(uint idx) const;
    bool set_parameter(uint idx, i32 val);
    static std::pair<i32, i32> get_parameter_range(uint idx);
    static const char *get_parameter_name(uint idx);
    static const char *get_parameter_short_name(uint idx);
    static i32 clamp_parameter_value(uint idx, i32 val);

    bool set_u7_parameter(uint idx, int val7);  // val in 0..127
    bool set_f32_parameter(uint idx, f32 valf);  // val in 0..1

    bool apply_nrpn(int nrpn, int val7);
};
#pragma pack(pop)

static_assert(sizeof(Program) == Program::data_length,
              "the program structure does not have the expected size");

struct Bank {
    enum {
        max_programs = 128,
        sysex_max_length = 6 + Bank::max_programs * 2 * Program::data_length,
    };

    Program pgm[max_programs];
    u8 pgm_count;

    static Bank load(const u8 *data, size_t length);
    static Bank load_sysex(const u8 *data, size_t length);
    static size_t save_sysex(u8 *data, const Bank &bank);
    static Bank read_sysex(FILE *file);
    static void write_sysex(FILE *file, const Bank &bank);
};

enum ParameterId {
#define PARAMETER_CASE(index, cat, name, gett, sett, min, max) \
    P_##cat##_##name = (index),
    EACH_PARAMETER(PARAMETER_CASE)
#undef PARAMETER_CASE
};

}  // namespace cws80
