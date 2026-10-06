//==============================================================================
// Groove Sketchbook — headless DSP smoke test.
// Runs every factory preset through BassChain and checks for non-finite
// samples, then stress-tests runtime cab/mic IR reloads.
// Build with: cmake -DGROOVE_BUILD_TESTS=ON ... (see README)
//==============================================================================
#include <juce_dsp/juce_dsp.h>

#include "BassDSP.h"
#include "ParamIDs.h"
#include "Presets.h"

#include <cmath>
#include <cstdio>

namespace
{
    float F (const FactoryPreset& pr, const char* id, float dflt)
    {
        auto it = pr.floats.find (id);
        return it == pr.floats.end() ? dflt : it->second;
    }
    int C (const FactoryPreset& pr, const char* id, int dflt)
    {
        auto it = pr.choices.find (id);
        return it == pr.choices.end() ? dflt : it->second;
    }
    bool T (const FactoryPreset& pr, const char* id, bool dflt)
    {
        auto it = pr.toggles.find (id);
        return it == pr.toggles.end() ? dflt : it->second;
    }

    BassParams paramsFromPreset (const FactoryPreset& pr)
    {
        using namespace ParamIDs;
        BassParams bp;
        bp.compOn = T (pr, compOn, true);   bp.octOn = T (pr, octOn, false);
        bp.driveOn = T (pr, driveOn, true); bp.filterOn = T (pr, filterOn, false);
        bp.chorusOn = T (pr, chorusOn, true); bp.delayOn = T (pr, delayOn, false);
        bp.compSus = F (pr, compSus, .5f); bp.compAtk = F (pr, compAtk, .5f);
        bp.compLvl = F (pr, compLvl, .5f);
        bp.octSub = F (pr, octSub, .5f); bp.octUp = F (pr, octUp, 0.f);
        bp.octDry = F (pr, octDry, .7f);
        bp.driveGain = F (pr, driveGain, .3f); bp.driveTone = F (pr, driveTone, .6f);
        bp.driveLvl = F (pr, driveLvl, .7f);
        bp.filtSens = F (pr, filtSens, .5f); bp.filtQ = F (pr, filtQ, .5f);
        bp.filtMix = F (pr, filtMix, .5f);
        bp.choRate = F (pr, choRate, .3f); bp.choDepth = F (pr, choDepth, .3f);
        bp.choMix = F (pr, choMix, .2f);
        bp.dlyTime = F (pr, dlyTime, .3f); bp.dlyFb = F (pr, dlyFb, .2f);
        bp.dlyMix = F (pr, dlyMix, .15f);
        bp.ampGain = F (pr, ampGain, .4f); bp.ampBass = F (pr, ampBass, .7f);
        bp.ampMid = F (pr, ampMid, .5f); bp.ampTreble = F (pr, ampTreble, .6f);
        bp.ampMaster = F (pr, ampMaster, .8f);
        bp.ampCircuit = C (pr, ampCircuit, 1); bp.ampPower = C (pr, ampPower, 0);
        bp.cabType = C (pr, cabType, 1); bp.micType = C (pr, micType, 0);
        bp.micDist = F (pr, micDist, .1f); bp.micAngle = F (pr, micAngle, .5f);
        bp.roomAmt = F (pr, roomAmt, .2f);
        bp.inGain = F (pr, inGain, .5f); bp.diMix = F (pr, diMix, 1.f);
        bp.outGain = F (pr, outGain, .78f);
        return bp;
    }

    bool scanBuffer (const juce::AudioBuffer<float>& buf, double& peak, double& sumSq, long long& n)
    {
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            const float* d = buf.getReadPointer (ch);
            for (int i = 0; i < buf.getNumSamples(); ++i)
            {
                const float v = d[i];
                if (! std::isfinite (v))
                    return false;
                const double a = std::abs ((double) v);
                if (a > peak)
                    peak = a;
                sumSq += a * a;
                ++n;
            }
        }
        return true;
    }
}

int main()
{
    BassChain chain;
    juce::dsp::ProcessSpec spec { 48000.0, 512, 2 };
    chain.prepare (spec);
    std::printf ("reported latency: %d samples\n", chain.getLatencySamples());

    int failures = 0;

    for (const auto& pr : getFactoryPresets())
    {
        const BassParams bp = paramsFromPreset (pr);
        chain.reset();

        juce::AudioBuffer<float> buf (2, 512);
        double peak = 0, sumSq = 0;
        long long total = 0;
        double phase = 0, phase2 = 0;
        bool ok = true;

        for (int b = 0; b < 250 && ok; ++b) // ~2.6 seconds
        {
            for (int i = 0; i < 512; ++i)
            {
                const double t = (double) (b * 512 + i) / 48000.0;
                const double cyc = std::fmod (t, 0.5); // a "pick" every 0.5 s
                const double env = std::exp (-cyc * 6.0);
                phase  += 2.0 * 3.14159265358979 * 55.0 / 48000.0;
                phase2 += 2.0 * 3.14159265358979 * 82.5 / 48000.0;
                const float s = (float) ((std::sin (phase) * 0.5
                                        + std::sin (phase2) * 0.25) * env * 0.8);
                buf.setSample (0, i, s);
                buf.setSample (1, i, s * 0.9f);
            }
            chain.process (buf, bp);
            ok = scanBuffer (buf, peak, sumSq, total);
        }

        const double rms = std::sqrt (sumSq / (double) (total > 0 ? total : 1));
        const bool levelOk = peak <= 4.0;
        std::printf ("[%-7s] peak=%.3f rms=%.4f %s\n",
                     pr.id.c_str(), peak, rms,
                     (ok && levelOk) ? "ok" : "!! FAILED !!");
        if (! (ok && levelOk))
            ++failures;
    }

    // Runtime cab/mic switching stress (IR reloads while processing)
    {
        BassParams bp;
        juce::AudioBuffer<float> buf (2, 512);
        double peak = 0, sumSq = 0;
        long long total = 0;
        bool ok = true;
        double phase = 0;
        for (int k = 0; k < 24 && ok; ++k)
        {
            bp.cabType = k % 3;
            bp.micType = (k / 3) % 3;
            for (int i = 0; i < 512; ++i)
            {
                phase += 2.0 * 3.14159265358979 * 110.0 / 48000.0;
                const float s = (float) (std::sin (phase) * 0.4);
                buf.setSample (0, i, s);
                buf.setSample (1, i, s);
            }
            chain.process (buf, bp);
            ok = scanBuffer (buf, peak, sumSq, total);
        }
        std::printf ("[cab/mic ] peak=%.3f %s\n", peak, ok ? "ok" : "!! FAILED !!");
        if (! ok)
            ++failures;
    }

    if (failures == 0)
        std::printf ("SMOKE OK\n");
    else
        std::printf ("SMOKE FAILED (%d)\n", failures);

    return failures == 0 ? 0 : 1;
}
