#include "plugin.h"
#include "utility/arithmetic.h"
#include "utility/debug.h"

SynthPlugin::SynthPlugin()
    : Plugin(cws80::Param::num_params, 0, 0),  // parameters, programs, states
      ins_(*this)
{
    cws80::Instrument &ins = ins_;
    ins.initialize(getSampleRate(), bufferFrames);
}

const char *SynthPlugin::getLabel() const
{
    return CWS80_LABEL;
}

const char *SynthPlugin::getMaker() const
{
    return CWS80_AUTHOR;
}

const char *SynthPlugin::getLicense() const
{
    return CWS80_LICENSE;
}

u32 SynthPlugin::getVersion() const
{
    return CWS80_VERSION;
}

int64_t SynthPlugin::getUniqueId() const
{
    return CWS80_UNIQUE_ID;
}

void SynthPlugin::initParameter(u32 index, Parameter &param)
{
    const cws80::Program &initpgm = cws80::initial_program();

#define PARAMETER_CASE(index, cat, nam, gett, sett, vmin, vmax)     \
    case index:                                                     \
        param.hints = kParameterIsAutomable|kParameterIsInteger;    \
        param.name = #cat " " #nam;                                 \
        param.symbol = #cat "_" #nam;                               \
        param.ranges.def = initpgm.get_parameter(index);            \
        param.ranges.min = vmin;                                    \
        param.ranges.max = vmax;                                    \
        break;

    switch (index) {
        EACH_PARAMETER(PARAMETER_CASE)
    default:
            assert(false);
    }

#undef PARAMETER_CASE
}

float SynthPlugin::getParameterValue(u32 index) const
{
    const cws80::Instrument &ins = ins_;
    return ins.get_parameter(index);
}

void SynthPlugin::setParameterValue(u32 index, float value)
{
    cws80::Instrument &ins = ins_;
    ins.set_parameter(index, value);
}

void SynthPlugin::run(const float **, float **outputs, u32 frames,
                      const MidiEvent *midiEvents, u32 midiCount)
{
    cws80::Instrument &ins = ins_;
    std::shared_ptr<Ring_Buffer> requests_in = requests_in_.lock();

    cws80::Request::T hdr;
    if (requests_in && requests_in->peek(hdr)) {
        cws80::RequestTraits tr(hdr.type);
        size_t size = tr.size();
        alignas(cws80::Request::T) u8 data[tr.max_size()];
        if (requests_in->get(data, size))
            ins.receive_request(*reinterpret_cast<cws80::Request::T *>(data));
    }

    float *outL = outputs[0];
    float *outR = outputs[1];

    i16 bufL[bufferFrames];
    i16 bufR[bufferFrames];

    u32 frameIndex = 0;
    u32 midiIndex = 0;

    const float outputGain = 4.0;

    while (frameIndex < frames) {
        u32 frameCount = frames - frameIndex;
        frameCount = (frameCount < bufferFrames) ? frameCount : bufferFrames;

        while (midiIndex < midiCount) {
            const MidiEvent &ev = midiEvents[midiIndex];
            u32 ftime = ev.frame;
            if (ftime >= frameIndex + frameCount)
                break;
            u32 size = ev.size;
            const u8 *data = (ev.size <= ev.kDataSize) ? ev.data : ev.dataExt;
            ins.receive_midi(data, size, ftime - frameIndex);
            ++midiIndex;
        }

        ins.synthesize(bufL, bufR, frameCount);
        for (u32 i = 0; i < frameCount; ++i) {
            outL[frameIndex + i] = clamp(bufL[i] * (outputGain / 32768), -1.0f, +1.0f);
            outR[frameIndex + i] = clamp(bufR[i] * (outputGain / 32768), -1.0f, +1.0f);
        }

        frameIndex += frameCount;
    }

    while (midiIndex < midiCount) {
        const MidiEvent &ev = midiEvents[midiIndex++];
        u32 size = ev.size;
        const u8 *data = (ev.size <= ev.kDataSize) ? ev.data : ev.dataExt;
        ins.receive_midi(data, size, 0);
    }
}

// implement FxMaster
bool SynthPlugin::emit_notification(const cws80::Notification::T &ntf)
{
    std::shared_ptr<Ring_Buffer> notifications_out = notifications_out_.lock();
    if (!notifications_out)
        return false;

    cws80::NotificationTraits tr(ntf.type);
    size_t size = tr.size();
    const u8 *data = reinterpret_cast<const u8 *>(&ntf);

    //debug("Send notification {}", size);

    if (!notifications_out->put(data, size))
        return false;

    return true;
}

START_NAMESPACE_DISTRHO

Plugin *createPlugin()
{
    SynthPlugin *plugin = new SynthPlugin;
    debug("New synth with rate {} and buffer {}", plugin->getSampleRate(), plugin->getBufferSize());
    return plugin;
}

END_NAMESPACE_DISTRHO
