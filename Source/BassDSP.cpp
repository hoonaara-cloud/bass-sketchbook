#include "BassDSP.h"

namespace
{
    float expMap (float v, float lo, float hi)
    {
        v = juce::jlimit (0.f, 1.f, v);
        return lo * std::pow (hi / lo, v);
    }

    // JUCE's shelf/peak makers expect LINEAR gain factors, not dB.
    inline float dbToLin (float db) { return juce::Decibels::decibelsToGain (db); }
}

BassChain::BassChain()
    : driveOS (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR),
      ampOS   (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR)
{
}

void BassChain::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    jassert (spec.numChannels == 2);
    const auto maxBlock = (int) spec.maximumBlockSize;

    inputGain.prepare (spec);  outputGain.prepare (spec);
    compMakeup.prepare (spec); ampMasterGain.prepare (spec);
    inputGain.setRampDurationSeconds (0.02);
    outputGain.setRampDurationSeconds (0.02);
    compMakeup.setRampDurationSeconds (0.02);
    ampMasterGain.setRampDurationSeconds (0.02);

    comp.prepare (spec);

    drivePreTone.prepare (spec); drivePostLP.prepare (spec);
    driveOS.initProcessing ((size_t) maxBlock);

    envFilter.prepare (spec);
    envFilter.setMode (juce::dsp::LadderFilterMode::LPF24);

    chorus.prepare (spec);

    juce::dsp::ProcessSpec dlSpec { sampleRate, spec.maximumBlockSize, 1 };
    dlyL.setMaximumDelayInSamples ((int) (sampleRate * 1.0) + 64);
    dlyR.setMaximumDelayInSamples ((int) (sampleRate * 1.0) + 64);
    dlyL.prepare (dlSpec); dlyR.prepare (dlSpec);
    maxDlySamples = (float) dlyL.getMaximumDelayInSamples();

    bassShelf.prepare (spec); midPeak.prepare (spec); trebShelf.prepare (spec);
    ampOS.initProcessing ((size_t) maxBlock);

    micLP.prepare (spec); presenceShelf.prepare (spec);

    roomL.setMaximumDelayInSamples ((int) (sampleRate * 0.25));
    roomR.setMaximumDelayInSamples ((int) (sampleRate * 0.25));
    roomL.prepare (dlSpec); roomR.prepare (dlSpec);
    roomSamps[0] = (float) (0.047 * sampleRate);
    roomSamps[1] = (float) (0.053 * sampleRate);

    loadCabIR (1, 0); // load IR *before* convolver.prepare, as JUCE recommends
    convolver.prepare (spec);
    cachedCab = 1; cachedMic = 0;

    dryBuffer.setSize (2, maxBlock, false, false, false);
    wetBuffer.setSize (2, maxBlock, false, false, false);

    compWet.reset (sampleRate, 0.03);
    octWet.reset (sampleRate, 0.03);
    drivePre.reset (sampleRate, 0.02);
    driveLvlS.reset (sampleRate, 0.03);
    driveWet.reset (sampleRate, 0.03);
    filtWet.reset (sampleRate, 0.03);
    choWet.reset (sampleRate, 0.03);
    dlyWet.reset (sampleRate, 0.04);
    dlyFbS.reset (sampleRate, 0.05);
    ampPre.reset (sampleRate, 0.02);
    roomMixS.reset (sampleRate, 0.05);
    diBlend.reset (sampleRate, 0.05);

    trackLP.setCutoff (180.f, sampleRate);
    upHP[0].setCutoff (25.f, sampleRate); upHP[1].setCutoff (25.f, sampleRate);
    roomLP[0].setCutoff (2200.f, sampleRate); roomLP[1].setCutoff (2200.f, sampleRate);

    trackPeriod = (float) (sampleRate / 82.4);

    envAtk = 1.f - std::exp (-1.f / (0.005f * (float) sampleRate));
    envRel = 1.f - std::exp (-1.f / (0.18f * (float) sampleRate));

    reset();
}

