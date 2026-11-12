#include "PluginProcessor.h"

namespace
{
enum { Wave, Detune, Osc2, Sub, Noise, Cutoff, Reso, EnvAmt, FDecay,
       Atk, Dec, Sus, Rel, Glide, Bits, Rate, Drive, LfoRate, LfoDepth, Volume, NumParams };

struct PDef { const char* id; const char* name; float lo, hi, def, step, skew; };

const PDef defs[NumParams] = {
    { "wave",     "Wave (0 Saw,1 Sqr,2 Pulse,3 Tri)", 0, 3, 1, 1, 1 },
    { "detune",   "Osc2 Detune (cents)", 0, 50, 10, 0, 1 },
    { "osc2",     "Osc2 Level",   0, 1, 0.5f, 0, 1 },
    { "sub",      "Sub Level",    0, 1, 0.6f, 0, 1 },
    { "noise",    "Noise",        0, 1, 0.0f, 0, 1 },
    { "cutoff",   "Cutoff (Hz)",  40, 12000, 1500, 0, 0.3f },
    { "reso",     "Resonance",    0.5f, 10, 1, 0, 0.5f },
    { "envamt",   "Filter Env Amount", 0, 1, 0.4f, 0, 1 },
    { "fdecay",   "Filter Env Decay",  0.02f, 2, 0.25f, 0, 0.4f },
    { "atk",      "Attack",       0.001f, 1, 0.001f, 0, 0.4f },
    { "dec",      "Decay",        0.02f, 2, 0.3f, 0, 0.4f },
    { "sus",      "Sustain",      0, 1, 0.8f, 0, 1 },
    { "rel",      "Release",      0.01f, 2, 0.1f, 0, 0.4f },
    { "glide",    "Glide (ms)",   0, 300, 0, 0, 1 },
    { "bits",     "Bit Depth",    2, 16, 8, 1, 1 },
    { "rate",     "Downsample",   1, 32, 2, 1, 1 },
    { "drive",    "Drive",        0, 1, 0.3f, 0, 1 },
    { "lforate",  "LFO Rate (Hz)", 0.1f, 20, 4, 0, 0.4f },
    { "lfodepth", "LFO > Cutoff", 0, 1, 0, 0, 1 },
    { "vol",      "Volume",       0, 1, 0.7f, 0, 1 },
};

using Vals = std::array<float, NumParams>;
struct Preset { const char* name; Vals v; };

//            wave det  osc2 sub  noise cutoff reso env  fdec  atk    dec  sus  rel   glide bits rate drive lfoR lfoD vol
const Preset presets[] = {
 { "Atari Sub",          {1,  0,  0.0f,0.8f,0.00f, 900, 1.0f,0.4f,0.20f,0.001f,0.3f,0.9f,0.08f,  0,  8,  2,0.2f, 4.0f,0.0f,0.70f} },
 { "Crushed Saw",        {0, 12,  0.8f,0.5f,0.00f,2500, 2.0f,0.6f,0.25f,0.001f,0.4f,0.8f,0.10f,  0,  6,  3,0.5f, 4.0f,0.0f,0.65f} },
 { "Glitch Wobble",      {0,  8,  0.7f,0.6f,0.05f,1500, 4.0f,0.2f,0.30f,0.001f,0.5f,0.9f,0.10f, 20,  7,  4,0.6f, 5.5f,0.8f,0.60f} },
 { "Pulse Grit",         {2,  7,  0.6f,0.4f,0.10f,1800, 1.5f,0.5f,0.15f,0.001f,0.25f,0.7f,0.06f, 0,  5,  6,0.4f, 4.0f,0.1f,0.65f} },
 { "Noise Sub",          {1,  0,  0.3f,1.0f,0.25f, 600, 1.0f,0.3f,0.30f,0.002f,0.4f,0.9f,0.15f,  0,  4,  8,0.7f, 2.0f,0.0f,0.60f} },
 { "Dying Arcade",       {3, 20,  0.6f,0.6f,0.15f,3000, 3.0f,0.8f,0.50f,0.001f,0.8f,0.6f,0.30f,120,  4, 12,0.6f, 0.5f,0.3f,0.60f} },
 { "Lo-Fi Square",       {1,  5,  0.5f,0.3f,0.00f,1200, 1.2f,0.3f,0.20f,0.001f,0.3f,0.8f,0.10f,  0, 10,  2,0.3f, 4.0f,0.0f,0.70f} },
 { "Distorted Sub Pulse",{2,  0,  0.0f,1.0f,0.00f, 500, 0.8f,0.2f,0.25f,0.001f,0.3f,1.0f,0.10f, 30, 12,  1,0.9f, 4.0f,0.0f,0.55f} },
 { "Memo Boy",           {0, 15,  0.7f,0.4f,0.05f,2200, 2.5f,0.5f,0.18f,0.001f,0.35f,0.7f,0.12f, 15,  5,  5,0.5f, 6.0f,0.2f,0.60f} },
 { "Insomniac",          {2, 25,  0.8f,0.5f,0.20f,3500, 3.5f,0.7f,0.60f,0.005f,1.00f,0.8f,0.40f, 60,  4, 10,0.7f, 0.8f,0.5f,0.55f} },
 { "Sub Kick Bass",      {3,  0,  0.0f,1.0f,0.00f, 300, 1.0f,0.9f,0.12f,0.001f,0.25f,0.0f,0.10f,  0, 12,  1,0.4f, 4.0f,0.0f,0.80f} },
 { "Acid Chip",          {0,  5,  0.3f,0.3f,0.00f, 700, 8.0f,0.9f,0.15f,0.001f,0.20f,0.6f,0.06f, 40,  8,  2,0.4f, 4.0f,0.0f,0.60f} },
 { "Rave Stab Bass",     {0, 30,  1.0f,0.4f,0.10f,4000, 1.5f,0.3f,0.30f,0.001f,0.30f,0.5f,0.20f,  0,  6,  3,0.6f, 4.0f,0.0f,0.55f} },
 { "Broken Gameboy",     {2,  0,  0.5f,0.3f,0.30f,5000, 1.0f,0.1f,0.20f,0.001f,0.15f,0.8f,0.05f,  0,  3, 16,0.3f, 7.0f,0.4f,0.60f} },
};
constexpr int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));

