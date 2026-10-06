#pragma once

//==============================================================================
// Groove Sketchbook — factory presets (tones of the masters) + UI metadata.
// Pure standard C++ on purpose: also usable from the headless DSP smoke test.
//==============================================================================
#include "ParamIDs.h"

#include <map>
#include <string>
#include <vector>

struct FactoryPreset
{
    std::string id;
    std::string label;
    std::string sub;
    std::string ampLabel;
    std::string notes;

    std::map<std::string, float> floats; // normalised 0..1
    std::map<std::string, int>   choices; // indices
    std::map<std::string, bool>  toggles;
};

inline std::vector<FactoryPreset> getFactoryPresets()
{
    using namespace ParamIDs;

    const float outDefault = 0.778f; // ~ -6 dB

    FactoryPreset flea { "flea", "FLEA",
        "Californication funk - StingRay -> GK 800RB", "GK 800RB",
        "FLEA recipe: fresh roundwounds -> comp 4:1 -> hair of drive -> GK with boost @800Hz for the pop. "
        "Play ahead of the beat! Try: chorus mix UP for Under the Bridge verses.",
        { { compSus, .55f }, { compAtk, .40f }, { compLvl, .65f },
          { octSub, .50f }, { octUp, 0.f }, { octDry, .70f },
          { driveGain, .30f }, { driveTone, .60f }, { driveLvl, .70f },
          { filtSens, .50f }, { filtQ, .50f }, { filtMix, .50f },
          { choRate, .35f }, { choDepth, .30f }, { choMix, .25f },
          { dlyTime, .30f }, { dlyFb, .20f }, { dlyMix, .15f },
          { ampGain, .40f }, { ampBass, .70f }, { ampMid, .55f }, { ampTreble, .65f }, { ampMaster, .80f },
          { micDist, .12f }, { micAngle, .50f }, { roomAmt, .22f },
          { inGain, .50f }, { diMix, 1.f }, { outGain, outDefault } },
        { { ampCircuit, 1 }, { ampPower, 0 }, { cabType, 1 }, { micType, 0 } },
        { { compOn, true }, { octOn, false }, { driveOn, true },
          { filterOn, false }, { chorusOn, true }, { delayOn, false } } };

    FactoryPreset bootsy { "bootsy", "BOOTSY",
        "Space bass - Star -> Mu-Tron -> SVT", "SVT Classic",
        "BOOTSY recipe: envelope WIDE OPEN -> sub-octave underneath -> SVT fridge. "
        "Play ON THE ONE and let the filter do the talking. Star-shaped glasses: optional but encouraged.",
        { { compSus, .60f }, { compAtk, .35f }, { compLvl, .70f },
          { octSub, .70f }, { octUp, .15f }, { octDry, .60f },
          { driveGain, .20f }, { driveTone, .50f }, { driveLvl, .50f },
          { filtSens, .80f }, { filtQ, .65f }, { filtMix, .80f },
          { choRate, .30f }, { choDepth, .30f }, { choMix, .20f },
          { dlyTime, .30f }, { dlyFb, .20f }, { dlyMix, .15f },
          { ampGain, .55f }, { ampBass, .85f }, { ampMid, .45f }, { ampTreble, .55f }, { ampMaster, .85f },
          { micDist, .08f }, { micAngle, .50f }, { roomAmt, .15f },
          { inGain, .50f }, { diMix, 1.f }, { outGain, outDefault } },
        { { ampCircuit, 0 }, { ampPower, 1 }, { cabType, 0 }, { micType, 0 } },
        { { compOn, true }, { octOn, true }, { driveOn, false },
          { filterOn, true }, { chorusOn, false }, { delayOn, false } } };

    FactoryPreset jaco { "jaco", "JACO",
        "Fretless song - Jazz -> Acoustic 360", "Acoustic 360",
        "JACO recipe: fretless, bridge pickup soloed, tone rolled -> light comp -> chorus doubling -> 360 + 15 inch. "
        "The MWAH lives behind the 7th fret - vibrato with intent.",
        { { compSus, .35f }, { compAtk, .60f }, { compLvl, .60f },
          { octSub, .30f }, { octUp, 0.f }, { octDry, .70f },
          { driveGain, .15f }, { driveTone, .50f }, { driveLvl, .50f },
          { filtSens, .40f }, { filtQ, .40f }, { filtMix, .40f },
          { choRate, .25f }, { choDepth, .45f }, { choMix, .55f },
          { dlyTime, .45f }, { dlyFb, .30f }, { dlyMix, .25f },
          { ampGain, .35f }, { ampBass, .55f }, { ampMid, .65f }, { ampTreble, .45f }, { ampMaster, .60f },
          { micDist, .45f }, { micAngle, .389f }, { roomAmt, .55f },
          { inGain, .50f }, { diMix, 1.f }, { outGain, outDefault } },
        { { ampCircuit, 1 }, { ampPower, 1 }, { cabType, 2 }, { micType, 2 } },
        { { compOn, true }, { octOn, false }, { driveOn, false },
          { filterOn, false }, { chorusOn, true }, { delayOn, true } } };

    FactoryPreset geddy { "geddy", "GEDDY",
        "Rush grind - Ric -> SVT + DI blend", "SVT + DI",
        "GEDDY recipe: Ric stereo -> gritty drive -> SVT, plus 60% clean DI underneath (MIX knob!). "
        "Mids are the message - cut through two guitars and a drum kit the size of a house.",
        { { compSus, .50f }, { compAtk, .45f }, { compLvl, .65f },
          { octSub, .30f }, { octUp, 0.f }, { octDry, .70f },
          { driveGain, .55f }, { driveTone, .70f }, { driveLvl, .65f },
          { filtSens, .40f }, { filtQ, .40f }, { filtMix, .40f },
          { choRate, .40f }, { choDepth, .35f }, { choMix, .40f },
          { dlyTime, .30f }, { dlyFb, .20f }, { dlyMix, .15f },
          { ampGain, .65f }, { ampBass, .60f }, { ampMid, .80f }, { ampTreble, .70f }, { ampMaster, .75f },
          { micDist, .05f }, { micAngle, .667f }, { roomAmt, .18f },
          { inGain, .50f }, { diMix, .60f }, { outGain, outDefault } },
        { { ampCircuit, 0 }, { ampPower, 1 }, { cabType, 0 }, { micType, 1 } },
        { { compOn, true }, { octOn, false }, { driveOn, true },
          { filterOn, false }, { chorusOn, true }, { delayOn, false } } };

    FactoryPreset cliff { "cliff", "CLIFF",
        "Anesthesia pull - Ric -> fuzz + wah", "Tube 300W",
        "CLIFF recipe: fuzz DIMED -> wah parked as a filter -> tube head breaking up. "
        "Anesthesia: the bass solo as heavy metal scripture. Headbanging mandatory.",
        { { compSus, .40f }, { compAtk, .40f }, { compLvl, .50f },
          { octSub, .30f }, { octUp, 0.f }, { octDry, .70f },
          { driveGain, .90f }, { driveTone, .55f }, { driveLvl, .75f },
          { filtSens, .60f }, { filtQ, .75f }, { filtMix, .70f },
          { choRate, .30f }, { choDepth, .30f }, { choMix, .20f },
          { dlyTime, .55f }, { dlyFb, .45f }, { dlyMix, .35f },
          { ampGain, .80f }, { ampBass, .65f }, { ampMid, .70f }, { ampTreble, .60f }, { ampMaster, .85f },
          { micDist, .10f }, { micAngle, .50f }, { roomAmt, .30f },
          { inGain, .50f }, { diMix, 1.f }, { outGain, outDefault } },
        { { ampCircuit, 0 }, { ampPower, 1 }, { cabType, 1 }, { micType, 1 } },
        { { compOn, false }, { octOn, false }, { driveOn, true },
          { filterOn, true }, { chorusOn, false }, { delayOn, true } } };

    FactoryPreset wooten { "wooten", "WOOTEN",
        "Modern thump - Fodera -> hi-fi rig", "Hi-Fi 800W",
        "WOOTEN recipe: fast comp -> touch of sub -> gentle envelope -> hi-fi head. "
        "Lows TIGHT, technique frightening - thumb like a metronome with feelings.",
        { { compSus, .65f }, { compAtk, .70f }, { compLvl, .70f },
          { octSub, .45f }, { octUp, 0.f }, { octDry, .75f },
          { driveGain, .20f }, { driveTone, .50f }, { driveLvl, .50f },
          { filtSens, .55f }, { filtQ, .45f }, { filtMix, .35f },
          { choRate, .30f }, { choDepth, .30f }, { choMix, .20f },
          { dlyTime, .30f }, { dlyFb, .18f }, { dlyMix, .12f },
          { ampGain, .45f }, { ampBass, .60f }, { ampMid, .55f }, { ampTreble, .70f }, { ampMaster, .70f },
          { micDist, .20f }, { micAngle, .556f }, { roomAmt, .35f },
          { inGain, .50f }, { diMix, 1.f }, { outGain, outDefault } },
        { { ampCircuit, 1 }, { ampPower, 0 }, { cabType, 1 }, { micType, 0 } },
        { { compOn, true }, { octOn, true }, { driveOn, false },
          { filterOn, true }, { chorusOn, false }, { delayOn, true } } };

    return { flea, bootsy, jaco, geddy, cliff, wooten };
}