void BassChain::reset()
{
    inputGain.reset(); outputGain.reset();
    compMakeup.reset(); ampMasterGain.reset();
    comp.reset();
    drivePreTone.reset(); drivePostLP.reset(); driveOS.reset();
    envFilter.reset(); envState = 0.f;
    chorus.reset();
    dlyL.reset(); dlyR.reset();
    bassShelf.reset(); midPeak.reset(); trebShelf.reset(); ampOS.reset();
    convolver.reset();
    micLP.reset(); presenceShelf.reset();
    roomL.reset(); roomR.reset();
    trackLP.reset(); upHP[0].reset(); upHP[1].reset();
    roomLP[0].reset(); roomLP[1].reset();
    prevTrack = 0.f; trackHold = 0.f; trackGate = 0.f; subPhase = 0.f;
    dryBuffer.clear(); wetBuffer.clear();
}

int BassChain::getLatencySamples() const
{
    const float lat = (float) convolver.getLatency()
                    + driveOS.getLatencyInSamples()
                    + ampOS.getLatencyInSamples();
    return (int) std::ceil (lat);
}

void BassChain::mixRamp (juce::AudioBuffer<float>& mainBuf,
                         const juce::AudioBuffer<float>& wetBuf,
                         juce::SmoothedValue<float>& ramp)
{
    const int n = mainBuf.getNumSamples();
    for (int ch = 0; ch < 2; ++ch)
    {
        float* m = mainBuf.getWritePointer (ch);
        const float* w = wetBuf.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
            m[i] += (w[i] - m[i]) * ramp.getNextValue();
    }
}

void BassChain::process (juce::AudioBuffer<float>& buffer, const BassParams& p)
{
    jassert (buffer.getNumChannels() == 2);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    inputGain.setGainDecibels (24.f * p.inGain - 12.f);
    inputGain.process (ctx);

    dryBuffer.makeCopyOf (buffer);

    applyComp (buffer, p);
    applyOct (buffer, p);
    applyDrive (buffer, p);
    applyFilter (buffer, p);
    applyChorus (buffer, p);
    applyDelay (buffer, p);
    applyAmp (buffer, p);
    applyCab (buffer, p);

    // DI blend against the post-input dry copy
    diBlend.setTargetValue (p.diMix);
    const int n = buffer.getNumSamples();
    for (int ch = 0; ch < 2; ++ch)
    {
        float* w = buffer.getWritePointer (ch);
        const float* d = dryBuffer.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
            w[i] = d[i] + (w[i] - d[i]) * diBlend.getNextValue();
    }

    juce::dsp::AudioBlock<float> block2 (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx2 (block2);
    outputGain.setGainDecibels (54.f * p.outGain - 48.f);
    outputGain.process (ctx2);

    // Safety soft-clip + peak metering
    float peak = 0.f;
    for (int ch = 0; ch < 2; ++ch)
    {
        float* w = buffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            const float y = std::tanh (w[i]);
            w[i] = y;
            const float a = std::abs (y);
            if (a > peak)
                peak = a;
        }
    }
    peakLevel.store (peak);
}

void BassChain::applyComp (juce::AudioBuffer<float>& buffer, const BassParams& p)
{
    comp.setThreshold (-50.f + p.compSus * 42.f);
    comp.setRatio (1.5f + p.compSus * 18.5f);
    comp.setAttack (expMap (p.compAtk, 0.1f, 80.f));
    comp.setRelease (120.f);
    compMakeup.setGainDecibels (p.compLvl * 12.f);
    compWet.setTargetValue (p.compOn ? 1.f : 0.f);

    wetBuffer.makeCopyOf (buffer);
    juce::dsp::AudioBlock<float> wb (wetBuffer);
    juce::dsp::ProcessContextReplacing<float> ctx (wb);
    comp.process (ctx);
    compMakeup.process (ctx);
    mixRamp (buffer, wetBuffer, compWet);
}

