#pragma once

//==============================================================================
// Groove Sketchbook — AudioProcessor (APVTS + DSP chain + factory presets).
//==============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "BassDSP.h"
#include "Presets.h"

class GrooveSketchbookProcessor : public juce::AudioProcessor
{
public:
    GrooveSketchbookProcessor();
    ~GrooveSketchbookProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Groove Sketchbook"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Presets + UI helpers (call from the message thread)
    void loadPreset (int index);
    int getCurrentPreset() const { return currentPreset.load(); }
    int getNumFactoryPresets() const { return (int) factoryPresets.size(); }
    const FactoryPreset& getFactoryPreset (int i) const { return factoryPresets[(size_t) i]; }

    float getOutputLevel() const { return chain.peakLevel.load(); }
    int getBlockCounter() const { return blockCounter.load(); }
    float getParam01 (const char* id) const
        { return apvts.getRawParameterValue (juce::String (id))->load(); }
    void setChoiceParam (const char* id, int index, int numChoices);

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    BassChain chain;
    std::vector<FactoryPreset> factoryPresets;
    std::atomic<int> currentPreset { 0 };
    std::atomic<int> blockCounter { 0 };
    bool prepared = false;
    double currentSR = 48000.0;
    int currentBlock = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrooveSketchbookProcessor)
};
