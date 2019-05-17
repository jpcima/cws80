#include "cws80_ins.h"
#include <stdio.h>

namespace cws80 {

#define trace_vcm(...)
// #define trace_vcm(fmt, ...) do fprintf(stderr, "[vcm] " fmt "\n", ##__VA_ARGS__); while (0)

uint Instrument::find_voice(uint key)
{
    for (uint vnum : vcorder_) {
        Voice &vc = voices_[vnum];
        bool allocd = vcallocd_[vnum];
        bool foreign = vcforeign_[vnum];
        if (vc.key() == key && allocd && !foreign) {
            trace_vcm("voice %u found for key %u", vnum, key);
            return vnum;
        }
    }
    trace_vcm("no voice found for key %u", key);
    return ~0u;
}

bounded_vector<uint, polymax> Instrument::find_all_voices(uint key)
{
    bounded_vector<uint, polymax> all;
    for (uint vnum : vcorder_) {
        Voice &vc = voices_[vnum];
        bool allocd = vcallocd_[vnum];
        if (vc.key() == key && allocd)
            all.push_back(vnum);
    }
    return all;
}

uint Instrument::allocate_voice()
{
    const Program &pgm = program_;
    uint vnum = ~0u;
    uint poly = pgm.misc.MONO ? 1 : poly_;
    uint count = 0;
    const bool steal = true;

    for (uint p = 0; p < polymax && vnum == ~0u && count < poly; ++p) {
        bool allocd = vcallocd_[p];
        // bool foreign = vcforeign_[p];
        count += allocd /* && !foreign */;
        vnum = allocd ? ~0u : p;
    }

    if (vnum != ~0u) {
        trace_vcm("allocate voice %u", vnum);
        vcallocd_[vnum] = true;
        vcorder_.insert(vcorder_.begin(), (u8)vnum);
    }
    else if (steal && !vcorder_.empty()) {
        vnum = vcorder_.back();
        trace_vcm("steal voice %u", vnum);
        vcorder_.pop_back();
        vcorder_.insert(vcorder_.begin(), (u8)vnum);
    }

    return vnum;
}

void Instrument::reorder_voice_first(uint vnum)
{
    bounded_vector<u8, polymax> orig = vcorder_;
    vcorder_.clear();
    vcorder_.push_back(vnum);
    for (uint other : orig) {
        if (vnum != other)
            vcorder_.push_back(other);
    }
}

void Instrument::shutdown_idle_voices()
{
    bounded_vector<u8, polymax> orig = vcorder_;
    vcorder_.clear();
    for (uint vnum : orig) {
        Voice &vc = voices_[vnum];
        if (!vc.finished()) {
            vcorder_.push_back(vnum);
        }
        else {
            trace_vcm("shutdown voice %u", vnum);
            vcallocd_[vnum] = false;
        }
    }
}

}  // namespace cws80