void BassChain::applyOct (juce::AudioBuffer<float>& buffer, const BassParams& p)
{
    octWet.setTargetValue (p.octOn ? 1.f : 0.f);

    const int n = buffer.getNumSamples();
    const float* inL = buffer.getReadPointer (0);
    const float* inR = buffer.getReadPointer (1);
    float* wL = wetBuffer.getWritePointer (0);
    float* wR = wetBuffer.getWritePointer (1);

    const float minPeriod = (float) (sampleRate / 300.0);
    const float maxPeriod = (float) (sampleRate / 28.0);
    const float twoPi = 6.2831853f;

    for (int i = 0; i < n; ++i)
    {
        const float l = inL[i], r = inR[i];
        const float tr = trackLP.process (0.5f * (l + r));

        // Zero-crossing period tracking on the lowpassed mono sum
        if (prevTrack <= 0.f && tr > 0.f)
        {
            if (trackHold > minPeriod * 0.7f)
            {
                const float per = juce::jlimit (minPeriod, maxPeriod, trackHold);
                trackPeriod += (per - trackPeriod) * 0.35f;
            }
            trackHold = 0.f;
        }
        trackHold += 1.f;
        prevTrack = tr;

        const float gateTarget = (trackHold < (float) (sampleRate * 0.06)) ? 1.f : 0.f;
        trackGate += (gateTarget - trackGate) * (gateTarget > trackGate ? 0.01f : 0.0005f);

        const float lvl = juce::jmin (1.f, std::abs (tr) * 6.f);
        float freq = (float) (sampleRate / juce::jmax (1.f, trackPeriod));
        freq = juce::jlimit (28.f, 300.f, freq);

        subPhase += twoPi * (freq * 0.5f) / (float) sampleRate;
        if (subPhase > twoPi)
            subPhase -= twoPi;

        const float sub = std::sin (subPhase) * trackGate * lvl;
        const float up1 = upHP[0].process (std::abs (l) * 1.6f);
        const float up2 = upHP[1].process (std::abs (r) * 1.6f);

        wL[i] = l * p.octDry + sub * p.octSub * 0.9f + up1 * p.octUp * 0.5f;
        wR[i] = r * p.octDry + sub * p.octSub * 0.9f + up2 * p.octUp * 0.5f;
    }
    mixRamp (buffer, wetBuffer, octWet);
}

void BassChain::applyDrive (juce::AudioBuffer<float>& buffer, const BassParams& p)
{
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    driveWet.setTargetValue (p.driveOn ? 1.f : 0.f);
    drivePre.setTargetValue (1.f + p.driveGain * 39.f);
    driveLvlS.setTargetValue (p.driveLvl * 1.5f);

    wetBuffer.makeCopyOf (buffer);

    *drivePreTone.state = *Coeffs::makeLowPass (sampleRate, expMap (p.driveTone, 700.f, 9000.f));

    juce::dsp::AudioBlock<float> wb (wetBuffer);
    juce::dsp::ProcessContextReplacing<float> ctx (wb);
    drivePreTone.process (ctx);

    auto osBlock = driveOS.processSamplesUp (ctx.getInputBlock());
    for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
    {
        float* d = osBlock.getChannelPointer (ch);
        for (size_t i = 0; i < osBlock.getNumSamples(); ++i)
            d[i] = std::tanh (d[i] * drivePre.getNextValue());
    }
    driveOS.processSamplesDown (ctx.getOutputBlock());

    *drivePostLP.state = *Coeffs::makeLowPass (sampleRate, 7500.f);
    drivePostLP.process (ctx);

    const int n = wetBuffer.getNumSamples();
    for (int ch = 0; ch < 2; ++ch)
    {
        float* w = wetBuffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
            w[i] *= driveLvlS.getNextValue();
    }
    mixRamp (buffer, wetBuffer, driveWet);
}

void BassChain::applyFilter (juce::AudioBuffer<float>& buffer, const BassParams& p)
{
    filtWet.setTargetValue (p.filterOn ? p.filtMix : 0.f);
    envFilter.setResonance (0.05f + p.filtQ * 0.80f);

    wetBuffer.makeCopyOf (buffer);
    const int n = buffer.getNumSamples();
    float* wL = wetBuffer.getWritePointer (0);
    float* wR = wetBuffer.getWritePointer (1);
    const float range = p.filtSens * 6.5f;

    for (int i = 0; i < n; ++i)
    {
        const float mono = 0.5f * (wL[i] + wR[i]);
        const float a = std::abs (mono);
        envState += (a - envState) * (a > envState ? envAtk : envRel);
        const float envN = juce::jlimit (0.f, 1.f, envState * 2.5f);
        envFilter.setCutoffFrequencyHz (160.f * std::pow (2.f, range * envN));
        wL[i] = envFilter.processSamplePublic (wL[i], 0);
        wR[i] = envFilter.processSamplePublic (wR[i], 1);
    }
    mixRamp (buffer, wetBuffer, filtWet);
}

