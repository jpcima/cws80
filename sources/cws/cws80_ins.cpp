#include "cws/cws80_ins.h"
#include "cws/cws80_data.h"
#include "cws/cws80_data_banks.h"
#include "utility/arithmetic.h"
#include "utility/scope_guard.h"
#include <algorithm>
#include <limits>
#include <stdio.h>
#include <math.h>

namespace cws80 {

static uint Ins_vel2_table[127];
static bool Ins_table_init = false;

//------------------------------------------------------------------------------
Voice::~Voice()
{
}

void Voice::initialize(f64 fs, uint bs, pb_alloc<> &alloc, Voice *other)
{
    alloc_ = &alloc;
    bs_ = bs;

    for (uint i = 0; i < 16; ++i)
        mods_[i] = std::make_shared<mod_buffer>(bs);

    for (uint i = 0; i < 4; ++i) {
        Env &env = env_[i];
        Env *other_env = other ? &other->env_[0] : (i == 0) ? nullptr : &env_[0];
        env.initialize(fs, bs, other_env);
        env.setparam(&pgm_.envs[i]);
    }
    for (uint i = 0; i < 3; ++i) {
        Lfo &lfo = lfo_[i];
        Lfo *other_lfo = other ? &other->lfo_[0] : (i == 0) ? nullptr : &lfo_[0];
        lfo.initialize(fs, bs, other_lfo);
        lfo.setparam(&pgm_.lfos[i]);
    }
    for (uint i = 0; i < 3; ++i) {
        Osc &osc = osc_[i];
        Osc *other_osc = other ? &other->osc_[0] : (i == 0) ? nullptr : &osc_[0];
        osc.initialize(fs, bs, other_osc);
        osc.setparam(&pgm_.oscs[i]);
        Dca &dca = dca_[i];
        dca.initialize(fs, bs);
        dca.setparam(&pgm_.oscs[i]);
    }

    Vcf &vcf = vcf_;
    vcf.initialize(fs, bs);
    vcf.setparam(&pgm_.misc);

    Sat &sat = sat_;
    Sat *other_sat = other ? &other->sat_ : nullptr;
    sat.initialize(fs, bs, other_sat);

    Dca4 &dca4 = dca4_;
    dca4.initialize(fs, bs);
    dca4.setparam(&pgm_.misc);
}

void Voice::reset()
{
    mod(Mod::PRESS)->clear();

    const Program &pgm = pgm_;

    for (uint i = 0; i < 3; ++i)
        lfo_[i].reset();
    if (pgm.misc.ENV)
        for (uint i = 0; i < 4; ++i)
            env_[i].reset();
    if (pgm.misc.OSC)
        for (uint i = 0; i < 3; ++i)
            osc_[i].reset();
    for (uint i = 0; i < 3; ++i)
        dca_[i].reset();
    vcf_.reset();
    sat_.reset();
    dca4_.reset();
}

void Voice::synthesize_adding(i16 *outl, i16 *outr, uint nframes)
{
    pb_alloc<> &alloc = *alloc_;
    const Program &pgm = pgm_;
    const Program::Misc &miscpar = pgm.misc;
    uint key = key_;

    i16 *zeroin = (i16 *)alloc.unchecked_alloc(nframes * sizeof(i16));
    SCOPE(exit)
    {
        alloc.free(zeroin);
    };
    std::fill(zeroin, zeroin + nframes, 0);

    i16 *dummyout = (i16 *)alloc.unchecked_alloc(nframes * sizeof(i16));
    SCOPE(exit)
    {
        alloc.free(dummyout);
    };

    i16 *oscout[3] = {};
    i16 *dcaout[3] = {};

    for (uint i = 0; i < 3; ++i)
        oscout[i] = (i16 *)alloc.unchecked_alloc(nframes * sizeof(i16));
    SCOPE(exit)
    {
        for (uint i = 0; i < 3; ++i)
            alloc.free(oscout[2 - i]);
    };

    for (uint i = 0; i < 3; ++i)
        dcaout[i] = (i16 *)alloc.unchecked_alloc(nframes * sizeof(i16));
    SCOPE(exit)
    {
        for (uint i = 0; i < 3; ++i)
            alloc.free(dcaout[2 - i]);
    };

    i8 *syncout = (i8 *)alloc.unchecked_alloc(nframes);
    SCOPE(exit)
    {
        alloc.free(syncout);
    };

    // TODO synthesize
    for (uint i = 0; i < 3; ++i) {
        Osc &osc = osc_[i];
        Dca &dca = dca_[i];

        const Osc::Param &oscpar = pgm.oscs[i];

        const i8 *oscmods[2] = {mod((Mod)oscpar.FMSRC1)->for_input(nframes),
                                mod((Mod)oscpar.FMSRC2)->for_input(nframes)};
        const i8 oscmodamts[2] = {oscpar.FCMODAMT1, oscpar.FCMODAMT2};

        osc.generate(oscout[i], (i == 1 && miscpar.SYNC) ? syncout : (const i8 *)zeroin,
                     (i == 0) ? syncout : (i8 *)dummyout, oscmods, oscmodamts, key, nframes);

        const i8 *dcamods[2] = {mod((Mod)oscpar.AMSRC1)->for_input(nframes),
                                mod((Mod)oscpar.AMSRC2)->for_input(nframes)};
        const i8 dcamodamts[2] = {oscpar.AMAMT1, oscpar.AMAMT2};

        dca.generate(dcaout[i], oscout[i], (i == 1 && miscpar.AM) ? oscout[0] : zeroin,
                     dcamods, dcamodamts, nframes);
    }

    i32 *satin = (i32 *)alloc.unchecked_alloc(nframes * sizeof(i32));
    SCOPE(exit)
    {
        alloc.free(satin);
    };

    for (uint i = 0; i < nframes; ++i)
        satin[i] = (i32)dcaout[0][i] + (i32)dcaout[1][i] + (i32)dcaout[2][i];

    i16 *satout = (i16 *)alloc.unchecked_alloc(nframes * sizeof(i16));
    SCOPE(exit)
    {
        alloc.free(satout);
    };

    Sat &sat = sat_;
    sat.generate(satin, satout, nframes);

    i16 *vcfout = (i16 *)alloc.unchecked_alloc(nframes * sizeof(i16));
    SCOPE(exit)
    {
        alloc.free(vcfout);
    };

    const i8 *vcfmods[2] = {mod((Mod)miscpar.FCSRC1)->for_input(nframes),
                            mod((Mod)miscpar.FCSRC2)->for_input(nframes)};
    const i8 vcfmodamts[2] = {miscpar.FCMODAMT1, miscpar.FCMODAMT2};

    Vcf &vcf = vcf_;
    vcf.generate(vcfout, satout, vcfmods, vcfmodamts, key, nframes);

    Dca4 &dca4 = dca4_;
    const i8 *dca4mod = mod(Mod::ENV4)->for_input(nframes);
    const i8 *panmod = mod((Mod)miscpar.PANMODSRC)->for_input(nframes);
    dca4.generate_adding(outl, outr, vcfout, dca4mod, panmod, nframes);

    // prepare for the next new MIDI sequence
    mod(Mod::PRESS)->cycle();
}

void Voice::synthesize_mods(uint nframes)
{
    const Program &pgm = pgm_;

    mod(Mod::PRESS)->repeat_upto(nframes - 1);

    i8 kybd = key_ / 2;
    i8 kybd2 = clamp((((int)key_ - 36) * 126 / 60), 0, 126) - 63;
    i8 vel = vel_ / 2;
    i8 vel2 = Ins_vel2_table[vel_];
    mod(Mod::KYBD)->fill_entire(kybd, nframes);
    mod(Mod::KYBD2)->fill_entire(kybd2, nframes);
    mod(Mod::VEL)->fill_entire(vel, nframes);
    mod(Mod::VEL2)->fill_entire(vel2, nframes);

    for (uint i = 0; i < 4; ++i) {
        Env &env = env_[i];
        Mod dst = (Mod)((int)Mod::ENV1 + i);
        env.generate(mod(dst)->for_output(nframes), nframes);
    }

    //
    for (uint i = 0; i < 3; ++i) {
        Lfo &lfo = lfo_[i];
        const Lfo::Param &param = pgm.lfos[i];
        Mod dst = (Mod)((int)Mod::LFO1 + i);
        Mod src = (Mod)param.MOD();
        lfo.generate(mod(dst)->for_output(nframes), mod(src)->for_input(nframes), nframes);
    }
}

void Voice::trigger(uint key, uint vel, uint ftime)
{
    const Program &pgm = pgm_;

    key_ = key;
    vel_ = vel;
    handle_aftertouch(vel, ftime);

    for (uint i = 0; i < 3; ++i) {
        if (pgm.lfos[i].RESET)
            lfo_[i].reset();
    }

    for (uint i = 0; i < 4; ++i)
        env_[i].trigger(vel);
}

void Voice::release(uint vel, uint ftime)
{
    (void)ftime;

    for (uint i = 0; i < 4; ++i)
        env_[i].release(vel);
}

void Voice::handle_aftertouch(uint vel, uint ftime)
{
    mod(Mod::PRESS)->append(ftime, vel / 2);
}

//------------------------------------------------------------------------------
Instrument::Instrument(FxMaster &master)
{
    master_ = &master;

    if (!Ins_table_init) {
        initialize_tables();
        Ins_table_init = true;
    }

    load_default_banks();
    enable_program(selected_program());
}

void Instrument::initialize(f64 fs, uint bs)
{
    pb_alloc<> &alloc = alloc_;
    alloc = pb_alloc<>(allocatable_buffers * bs * sizeof(i32));

    mb_wheel_ = std::make_shared<mod_buffer>(bs);
    mb_pedal_ = std::make_shared<mod_buffer>(bs);
    mb_xctrl_ = std::make_shared<mod_buffer>(bs);

    for (uint p = 0; p < polymax; ++p) {
        Voice &vc = voices_[p];
        vc.initialize(fs, bs, alloc, (p == 0) ? nullptr : &voices_[0]);
        vc.mod(Mod::WHEEL) = mb_wheel_;
        vc.mod(Mod::PEDAL) = mb_pedal_;
        vc.mod(Mod::XCTRL) = mb_xctrl_;
    }


    fs_ = fs;
}

void Instrument::initialize_tables()
{
    scoped_fesetround(FE_TONEAREST);

    for (uint i = 0; i < 127; ++i)
        Ins_vel2_table[i] = lrint(63.0 * sin(acos((127.0 - i) / 127.0)));
}

void Instrument::select_program(uint banknum, uint prognum)
{
    if (banknum == banknum_ && prognum == prognum_)
        return;

    Bank &bank = banks_[banknum];
    enable_program(bank.pgm[prognum]);
    should_notify_program_ = true;
    bank.pgm_count = std::min<uint>(bank.pgm_count, prognum + 1);
    banknum_ = banknum;
    prognum_ = prognum;
}

void Instrument::enable_program(const Program &pgm)
{
    program_ = pgm;
    vcforeign_.set();
    should_notify_program_ = true;
}

i32 Instrument::get_parameter(uint idx) const
{
    return program_.get_parameter(idx);
}

void Instrument::set_parameter(uint idx, i32 val)
{
    if (program_.set_parameter(idx, val))
        should_notify_program_ = true;
}

void Instrument::set_f32_parameter(uint idx, f32 val)
{
    if (program_.set_f32_parameter(idx, val))
        should_notify_program_ = true;
}

void Instrument::rename_program(const char *name)
{
    if (program_.rename(name))
        should_notify_program_ = true;
}

char *Instrument::program_name(char namebuf[8]) const
{
    return program_.name(namebuf);
}

void Instrument::load_default_banks()
{
    banks_[0] = load_factory_bank();

    for (uint i = 0; i < 4; ++i)
        for (uint j = (i == 0) ? 40 : 0; j < 128; ++j)
            banks_[i].pgm[j].rename("------");

    uint bnum = 0;
    uint pnum = 41;
    for (uint i = 0; i < 11; ++i) {
        gsl::span<const u8> data = extra_bank_data[i];
        Bank bank = Bank::load_sysex(data.data(), data.size());
        assert(bank.pgm_count == 40);
        //
        banks_[bnum].pgm_count = pnum + 40;
        for (uint i = 0; i < 40; ++i)
            banks_[bnum].pgm[pnum + i] = bank.pgm[i];
        //
        pnum += 41;
        if (128 - pnum < 40) {
            pnum = 0;
            ++bnum;
        }
    }

    bank_notification_mask_ = (1u << 4) - 1;
}

void Instrument::load_bank(uint index, const Bank &bank)
{
    assert(index < 4);
    banks_[index] = bank;
    bank_notification_mask_ |= 1u << index;
}

void Instrument::select_xctrl(uint c)
{
    xctrl_ = c;
    mb_xctrl_->clear();
}

void Instrument::reset()
{
    for (uint vcnum : vcorder_) {
        voices_[vcnum].reset();
        vcallocd_[vcnum] = false;
    }
    vcorder_.clear();

    mb_wheel_->clear();
    mb_pedal_->clear();
    mb_xctrl_->clear();

    load_default_banks();
    select_program(0, 0);
}

void Instrument::synthesize(i16 *outl, i16 *outr, uint nframes)
{
    emit_notifications();

    //
    synthesize_mods(nframes);

    //
    std::fill(outl, outl + nframes, 0);
    std::fill(outr, outr + nframes, 0);

    for (uint vnum : vcorder_) {
        Voice &vc = voices_[vnum];
        vc.synthesize_adding(outl, outr, nframes);
    }

    // TODO synthesize


    //
    shutdown_idle_voices();

    // prepare for the next new MIDI sequence
    mb_wheel_->cycle();
    mb_pedal_->cycle();
    mb_xctrl_->cycle();

    // check temporary memory is released
    assert(alloc_.empty());
}

void Instrument::synthesize_mods(uint nframes)
{
    mb_wheel_->repeat_upto(nframes - 1);
    mb_pedal_->repeat_upto(nframes - 1);
    mb_xctrl_->repeat_upto(nframes - 1);

    for (uint vnum : vcorder_) {
        Voice &vc = voices_[vnum];
        vc.synthesize_mods(nframes);
    }
}

void Instrument::receive_request(const Request::T &req)
{
    switch (req.type) {
    case RequestType::SetProgram: {
        auto &setpgm = (const Request::SetProgram &)req;
        if (setpgm.prog >= 128)
            return;
        select_program(banknum_, setpgm.prog);
        break;
    }

    case RequestType::SetBank: {
        auto &setbank = (const Request::SetBank &)req;
        if (setbank.bank >= 4)
            return;
        select_program(setbank.bank, prognum_);
        break;
    }

    case RequestType::LoadBank: {
        auto &loadbank = (const Request::LoadBank &)req;
        const Bank &data = loadbank.data;
        uint count = data.pgm_count;
        if (count >= 128)
            return;
        uint banknum = banknum_;
        Bank &currbank = banks_[banknum];
        for (uint i = 0; i < count; ++i)
            currbank.pgm[i] = data.pgm[i];
        const Program &initpgm = initial_program();
        for (uint i = count; i < 128; ++i)
            currbank.pgm[i] = initpgm;
        bank_notification_mask_ |= 1u << banknum;
        enable_program(currbank.pgm[prognum_]);
        break;
    }

    case RequestType::RenameProgram: {
        auto &renamepgm = (const Request::RenameProgram &)req;
        std::copy_n(renamepgm.name, 6, program_.NAME);
        should_notify_program_ = true;
        break;
    }

    case RequestType::InitProgram: {
        enable_program(initial_program());
        break;
    }

    case RequestType::WriteProgram: {
        Bank &currbank = banks_[banknum_];
        currbank.pgm[prognum_] = program_;
        should_notify_write_ = true;
        break;
    }

    case RequestType::SetParameter: {
        auto &setpm = (const Request::SetParameter &)req;
        Program &pgm = program_;
        if (pgm.set_parameter(setpm.index, setpm.value))
            should_notify_program_ = true;
        break;
    }

    case RequestType::GetBankData: {
        auto &getbd = (const Request::GetBankData &)req;
        u32 num = getbd.bank;
        if (num >= 4)
            return;
        bank_notification_mask_ |= 1u << num;
        break;
    }

    case RequestType::NoteOn: {
        auto &note = (const Request::NoteOn &)req;
        handle_noteon(note.key, note.velocity, 0);
        break;
    }

    case RequestType::NoteOff: {
        auto &note = (const Request::NoteOff &)req;
        handle_noteoff(note.key, note.velocity, 0);
        break;
    }

    default:
        break;
    }
}

void Instrument::emit_notifications()
{
    FxMaster *master = master_;

    if (should_notify_write_) {
        Notification::Write ntf;
        if (master->emit_notification(ntf))
            should_notify_write_ = false;
    }

    bool bexpected;

    bexpected = true;
    if (should_notify_program_.compare_exchange_weak(bexpected, false)) {
        Notification::Program ntf;
        ntf.bank = banknum_;
        ntf.prog = prognum_;
        ntf.data = program_;
        if (!master->emit_notification(ntf))
            should_notify_program_.store(true);
    }

    if (bank_notification_mask_) {
        uint i = 0;
        while (i < 4 && !(bank_notification_mask_ & (1u << i)))
            ++i;
        if (i < 4) {
            Notification::Bank ntf;
            ntf.num = i;
            ntf.data = banks_[i];
            if (master->emit_notification(ntf))
                bank_notification_mask_ &= ~(1u << i);
        }
    }
}

}  // namespace cws80