inline float osc (float p, int wave)
{
    switch (wave)
    {
        case 0:  return 2.f * p - 1.f;                       // saw
        case 1:  return p < 0.5f  ? 1.f : -1.f;              // square
        case 2:  return p < 0.25f ? 1.f : -1.f;              // 25% pulse
        default: return 2.f * std::abs (2.f * p - 1.f) - 1.f; // triangle
    }
}
} // namespace

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout CastleBassProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& d : defs)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { d.id, 1 }, d.name,
            juce::NormalisableRange<float> (d.lo, d.hi, d.step, d.skew), d.def));
    return layout;
}

CastleBassProcessor::CastleBassProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < NumParams; ++i)
        params[(size_t) i] = apvts.getRawParameterValue (defs[i].id);
    noteStack.reserve (64);
}

void CastleBassProcessor::prepareToPlay (double sampleRate, int blockSize)
{
    sr = sampleRate;
    ampEnv.setSampleRate (sr);
    ampEnv.reset();
    filter.prepare ({ sr, (juce::uint32) blockSize, 1 });
    filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.reset();
    noteStack.clear();
}

void CastleBassProcessor::handleMidi (const juce::MidiMessage& m)
{
    auto removeNote = [this] (int n)
    {
        noteStack.erase (std::remove (noteStack.begin(), noteStack.end(), n), noteStack.end());
    };

    if (m.isNoteOn())
    {
        removeNote (m.getNoteNumber());
        noteStack.push_back (m.getNoteNumber());
        targetFreq = (float) juce::MidiMessage::getMidiNoteInHertz (m.getNoteNumber());
        if (noteStack.size() == 1) curFreq = targetFreq;
        ampEnv.noteOn();
        fEnv = 1.f;
    }
    else if (m.isNoteOff())
    {
        removeNote (m.getNoteNumber());
        if (noteStack.empty())
            ampEnv.noteOff();
        else
            targetFreq = (float) juce::MidiMessage::getMidiNoteInHertz (noteStack.back());
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        noteStack.clear();
        ampEnv.reset();
    }
}

void CastleBassProcessor::processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buf.clear();
    ampEnv.setParameters ({ param (Atk), param (Dec), param (Sus), param (Rel) });

    int pos = 0;
    for (const auto meta : midi)
    {
        render (buf, pos, meta.samplePosition);
        pos = meta.samplePosition;
        handleMidi (meta.getMessage());
    }
    render (buf, pos, buf.getNumSamples());
}