void BassChain::applyChorus (juce::AudioBuffer<float>& buffer, const BassParams& p)
{
    choWet.setTargetValue (p.chorusOn ? 1.f : 0.f);
    chorus.setRate (expMap (p.choRate, 0.05f, 8.f));
    chorus.setDepth (p.choDepth);
    chorus.setCentreDelay (5.5f);
    chorus.setFeedback (0.25f);
    chorus.setMix (p.choMix);

    wetBuffer.makeCopyOf (buffer);
    juce::dsp::AudioBlock<float> wb (wetBuffer);
    juce::dsp::ProcessContextReplacing<float> ctx (wb);
    chorus.process (ctx);
    mixRamp (buffer, wetBuffer, choWet);
}

void BassChain::applyDelay (juce::AudioBuffer<float>& buffer, const BassParams& p)
{
    dlyWet.setTargetValue (p.delayOn ? p.dlyMix : 0.f);
    dlyFbS.setTargetValue (p.dlyFb * 0.72f);

    const float dMs = expMap (p.dlyTime, 20.f, 700.f);
    float dL = juce::jmin (dMs * 0.001f * (float) sampleRate, maxDlySamples - 1.f);
    float dR = juce::jmin (dL + 0.007f * (float) sampleRate, maxDlySamples - 1.f);

    wetBuffer.makeCopyOf (buffer);
    const int n = buffer.getNumSamples();
    float* wL = wetBuffer.getWritePointer (0);
    float* wR = wetBuffer.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        const float inL = wL[i], inR = wR[i];
        const float echoL = dlyL.popSample (0, dL);
        const float echoR = dlyR.popSample (0, dR);
        const float fb = dlyFbS.getNextValue();
        dlyL.pushSample (0, inL + echoL * fb);
        dlyR.pushSample (0, inR + echoR * fb);
        const float w = dlyWet.getNextValue();
        wL[i] = inL + (echoL - inL) * w;
        wR[i] = inR + (echoR - inR) * w;
    }
    buffer.makeCopyOf (wetBuffer);
}

void BassChain::applyAmp (juce::AudioBuffer<float>& buffer, const BassParams& p)
{
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    const float headroom = (p.ampPower == 0) ? 1.f : 1.35f; // 300W breaks up earlier
    ampPre.setTargetValue ((1.f + p.ampGain * 24.f) * headroom);

    *bassShelf.state = *Coeffs::makeLowShelf  (sampleRate, 110.f, 0.7f, dbToLin ((p.ampBass - 0.5f) * 24.f));
    *midPeak.state   = *Coeffs::makePeakFilter (sampleRate, 750.f, 0.9f, dbToLin ((p.ampMid - 0.5f) * 24.f));
    *trebShelf.state = *Coeffs::makeHighShelf (sampleRate, 3200.f, 0.7f, dbToLin ((p.ampTreble - 0.5f) * 24.f));

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    bassShelf.process (ctx);
    midPeak.process (ctx);
    trebShelf.process (ctx);

    const bool tube = (p.ampCircuit == 0);
    auto osBlock = ampOS.processSamplesUp (ctx.getInputBlock());
    for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
    {
        float* d = osBlock.getChannelPointer (ch);
        for (size_t i = 0; i < osBlock.getNumSamples(); ++i)
        {
            const float x = d[i] * ampPre.getNextValue();
            d[i] = tube ? std::tanh (x) : std::atan (x) * 1.25f;
        }
    }
    ampOS.processSamplesDown (ctx.getOutputBlock());

    ampMasterGain.setGainLinear (p.ampMaster * p.ampMaster * 1.5f);
    ampMasterGain.process (ctx);
}

void BassChain::applyCab (juce::AudioBuffer<float>& buffer, const BassParams& p)
{
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    if (p.cabType != cachedCab || p.micType != cachedMic)
    {
        loadCabIR (p.cabType, p.micType);
        cachedCab = p.cabType;
        cachedMic = p.micType;
    }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    convolver.process (ctx);

    *micLP.state = *Coeffs::makeLowPass (sampleRate, expMap (1.f - p.micDist, 2200.f, 18000.f));
    micLP.process (ctx);

    const float deg = p.micAngle * 90.f - 45.f;
    const float cut = std::abs (deg) / 45.f * 6.f;
    *presenceShelf.state = *Coeffs::makeHighShelf (sampleRate, 5000.f, 0.7f, dbToLin (-cut));
    presenceShelf.process (ctx);

    // Cheap "room": short dual feedback delay, lowpassed, added in parallel
    roomMixS.setTargetValue (p.roomAmt * 0.4f);
    const int n = buffer.getNumSamples();
    float* wL = buffer.getWritePointer (0);
    float* wR = buffer.getWritePointer (1);
    for (int i = 0; i < n; ++i)
    {
        const float inL = wL[i], inR = wR[i];
        const float rL = roomL.popSample (0, roomSamps[0]);
        const float rR = roomR.popSample (0, roomSamps[1]);
        roomL.pushSample (0, inL * 0.5f + rL * 0.32f);
        roomR.pushSample (0, inR * 0.5f + rR * 0.32f);
        const float m = roomMixS.getNextValue();
        wL[i] = inL + roomLP[0].process (rL) * m;
        wR[i] = inR + roomLP[1].process (rR) * m;
    }
}

