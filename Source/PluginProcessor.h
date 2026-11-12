#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>

class CastleBassProcessor : public juce::AudioProcessor
{
public:
    CastleBassProcessor();
    ~CastleBassProcessor() override = default;

    void prepareToPlay (double sampleRate, int blockSize) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Castle Bass"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void render (juce::AudioBuffer<float>&, int start, int end);
    void handleMidi (const juce::MidiMessage&);
    float param (int i) const { return params[(size_t) i]->load(); }

    std::array<std::atomic<float>*, 20> params {};
    int currentProgram = 0;

    // voice state (monophonic, last-note priority)
    std::vector<int> noteStack;
    double sr = 44100.0;
    float targetFreq = 55.f, curFreq = 55.f;
    float ph1 = 0.f, ph2 = 0.f, phSub = 0.f, lfoPh = 0.f;
    float fEnv = 0.f, held = 0.f;
    int crushCount = 0;
    juce::ADSR ampEnv;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CastleBassProcessor)
};