//==============================================================================
// UI metadata: pedal order, knob parameters and labels.
//==============================================================================
struct PedalMeta
{
    std::string id;
    std::string shortName;
    std::string fullName;
    std::string hint;
    std::string onParam;
    std::string knobParams[3];
    std::string knobLabels[3];
};

inline std::vector<PedalMeta> getPedalMetas()
{
    using namespace ParamIDs;
    return {
        { "comp", "COMP", "COMPRESSOR", "round & even - tames slap & pop", compOn,
          { compSus, compAtk, compLvl }, { "SUSTAIN", "ATTACK", "LEVEL" } },
        { "oct", "OCT", "OCTAVER", "sub-thunder, one octave down", octOn,
          { octSub, octUp, octDry }, { "SUB", "UP", "DRY" } },
        { "drive", "DRIVE", "DRIVE / FUZZ", "from woolly edge to full meltdown", driveOn,
          { driveGain, driveTone, driveLvl }, { "GAIN", "TONE", "LEVEL" } },
        { "filter", "FILTER", "ENV FILTER", "quack like a duck with a groove", filterOn,
          { filtSens, filtQ, filtMix }, { "SENS", "Q", "MIX" } },
        { "chorus", "CHORUS", "CHORUS", "watery width - the 80s called", chorusOn,
          { choRate, choDepth, choMix }, { "RATE", "DEPTH", "MIX" } },
        { "delay", "DELAY", "DELAY", "space echoes for bass solos", delayOn,
          { dlyTime, dlyFb, dlyMix }, { "TIME", "REPEATS", "MIX" } },
    };
}

struct KnobMeta
{
    std::string param;
    std::string label;
};

inline std::vector<KnobMeta> getAmpKnobs()
{
    using namespace ParamIDs;
    return { { ampGain, "GAIN" }, { ampBass, "BASS" }, { ampMid, "MID" },
             { ampTreble, "TREBLE" }, { ampMaster, "MASTER" } };
}

inline std::vector<std::string> getCabNames() { return { "8x10", "4x10", "1x15" }; }
inline std::vector<std::string> getMicNames() { return { "D112", "SM57", "U47" }; }
