#pragma once
#include "cws/cws80_ins_util.h"
#include "cws/cws80_program.h"
#include "cws/cws80_data.h"
#include "plugin/plug_fx_master.h"
#include "cws/component/env.h"
#include "cws/component/lfo.h"
#include "cws/component/osc.h"
#include "cws/component/dca.h"
#include "cws/component/sat.h"
#include "cws/component/vcf.h"
#include "cws/component/dca4.h"
#include "utility/pb_alloc.h"
#include "utility/types.h"
#include "utility/container/bounded_vector.h"
#include <atomic>
#include <bitset>
#include <array>
#include <memory>

namespace cws80 {

enum { polymax = 16 };
typedef std::bitset<polymax> polybits;

typedef basic_mod_buffer<i8> mod_buffer;
typedef std::shared_ptr<mod_buffer> mod_buffer_ptr;

//------------------------------------------------------------------------------
class Voice {
public:
    ~Voice();
    void initialize(f64 fs, uint bs, pb_alloc<> &alloc, Voice *other);

    void reset();
    void synthesize_adding(i16 *outl, i16 *outr, uint nframes);
    void synthesize_mods(uint nframes);
    void trigger(uint key, uint vel, uint ftime);
    void release(uint vel, uint ftime);
    bool finished() const { return !env_[3].running(); }

    uint key() const { return key_; }

    mod_buffer_ptr &mod(Mod m) { return mods_[(int)m]; }

    Program &program() { return pgm_; }
    const Program &program() const { return pgm_; }

    void handle_aftertouch(uint vel, uint ftime);

private:
    // O(1) memory allocator
    pb_alloc<> *alloc_;
    // key played on this voice
    uint key_ = 0;
    // initial velocity of this note
    uint vel_ = 0;
    // buffer size
    uint bs_ = 0;
    // modulation output buffers
    mod_buffer_ptr mods_[16];
    // components
    Env env_[4];
    Lfo lfo_[3];
    Osc osc_[3];
    Dca dca_[3];
    Sat sat_;
    Vcf vcf_;
    Dca4 dca4_;
    // active program on this voice
    Program pgm_;
};

//------------------------------------------------------------------------------
class Instrument {
public:
    explicit Instrument(FxMaster &master);
    void initialize(f64 fs, uint bs);
    void load_default_banks();
    void load_bank(uint index, const Bank &bank);

    enum class PressureType : bool { Channel, Key };

    void select_midi_channel(uint c) { midichan_ = c; }
    void select_xctrl(uint c);
    void select_ptype(PressureType pt) { ptype_ = pt; }

    void reset();
    void synthesize(i16 *outl, i16 *outr, uint nframes);
    void synthesize_mods(uint nframes);

    void receive_request(const Request::T &req);

    void receive_midi(const u8 *msg, uint len, uint ftime);
    void receive_sysex(const u8 *msg, uint len, uint ftime);

    void select_program(uint banknum, uint prognum);
    void enable_program(const Program &pgm);

    // setparameter: can be invoked by UI thread {
    i32 get_parameter(uint idx) const;
    void set_parameter(uint idx, i32 val);
    void set_f32_parameter(uint idx, f32 val);  // val in 0..1
    // }

    // get/set name of the active program
    void rename_program(const char *name);
    char *program_name(char namebuf[8]) const;

    // find voice associated to key (in the current program)
    uint find_voice(uint key);
    // find voices associated to key (in any program)
    bounded_vector<uint, polymax> find_all_voices(uint key);
    // allocate a new voice
    uint allocate_voice();
    // make an allocated voice the first in order of recency
    void reorder_voice_first(uint vnum);

    // the active program (contains user edits)
    const Program &active_program() const { return program_; }
    const std::array<Bank, 4> &banks() const { return banks_; }

    // the selected program (original without edits)
    Program &selected_program() { return banks_[banknum_].pgm[prognum_]; }
    const Program &selected_program() const
    {
        return banks_[banknum_].pgm[prognum_];
    }

    uint bank_number() const { return banknum_; }
    uint program_number() const { return prognum_; }

private:
    // audio master interface
    FxMaster *master_ = nullptr;
    // O(1) memory allocator
    pb_alloc<> alloc_;
    // size of the memory area of the O(1) allocator (# of buffers)
    static constexpr size_t allocatable_buffers = 16;
    // sample rate
    f64 fs_ = 44100;
    // polyphony 1...polymax
    uint poly_ = 8;
    // MIDI channel 0..15, or >15 = all
    uint midichan_ = 0;
    // RPN or NRPN number (<128 NRPN, >=128 RPN)
    int nrpn_ = 0;
    // External controller, defaults to Breath Controller
    uint xctrl_ = 2;
    // Pressure type
    PressureType ptype_ = PressureType::Key;

    // output buffer of the wheel modulator
    mod_buffer_ptr mb_wheel_;
    // output buffer of the pedal modulator
    mod_buffer_ptr mb_pedal_;
    // output buffer of the xctrl modulator
    mod_buffer_ptr mb_xctrl_;

    // active program
    Program program_;
    // whether the bank should be transmitted to the host
    std::atomic<uint> bank_notification_mask_{(1 << 4) - 1};
    // whether the program should be transmitted to the host
    std::atomic<bool> should_notify_program_{true};
    // whether a successful write should be notified
    bool should_notify_write_ = false;

    // voices
    std::array<Voice, polymax> voices_;
    // active voices in order (most recent first)
    bounded_vector<u8, polymax> vcorder_;
    // whether a voice is currently allocated
    polybits vcallocd_;
    // whether a voice plays another program than the instrument
    polybits vcforeign_;

    // bank memory
    std::array<Bank, 4> banks_{};
    // bank number 0-3
    uint banknum_ = 0;
    // program number 0-127
    uint prognum_ = 0;

private:
    static void initialize_tables();

private:
    void emit_notifications();
    // MIDI message handling
    void handle_noteoff(uint key, uint vel, uint ftime);
    void handle_noteon(uint key, uint vel, uint ftime);
    void handle_aftertouch(uint key, uint vel, uint ftime);
    void handle_cc(uint ctl, uint val, uint ftime);
    void handle_progchange(uint num, uint ftime);
    void handle_aftertouch(uint vel, uint ftime);
    void handle_pitchbend(uint bend, uint ftime);
    // Voice management
    void shutdown_idle_voices();
};

}  // namespace cws80
