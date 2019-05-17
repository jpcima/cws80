#pragma once
#include "DistrhoPlugin.hpp"
#include "utility/types.h"
#include "cws/cws80_ins.h"
#include "ring_buffer.h"
#include <memory>

class SynthPlugin : public Plugin, cws80::FxMaster {
public:
    SynthPlugin();

    std::weak_ptr<Ring_Buffer> requests_in_;
    std::weak_ptr<Ring_Buffer> notifications_out_;

private:
    static constexpr u32 bufferFrames = 64;

protected:
    const char *getLabel() const override;
    const char *getMaker() const override;
    const char *getLicense() const override;
    u32 getVersion() const override;
    int64_t getUniqueId() const override;

    void initParameter(u32 index, Parameter &param) override;
    float getParameterValue(u32 index) const override;
    void setParameterValue(u32 index, float value) override;
    void run(const float **, float **outputs, u32 frames,
             const MidiEvent *midiEvents, u32 midiCount) override;

protected:
    // implement FxMaster
    bool emit_notification(const cws80::Notification::T &ntf) override;

private:
    cws80::Instrument ins_;

private:
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthPlugin)
};
