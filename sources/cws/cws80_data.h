#pragma once
#include "utility/types.h"
#include <array>

namespace cws80 {

struct Program;
struct Bank;

struct Waveset {
    u8 wavenum[16];
};

struct Wave {
    u8 addr;
    u8 wsr;
    u8 semi;
    u8 fine;
};

struct WaveProperty {
    bool oneshot;
    char name[16];
};

struct Sample {
    const u8 *data;
    uint log2length;
    uint length() const { return 1u << log2length; }
};

enum class Mod : u8 {
    LFO1,
    LFO2,
    LFO3,
    ENV1,
    ENV2,
    ENV3,
    ENV4,
    VEL,
    VEL2,
    KYBD,
    KYBD2,
    WHEEL,
    PEDAL,
    XCTRL,
    PRESS,
    OFF,
};

enum class LfoWave : u8 {
    TRI,
    SAW,
    SQR,
    NOI,
};

typedef u8 WaveId;
typedef u8 WavesetId;

enum {
    waveset_count = 75,
    wave_count = 151,
};

const char *modulator_name(Mod id);
const char *lfo_wave_name(LfoWave id);

Waveset waveset_by_id(WavesetId id);
Waveset *waveset_by_name(const char *name, Waveset *buf);
char *waveset_name(WavesetId id, char buf[8]);

Wave wave_by_id(WaveId id);
Wave *wave_by_name(const char *name, Wave *buf);
uint wave_id_by_name(const char *name);
char *wave_name(WaveId id, char buf[16]);
bool wave_oneshot(WaveId id);
Sample wave_sample(const Wave &wave);

enum { factory_program_count = 40 };

const Program &factory_program(uint id);
Bank load_factory_bank();

const Program &initial_program();

extern const std::array<WaveProperty, 256> wave_property;

struct ROM {
    enum {
        prog_size = 64 * 1024,
        wave_size = 256 * 1024,
    };
    u8 wave[wave_size];
    u8 prog[prog_size];
};

extern const ROM rom_data;
extern const u8 init_prog_data[102];

}  // namespace cws80
