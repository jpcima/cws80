#include "cws80_data.h"
#include "cws80_program.h"
#include <stdio.h>
#include <string.h>

namespace cws80 {

const char *modulator_name(Mod id)
{
    switch (id) {
    case Mod::LFO1:
        return "LFO1";
    case Mod::LFO2:
        return "LFO2";
    case Mod::LFO3:
        return "LFO3";
    case Mod::ENV1:
        return "ENV1";
    case Mod::ENV2:
        return "ENV2";
    case Mod::ENV3:
        return "ENV3";
    case Mod::ENV4:
        return "ENV4";
    case Mod::VEL:
        return "VEL";
    case Mod::VEL2:
        return "VEL2";
    case Mod::KYBD:
        return "KYBD";
    case Mod::KYBD2:
        return "KYBD2";
    case Mod::WHEEL:
        return "WHEEL";
    case Mod::PEDAL:
        return "PEDAL";
    case Mod::XCTRL:
        return "XCTRL";
    case Mod::PRESS:
        return "PRESS";
    case Mod::OFF:
        return "OFF";
    default:
        return "";
    }
}

const char *lfo_wave_name(LfoWave id)
{
    switch (id) {
    case LfoWave::TRI:
        return "TRI";
    case LfoWave::SAW:
        return "SAW";
    case LfoWave::SQR:
        return "SQR";
    case LfoWave::NOI:
        return "NOI";
    default:
        return "";
    }
}

Waveset waveset_by_id(WavesetId id)
{
    const u8 *rom = rom_data.prog;
    uint addr = 0x1000 + 16 * id;

    Waveset waveset;
    memcpy(waveset.wavenum, &rom[addr], 16);

    return waveset;
}

Waveset *waveset_by_name(const char *name, Waveset *buf)
{
    bool found = false;
    char namebuf[8];
    for (uint i = 0; !found && i < waveset_count; ++i) {
        if (!strcmp(waveset_name(i, namebuf), name)) {
            found = true;
            *buf = waveset_by_id(i);
        }
    }
    return found ? buf : nullptr;
}

char *waveset_name(WavesetId id, char buf[8])
{
    const u8 *rom = rom_data.prog;
    uint addr = 0x63b9 + 6 * id;

    if (id >= waveset_count) {
        buf[0] = 0;
    }
    else {
        const u8 *strp = &rom[addr];
        uint i = 0;
        while (i < 6 && strp[i] == ' ')
            ++i;
        memcpy(buf, &rom[addr + i], 6 - i);
        buf[6 - i] = 0;
    }

    return buf;
}

Wave wave_by_id(WaveId id)
{
    const u8 *rom = rom_data.prog;
    uint addr = 0x14b0 + 4 * id;

    Wave wave;
    wave.addr = rom[addr];
    wave.wsr = rom[addr + 1];
    wave.semi = rom[addr + 2];
    wave.fine = rom[addr + 3];

    return wave;
}

Wave *wave_by_name(const char *name, Wave *buf)
{
    Wave *wav = nullptr;
    uint id = wave_id_by_name(name);
    if (id != ~0u) {
        *buf = wave_by_id((WaveId)id);
        wav = buf;
    }
    return wav;
}

uint wave_id_by_name(const char *name)
{
    uint id = ~0u;
    char namebuf[16];
    for (uint i = 0; id == ~0u && i < wave_count; ++i)
        if (!strcmp(wave_name(i, namebuf), name))
            id = i;
    return id;
}

char *wave_name(WaveId id, char buf[16])
{
    if (id >= wave_count) {
        sprintf(buf, "hidden.%u", id);
    }
    else {
        memcpy(buf, wave_property[id].name, 16);
    }

    return buf;
}

bool wave_oneshot(WaveId id)
{
    return wave_property[id].oneshot;
}

Sample wave_sample(const Wave &wave)
{
    Sample sample;
    uint rom = (wave.wsr & 0b11000000) >> 6;
    uint sizeindex = (wave.wsr & 0b111000) >> 3;
    uint offset = (rom << 16) | (wave.addr << 8);
    sample.data = rom_data.wave + offset;
    sample.log2length = 8 + sizeindex;
    return sample;
}

const Program &factory_program(uint id)
{
    const u8 *data = &rom_data.prog[0x3000 + id * Program::data_length];
    return *reinterpret_cast<const Program *>(data);
}

Bank load_factory_bank()
{
    return Bank::load(&rom_data.prog[0x3000], factory_program_count * Program::data_length);
}

const Program &initial_program()
{
    return *reinterpret_cast<const Program *>(init_prog_data);
}

const ROM rom_data = {{
#include "sqdata/wave2202.dat.h"
#include "sqdata/wave2203.dat.h"
#include "sqdata/wave2204.dat.h"
#include "sqdata/wave2205.dat.h"
                      },
                      {
#include "sqdata/sq80lorom.dat.h"
#include "sqdata/sq80hirom.dat.h"
                      }};

extern const u8 init_prog_data[102] = {
#include "sqdata/initprogram.dat.h"
};

const std::array<WaveProperty, 256> wave_property{{
    {false, "noise1"},     {false, "noise3"},         {false, "pulse.1"},
    {false, "pulse.2"},    {false, "pulse.3"},        {false, "pulse.4"},
    {false, "pulse.5"},    {false, "pulse.6"},        {false, "pulse.7"},
    {false, "bell.1"},     {false, "bell.2"},         {false, "bell.3"},
    {false, "bell.4"},     {false, "organ.1"},        {false, "organ.2"},
    {false, "synth1.1"},   {false, "synth1.2"},       {false, "synth1.3"},
    {false, "synth2.1"},   {false, "synth2.2"},       {false, "synth2.3"},
    {false, "sine"},       {false, "agg-sine"},       {false, "octave"},
    {false, "saw2"},       {false, "triang"},         {false, "oct+5"},
    {false, "synth3.1"},   {false, "prime-synth3.2"}, {false, "reed.1"},
    {false, "reed.2"},     {false, "kick"},           {false, "noise2"},
    {false, "pianolow.1"}, {false, "pianolow.2"},     {false, "pianolow.3"},
    {false, "bell.3.1"},   {false, "voice.1"},        {false, "voice.3"},
    {false, "bass.1"},     {false, "bass.2"},         {false, "bass.3"},
    {false, "bass.4"},     {false, "formant.1"},      {false, "formant.2"},
    {false, "formant.3"},  {false, "formant.4"},      {false, "formant.5"},
    {false, "formant.6"},  {false, "formant.7"},      {false, "formant.8"},
    {false, "formant.9"},  {false, "saw.1"},          {false, "saw.2"},
    {false, "saw.3"},      {false, "saw.4"},          {false, "saw.5"},
    {false, "saw.6"},      {false, "voice.2"},        {false, "voice.4"},
    {false, "voice.5"},    {false, "voice.6"},        {false, "voice.7"},
    {false, "el-pno.1"},   {false, "el-pno.2"},       {false, "el-pno.3"},
    {false, "pianohi.1"},  {false, "pianohi.2"},      {false, "pianohi.3"},
    {false, "pianohi.4"},  {false, "square.1"},       {false, "square.2"},
    {false, "square.3"},   {false, "square.4"},       {false, "square.5"},
    {false, "square.6"},   {false, "reed.3"},         {false, "reed.4"},
    {false, "reed.5"},     {false, "reed.6"},         {false, "reed.7"},
    {false, "reed.8"},     {true, "bowing.1"},        {true, "plunk"},
    {true, "plick"},       {true, "bowing.2"},        {true, "pick2.1"},
    {true, "plink"},       {true, "pick2.2"},         {true, "chiff.2"},
    {true, "chiff.1"},     {true, "pick1.1"},         {true, "pick1.2"},
    {true, "slap"},        {true, "mallet"},          {true, "tomtom"},
    {true, "tomtom.ds"},   {true, "snare"},           {true, "snare.ds"},
    {false, "metal"},      {false, "steam"},          {false, "voice3.1"},
    {false, "voice3.2"},   {false, "breath"},         {false, "chime.1"},
    {false, "chime.2"},    {true, "kick.2"},          {true, "logdrm"},
    {true, "logdrm.ds"},   {false, "grit.1"},         {false, "grit.2"},
    {false, "grit.3"},     {false, "grit.4"},         {false, "grit.5"},
    {false, "grit.6"},     {false, "grit.7"},         {false, "grit.8"},
    {true, "hi"},          {false, "brass.1"},        {false, "bell2.2"},
    {false, "bell2.1"},    {true, "click"},           {true, "thump"},
    {false, "brass.2"},    {false, "brass.3"},        {false, "string.2"},
    {false, "string.3"},   {false, "string.4"},       {false, "string.1"},
    {false, "alien.1"},    {false, "alien.2"},        {false, "alien.3"},
    {false, "alien.4"},    {false, "digit1.1"},       {false, "digit1.2"},
    {false, "digit2.1"},   {false, "digit2.2"},       {false, "clav.1"},
    {false, "clav.2"},     {false, "clav.3"},         {false, "clav.4"},
    {false, "glint.1"},    {false, "glint.2"},        {false, "glint.3"},
    {false, "glint.4"},    {false, "glint.5"},        {false, "glint.6"},
    {false, "glint.7"},    {false, "glint.8"},        {false, "glint.9"},
    {false, "glint.10"},
}};

}  // namespace cws80