void CastleBassProcessor::render (juce::AudioBuffer<float>& buf, int start, int end)
{
    if (end <= start) return;

    const int   wave      = (int) std::round (param (Wave));
    const float detRatio  = std::pow (2.f, param (Detune) / 1200.f);
    const float osc2Lvl   = param (Osc2), subLvl = param (Sub), noiseLvl = param (Noise);
    const float baseCut   = param (Cutoff), envAmt = param (EnvAmt);
    const float fCoef     = std::exp (-1.f / (param (FDecay) * (float) sr));
    const float glideMs   = param (Glide);
    const float glideCoef = glideMs < 0.5f ? 1.f : 1.f - std::exp (-1.f / (glideMs * 0.001f * (float) sr));
    const float step      = 2.f / std::pow (2.f, param (Bits));
    const int   rateN     = (int) param (Rate);
    const float driveGain = 1.f + param (Drive) * 14.f;
    const float driveNorm = 1.f / std::tanh (driveGain);
    const float lfoInc    = param (LfoRate) / (float) sr, lfoDepth = param (LfoDepth);
    const float vol       = param (Volume);
    const float maxCut    = 0.45f * (float) sr;

    filter.setResonance (param (Reso));
    auto* l = buf.getWritePointer (0);
    auto* r = buf.getWritePointer (1);

    for (int i = start; i < end; ++i)
    {
        curFreq += (targetFreq - curFreq) * glideCoef;
        const float inc = curFreq / (float) sr;

        // naive (non-bandlimited) oscillators on purpose: the aliasing is the grit
        float s = osc (ph1, wave)
                + osc2Lvl * osc (ph2, wave)
                + subLvl  * osc (phSub, 1)
                + noiseLvl * (rng.nextFloat() * 2.f - 1.f);
        s *= 0.4f;

        ph1 += inc;             if (ph1 >= 1.f)   ph1 -= 1.f;
        ph2 += inc * detRatio;  if (ph2 >= 1.f)   ph2 -= 1.f;
        phSub += inc * 0.5f;    if (phSub >= 1.f) phSub -= 1.f;

        lfoPh += lfoInc;        if (lfoPh >= 1.f) lfoPh -= 1.f;
        const float lfo = std::sin (juce::MathConstants<float>::twoPi * lfoPh);

        float cut = baseCut * std::pow (2.f, fEnv * envAmt * 5.f + lfo * lfoDepth * 2.f);
        filter.setCutoffFrequency (juce::jlimit (20.f, maxCut, cut));
        s = filter.processSample (0, s);

        s = std::tanh (s * driveGain) * driveNorm;

        // bitcrusher + sample-rate reducer
        if (++crushCount >= rateN)
        {
            crushCount = 0;
            held = step * std::floor (s / step + 0.5f);
        }

        s = held * ampEnv.getNextSample() * vol;
        fEnv *= fCoef;

        l[i] = r[i] = s;
    }
}

//==============================================================================
int CastleBassProcessor::getNumPrograms() { return numPresets; }

const juce::String CastleBassProcessor::getProgramName (int i)
{
    return juce::isPositiveAndBelow (i, numPresets) ? presets[i].name : "";
}

void CastleBassProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, numPresets)) return;
    currentProgram = index;
    for (int i = 0; i < NumParams; ++i)
        if (auto* p = apvts.getParameter (defs[i].id))
            p->setValueNotifyingHost (p->convertTo0to1 (presets[index].v[(size_t) i]));
}

void CastleBassProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void CastleBassProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
class CastleBassEditor : public juce::AudioProcessorEditor
{
public:
    explicit CastleBassEditor (CastleBassProcessor& p)
        : AudioProcessorEditor (&p), proc (p), generic (p)
    {
        for (int i = 0; i < p.getNumPrograms(); ++i)
            presetBox.addItem (p.getProgramName (i), i + 1);
        presetBox.setSelectedId (p.getCurrentProgram() + 1, juce::dontSendNotification);
        presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
        addAndMakeVisible (presetBox);
        addAndMakeVisible (generic);
        setSize (480, 680);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        presetBox.setBounds (r.removeFromTop (34).reduced (4));
        generic.setBounds (r);
    }

private:
    CastleBassProcessor& proc;
    juce::ComboBox presetBox;
    juce::GenericAudioProcessorEditor generic;
};

juce::AudioProcessorEditor* CastleBassProcessor::createEditor() { return new CastleBassEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CastleBassProcessor(); }
