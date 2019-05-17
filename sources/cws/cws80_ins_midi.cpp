#include "cws80_ins.h"
#include "cws80_ins_util.h"

namespace cws80 {

void Instrument::receive_midi(const u8 *msg, uint len, uint ftime)
{
    u8 status = 0;
    u8 data1 = 0;
    u8 data2 = 0;

    switch (len) {
    case 3:
        data2 = msg[2];  // fall through
    case 2:
        data1 = msg[1];  // fall through
    case 1:
        status = msg[0];
        break;
    case 0:
        return;
    default:  // sysex message
        return receive_sysex(msg, len, ftime);
    }

    if ((status & 0xf0) == 0xf0)
        return;  // system message

    // channel message
    uint chan = status & 15;
    if (midichan_ < 16 && chan != midichan_)
        return;

    data1 &= 127;
    data2 &= 127;

    u8 event = status >> 4;

    // special case: note-on with zero velocity
    event = (event == 0b1001 && data2 == 0) ? 0b1000 : event;

    switch (event) {
    case 0b1000:
        handle_noteoff(data1, data2, ftime);
        break;
    case 0b1001:
        handle_noteon(data1, data2, ftime);
        break;
    case 0b1010:
        handle_aftertouch(data1, data2, ftime);
        break;
    case 0b1011:
        handle_cc(data1, data2, ftime);
        break;
    case 0b1100:
        handle_progchange(data1, ftime);
        break;
    case 0b1101:
        handle_aftertouch(data1, ftime);
        break;
    case 0b1110:
        handle_pitchbend(data1 | (data2 << 7), ftime);
        break;
    }
}

void Instrument::receive_sysex(const u8 *msg, uint len, uint ftime)
{
    (void)ftime;

    if (len < 2 || msg[0] != 0xf0 || msg[len - 1] != 0xf7)
        return;  // not sysex


    // TODO sysex
}

#define trace_midi(...)
// #define trace_midi(fmt, ...) do fprintf(stderr, "[midi] " fmt "\n", ##__VA_ARGS__); while (0)

void Instrument::handle_noteoff(uint key, uint vel, uint ftime)
{
    trace_midi("note-off key=%u vel=%u", key, vel);

    for (uint vnum : find_all_voices(key)) {
        Voice &vc = voices_[vnum];
        vc.release(vel, ftime);
    }
}

void Instrument::handle_noteon(uint key, uint vel, uint ftime)
{
    trace_midi("note-on key=%u vel=%u", key, vel);

    uint vnum = find_voice(key);
    if (vnum != ~0u) {
        Voice &vc = voices_[vnum];
        reorder_voice_first(vnum);
        vc.trigger(key, vel, ftime);
    }
    else if ((vnum = allocate_voice()) != ~0u) {
        Voice &vc = voices_[vnum];
        vcforeign_[vnum] = false;
        vc.program() = program_;
        vc.reset();
        vc.trigger(key, vel, ftime);
    }
}

void Instrument::handle_aftertouch(uint key, uint vel, uint ftime)
{
    trace_midi("aftertouch key=%u vel=%u", key, vel);

    if (ptype_ == PressureType::Key) {
        uint vnum = find_voice(key);
        if (vnum != ~0u)
            voices_[vnum].handle_aftertouch(vel, ftime);
    }
}

void Instrument::handle_cc(uint ctl, uint val, uint ftime)
{
    trace_midi("control-change key=%u vel=%u", ctl, val);

    if (ctl == xctrl_)
        mb_xctrl_->append(ftime, val / 2);

    switch (ctl) {
    case 1:  // modulation wheel
        mb_wheel_->append(ftime, val / 2);
        break;
    case 4:  // foot controller
        mb_pedal_->append(ftime, val / 2);
        break;

    case 6:  // Data entry
        if (nrpn_ < 128) {
            if (program_.apply_nrpn(nrpn_, val))
                should_notify_program_ = true;
        }
        else {
            uint rpn = nrpn_ - 128;
            (void)rpn; /* TODO: RPN? */
        }
        break;

    case 98:  // NRPN LSB
        nrpn_ = val;
        break;

    case 100:  // RPN LSB
        nrpn_ = 128 + val;
        break;

    // channel mode controls
    case 120:  // TODO all sound off

        break;
    case 121:  // TODO reset all controllers

        break;
    case 122:  // local control
        break;
    case 123:  // TODO all notes off

        break;

    default:
        break;
    }
}

void Instrument::handle_progchange(uint num, uint ftime)
{
    (void)ftime;

    trace_midi("program-change num=%u", num);
    select_program(banknum_, num);
}

void Instrument::handle_aftertouch(uint vel, uint ftime)
{
    trace_midi("aftertouch vel=%u", vel);
    if (ptype_ == PressureType::Channel) {
        for (uint vnum : vcorder_)
            voices_[vnum].handle_aftertouch(vel, ftime);
    }
}

void Instrument::handle_pitchbend(uint bend, uint ftime)
{
    (void)ftime;

    trace_midi("pitchbend bend=%d", bend);

#pragma message("TODO: pitch bend")
    (void)bend;
}

}  // namespace cws80
