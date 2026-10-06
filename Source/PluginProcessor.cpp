#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "CrashLog.h"

#include <cmath>

GrooveSketchbookProcessor::GrooveSketchbookProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "GrooveSketchbook", createParameterLayout()),
      factoryPresets (getFactoryPresets())
{
    CrashLog::write ("processor constructed");
}

GrooveSketchbookProcessor::~GrooveSketchbookProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout
GrooveSketchbookProcessor::createParameterLayout()
{
    using namespace ParamIDs;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addFloat = [&] (const char* id, const char* name, float def)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id, 1 }, juce::String (name),
            juce::NormalisableRange<float> (0.f, 1.f), def));
    };
    auto addBool = [&] (const char* id, const char* name, bool def)
    {
        params.push_back (std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { id, 1 }, juce::String (name), def));
    };
    auto addChoice = [&] (const char* id, const char* name,
                          std::initializer_list<const char*> items, int def)
    {
        juce::StringArray arr;
        for (auto* s : items)
            arr.add (s);
        params.push_back (std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { id, 1 }, juce::String (name), arr, def));
    };

    // Defaults = Flea preset (page 1), so the plugin opens on a great sound.
    addBool (compOn, "Comp On", true);   addBool (octOn, "Oct On", false);
    addBool (driveOn, "Drive On", true); addBool (filterOn, "Filter On", false);
    addBool (chorusOn, "Chorus On", true); addBool (delayOn, "Delay On", false);

    addFloat (compSus, "Comp Sustain", .55f); addFloat (compAtk, "Comp Attack", .40f);
    addFloat (compLvl, "Comp Level", .65f);
    addFloat (octSub, "Oct Sub", .50f); addFloat (octUp, "Oct Up", 0.f);
    addFloat (octDry, "Oct Dry", .70f);
    addFloat (driveGain, "Drive Gain", .30f); addFloat (driveTone, "Drive Tone", .60f);
    addFloat (driveLvl, "Drive Level", .70f);
    addFloat (filtSens, "Filter Sens", .50f); addFloat (filtQ, "Filter Q", .50f);
    addFloat (filtMix, "Filter Mix", .50f);
    addFloat (choRate, "Chorus Rate", .35f); addFloat (choDepth, "Chorus Depth", .30f);
    addFloat (choMix, "Chorus Mix", .25f);
    addFloat (dlyTime, "Delay Time", .30f); addFloat (dlyFb, "Delay Feedback", .20f);
    addFloat (dlyMix, "Delay Mix", .15f);

    addFloat (ampGain, "Amp Gain", .40f); addFloat (ampBass, "Amp Bass", .70f);
    addFloat (ampMid, "Amp Mid", .55f);   addFloat (ampTreble, "Amp Treble", .65f);
    addFloat (ampMaster, "Amp Master", .80f);
    addChoice (ampCircuit, "Amp Circuit", { "Tube", "Solid" }, 1);
    addChoice (ampPower, "Amp Power", { "800W", "300W" }, 0);

    addChoice (cabType, "Cabinet", { "8x10", "4x10", "1x15" }, 1);
    addChoice (micType, "Microphone", { "D112", "SM57", "U47" }, 0);
    addFloat (micDist, "Mic Distance", .12f); addFloat (micAngle, "Mic Angle", .50f);
    addFloat (roomAmt, "Room", .22f);

    addFloat (inGain, "Input", .50f); addFloat (diMix, "DI Mix", 1.f);
    addFloat (outGain, "Output", .778f);

    return { params.begin(), params.end() };
}

void GrooveSketchbookProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSR = sampleRate > 0.0 ? sampleRate : 48000.0;
    currentBlock = samplesPerBlock > 0 ? samplesPerBlock : 512;
    juce::dsp::ProcessSpec spec { currentSR,
                                  (juce::uint32) currentBlock,
                                  (juce::uint32) 2 };
    chain.prepare (spec);
    setLatencySamples (chain.getLatencySamples());
    prepared = true;
    CrashLog::write ("prepareToPlay sr=" + juce::String (currentSR, 0) + " block=" + juce::String (currentBlock));
}

void GrooveSketchbookProcessor::releaseResources() {}

bool GrooveSketchbookProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    const auto in = layouts.getMainInputChannelSet();
    return in == juce::AudioChannelSet::mono()
        || in == juce::AudioChannelSet::stereo();
}

void GrooveSketchbookProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Some hosts may call processBlock before prepareToPlay - be safe.
    if (! prepared)
        prepareToPlay (currentSR, currentBlock);

    if (buffer.getNumSamples() == 0 || buffer.getNumChannels() < 2)
        return;

    static std::atomic<bool> processLogged { false };
    if (! processLogged.exchange (true))
        CrashLog::write ("first processBlock");

    // Mono in -> dual mono, so the whole chain always runs in stereo.
    if (getTotalNumInputChannels() == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, buffer.getNumSamples());

    using namespace ParamIDs;
    auto& s = apvts;
    BassParams bp;
    bp.compOn = s.getRawParameterValue (compOn)->load() > 0.5f;
    bp.octOn  = s.getRawParameterValue (octOn)->load() > 0.5f;
    bp.driveOn = s.getRawParameterValue (driveOn)->load() > 0.5f;
    bp.filterOn = s.getRawParameterValue (filterOn)->load() > 0.5f;
    bp.chorusOn = s.getRawParameterValue (chorusOn)->load() > 0.5f;
    bp.delayOn = s.getRawParameterValue (delayOn)->load() > 0.5f;

    bp.compSus = s.getRawParameterValue (compSus)->load();
    bp.compAtk = s.getRawParameterValue (compAtk)->load();
    bp.compLvl = s.getRawParameterValue (compLvl)->load();
    bp.octSub = s.getRawParameterValue (octSub)->load();
    bp.octUp  = s.getRawParameterValue (octUp)->load();
    bp.octDry = s.getRawParameterValue (octDry)->load();
    bp.driveGain = s.getRawParameterValue (driveGain)->load();
    bp.driveTone = s.getRawParameterValue (driveTone)->load();
    bp.driveLvl  = s.getRawParameterValue (driveLvl)->load();
    bp.filtSens = s.getRawParameterValue (filtSens)->load();
    bp.filtQ    = s.getRawParameterValue (filtQ)->load();
    bp.filtMix  = s.getRawParameterValue (filtMix)->load();
    bp.choRate  = s.getRawParameterValue (choRate)->load();
    bp.choDepth = s.getRawParameterValue (choDepth)->load();
    bp.choMix   = s.getRawParameterValue (choMix)->load();
    bp.dlyTime = s.getRawParameterValue (dlyTime)->load();
    bp.dlyFb   = s.getRawParameterValue (dlyFb)->load();
    bp.dlyMix  = s.getRawParameterValue (dlyMix)->load();

    bp.ampGain = s.getRawParameterValue (ampGain)->load();
    bp.ampBass = s.getRawParameterValue (ampBass)->load();
    bp.ampMid  = s.getRawParameterValue (ampMid)->load();
    bp.ampTreble = s.getRawParameterValue (ampTreble)->load();
    bp.ampMaster = s.getRawParameterValue (ampMaster)->load();
    bp.ampCircuit = (int) std::round (s.getRawParameterValue (ampCircuit)->load() * 1.f);
    bp.ampPower   = (int) std::round (s.getRawParameterValue (ampPower)->load() * 1.f);

    bp.cabType = (int) std::round (s.getRawParameterValue (cabType)->load() * 2.f);
    bp.micType = (int) std::round (s.getRawParameterValue (micType)->load() * 2.f);
    bp.micDist  = s.getRawParameterValue (micDist)->load();
    bp.micAngle = s.getRawParameterValue (micAngle)->load();
    bp.roomAmt  = s.getRawParameterValue (roomAmt)->load();

    bp.inGain  = s.getRawParameterValue (inGain)->load();
    bp.diMix   = s.getRawParameterValue (diMix)->load();
    bp.outGain = s.getRawParameterValue (outGain)->load();

    chain.process (buffer, bp);
    ++blockCounter;
}

void GrooveSketchbookProcessor::loadPreset (int index)
{
    index = juce::jlimit (0, getNumFactoryPresets() - 1, index);
    currentPreset.store (index);

    const auto& pr = factoryPresets[(size_t) index];

    for (const auto& kv : pr.floats)
        if (auto* p = apvts.getParameter (juce::String (kv.first.c_str())))
            p->setValueNotifyingHost (juce::jlimit (0.f, 1.f, kv.second));

    for (const auto& kv : pr.toggles)
        if (auto* p = apvts.getParameter (juce::String (kv.first.c_str())))
            p->setValueNotifyingHost (kv.second ? 1.f : 0.f);

    for (const auto& kv : pr.choices)
    {
        int n = 2;
        if (kv.first == ParamIDs::cabType || kv.first == ParamIDs::micType)
            n = 3;
        if (auto* p = apvts.getParameter (juce::String (kv.first.c_str())))
            p->setValueNotifyingHost (n > 1 ? (float) kv.second / (float) (n - 1) : 0.f);
    }
}

void GrooveSketchbookProcessor::setChoiceParam (const char* id, int index, int numChoices)
{
    if (auto* p = apvts.getParameter (juce::String (id)))
        p->setValueNotifyingHost (numChoices > 1 ? (float) index / (float) (numChoices - 1) : 0.f);
}

void GrooveSketchbookProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("groovePreset", currentPreset.load(), nullptr);
    juce::MemoryOutputStream mos (destData, false);
    state.writeToStream (mos);
}

void GrooveSketchbookProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData (data, (size_t) sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState (tree);
        currentPreset.store ((int) tree.getProperty ("groovePreset", 0));
    }
}

juce::AudioProcessorEditor* GrooveSketchbookProcessor::createEditor()
{
    CrashLog::write ("createEditor");
    return new GrooveSketchbookEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    CrashLog::write ("createPluginFilter");
    return new GrooveSketchbookProcessor();
}
