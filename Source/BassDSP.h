#pragma once

//==============================================================================
// Groove Sketchbook — DSP engine.
// Signal flow: input -> comp -> octaver -> drive -> env filter -> chorus ->
// delay -> amp -> cab (+room) -> DI blend -> output.
// All knob values are normalised 0..1 and mapped to real units internally.
//==============================================================================
#include <juce_dsp/juce_dsp.h>

#include <atomic>
#include <cmath>

struct BassParams
{
    bool compOn = true, octOn = false, driveOn = true;
    bool filterOn = false, chorusOn = true, delayOn = false;

    float compSus = .55f, compAtk = .40f, compLvl = .65f;
    float octSub = .50f, octUp = 0.f, octDry = .70f;
    float driveGain = .30f, driveTone = .60f, driveLvl = .70f;
    float filtSens = .50f, filtQ = .50f, filtMix = .50f;
    float choRate = .35f, choDepth = .30f, choMix = .25f;
    float dlyTime = .30f, dlyFb = .20f, dlyMix = .15f;

    float ampGain = .40f, ampBass = .70f, ampMid = .55f;
    float ampTreble = .65f, ampMaster = .80f;
    int ampCircuit = 1; // 0 = tube, 1 = solid-state
    int ampPower = 0;   // 0 = 800W, 1 = 300W

    int cabType = 1; // 0 = 8x10, 1 = 4x10, 2 = 1x15
    int micType = 0; // 0 = D112, 1 = SM57, 2 = U47
    float micDist = .12f, micAngle = .50f, roomAmt = .22f;

    float inGain = .50f, diMix = 1.f, outGain = .778f;
};

// Tiny one-pole helpers (zero API risk, used for tracking / utility paths).
struct OnePoleLP
{
    float y = 0.f, a = 1.f;
    void setCutoff (float fc, double sr) noexcept
        { a = 1.f - std::exp (-2.f * 3.14159265f * fc / (float) sr); }
    float process (float x) noexcept { y += a * (x - y); return y; }
    void reset() noexcept { y = 0.f; }
};

struct OnePoleHP
{
    float x1 = 0.f, y1 = 0.f, a = 1.f;
    void setCutoff (float fc, double sr) noexcept
        { a = std::exp (-2.f * 3.14159265f * fc / (float) sr); }
    float process (float x) noexcept
        { float out = a * (y1 + x - x1); x1 = x; y1 = out; return out; }
    void reset() noexcept { x1 = y1 = 0.f; }
};

using StereoIIR = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                 juce::dsp::IIR::Coefficients<float>>;

// LadderFilter keeps processSample() protected; this subclass exposes it so the
// envelope follower can modulate the cutoff per sample.
class EnvLadderFilter : public juce::dsp::LadderFilter<float>
{
public:
    float processSamplePublic (float x, size_t ch) noexcept { return processSample (x, ch); }
};

class BassChain
{
public:
    BassChain();
    ~BassChain() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const BassParams& p);
    int  getLatencySamples() const;

    std::atomic<float> peakLevel { 0.f };

private:
    void applyComp   (juce::AudioBuffer<float>& buffer, const BassParams& p);
    void applyOct    (juce::AudioBuffer<float>& buffer, const BassParams& p);
    void applyDrive  (juce::AudioBuffer<float>& buffer, const BassParams& p);
    void applyFilter (juce::AudioBuffer<float>& buffer, const BassParams& p);
    void applyChorus (juce::AudioBuffer<float>& buffer, const BassParams& p);
    void applyDelay  (juce::AudioBuffer<float>& buffer, const BassParams& p);
    void applyAmp    (juce::AudioBuffer<float>& buffer, const BassParams& p);
    void applyCab    (juce::AudioBuffer<float>& buffer, const BassParams& p);

    void loadCabIR (int cab, int mic);
    static void mixRamp (juce::AudioBuffer<float>& mainBuf,
                         const juce::AudioBuffer<float>& wetBuf,
                         juce::SmoothedValue<float>& ramp);

    double sampleRate = 48000.0;

    juce::dsp::Gain<float> inputGain, outputGain, compMakeup, ampMasterGain;
    juce::AudioBuffer<float> dryBuffer, wetBuffer;

    // Compressor
    juce::dsp::Compressor<float> comp;
    juce::SmoothedValue<float> compWet;

    // Octaver (pitch-tracked sub + rectified octave-up)
    OnePoleLP trackLP;
    float prevTrack = 0.f, trackPeriod = 585.f, trackHold = 0.f, trackGate = 0.f, subPhase = 0.f;
    OnePoleHP upHP[2];
    juce::SmoothedValue<float> octWet;

    // Drive
    juce::dsp::Oversampling<float> driveOS;
    StereoIIR drivePreTone, drivePostLP;
    juce::SmoothedValue<float> drivePre, driveLvlS, driveWet;

    // Envelope filter
    EnvLadderFilter envFilter;
    float envState = 0.f, envAtk = 0.02f, envRel = 0.001f;
    juce::SmoothedValue<float> filtWet;

    // Chorus
    juce::dsp::Chorus<float> chorus;
    juce::SmoothedValue<float> choWet;

    // Delay
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> dlyL, dlyR;
    float maxDlySamples = 48000.f;
    juce::SmoothedValue<float> dlyWet, dlyFbS;

    // Amp
    juce::dsp::Oversampling<float> ampOS;
    StereoIIR bassShelf, midPeak, trebShelf;
    juce::SmoothedValue<float> ampPre;

    // Cab + room
    juce::dsp::Convolution convolver;
    int cachedCab = -1, cachedMic = -1;
    StereoIIR micLP, presenceShelf;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> roomL, roomR;
    OnePoleLP roomLP[2];
    float roomSamps[2] = { 2256.f, 2544.f };
    juce::SmoothedValue<float> roomMixS, diBlend;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassChain)
};