void BassChain::loadCabIR (int cab, int mic)
{
    const int len = juce::jmax (256, (int) (sampleRate * 0.16));
    juce::AudioBuffer<float> ir (2, len);

    // Deterministic noise burst with exponential decay
    juce::Random rng ((juce::uint32) ((cab + 1) * 1000 + (mic + 1) * 77 + 13));
    for (int ch = 0; ch < 2; ++ch)
    {
        float* d = ir.getWritePointer (ch);
        for (int i = 0; i < len; ++i)
        {
            const float t = (float) i / (float) len;
            d[i] = (rng.nextFloat() * 2.f - 1.f) * std::exp (-4.5f * t);
        }
    }

    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    using Filter = juce::dsp::IIR::Filter<float>;

    auto runBiquad = [&] (typename Coeffs::Ptr c)
    {
        Filter fL (c), fR (c);
        float* dL = ir.getWritePointer (0);
        float* dR = ir.getWritePointer (1);
        for (int i = 0; i < len; ++i)
        {
            dL[i] = fL.processSample (dL[i]);
            dR[i] = fR.processSample (dR[i]);
        }
    };

    // Cabinet body
    if (cab == 0) // 8x10 fridge
    {
        runBiquad (Coeffs::makeHighPass (sampleRate, 58.f));
        runBiquad (Coeffs::makeLowPass (sampleRate, 5200.f));
        runBiquad (Coeffs::makePeakFilter (sampleRate, 110.f, 1.f, dbToLin (4.f)));
    }
    else if (cab == 2) // 1x15 wool
    {
        runBiquad (Coeffs::makeHighPass (sampleRate, 42.f));
        runBiquad (Coeffs::makeLowPass (sampleRate, 3400.f));
        runBiquad (Coeffs::makePeakFilter (sampleRate, 70.f, 1.f, dbToLin (5.f)));
    }
    else // 4x10 punch
    {
        runBiquad (Coeffs::makeHighPass (sampleRate, 52.f));
        runBiquad (Coeffs::makeLowPass (sampleRate, 5600.f));
        runBiquad (Coeffs::makePeakFilter (sampleRate, 100.f, 1.f, dbToLin (3.f)));
    }

    // Microphone colour
    if (mic == 0) // D112: fat lows + beater click
    {
        runBiquad (Coeffs::makeLowShelf (sampleRate, 120.f, 0.7f, dbToLin (3.f)));
        runBiquad (Coeffs::makePeakFilter (sampleRate, 3800.f, 1.f, dbToLin (4.f)));
    }
    else if (mic == 1) // SM57: mid bite
    {
        runBiquad (Coeffs::makeHighPass (sampleRate, 110.f));
        runBiquad (Coeffs::makePeakFilter (sampleRate, 4500.f, 0.8f, dbToLin (3.5f)));
    }
    else // U47: warm wool
    {
        runBiquad (Coeffs::makeLowShelf (sampleRate, 150.f, 0.7f, dbToLin (2.f)));
        runBiquad (Coeffs::makeHighShelf (sampleRate, 9000.f, 0.7f, dbToLin (-2.f)));
    }

    float peak = 0.001f;
    for (int ch = 0; ch < 2; ++ch)
        peak = juce::jmax (peak, ir.getMagnitude (ch, 0, len));
    ir.applyGain (0.9f / peak);

    const int fade = juce::jmin (256, len / 8);
    for (int ch = 0; ch < 2; ++ch)
    {
        float* d = ir.getWritePointer (ch);
        for (int i = 0; i < fade; ++i)
            d[len - 1 - i] *= (float) i / (float) fade;
    }

    convolver.loadImpulseResponse (std::move (ir), sampleRate,
                                   juce::dsp::Convolution::Stereo::yes,
                                   juce::dsp::Convolution::Trim::no,
                                   juce::dsp::Convolution::Normalise::yes);
}
