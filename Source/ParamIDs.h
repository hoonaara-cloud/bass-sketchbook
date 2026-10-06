#pragma once

//==============================================================================
// Groove Sketchbook — central registry of APVTS parameter IDs.
// All knob parameters are normalised 0..1 floats (mapped to real units in DSP).
//==============================================================================
namespace ParamIDs
{
    // Stomp on/off
    inline constexpr const char* compOn   = "comp_on";
    inline constexpr const char* octOn    = "oct_on";
    inline constexpr const char* driveOn  = "drive_on";
    inline constexpr const char* filterOn = "filter_on";
    inline constexpr const char* chorusOn = "chorus_on";
    inline constexpr const char* delayOn  = "delay_on";

    // Compressor
    inline constexpr const char* compSus = "comp_sus";
    inline constexpr const char* compAtk = "comp_atk";
    inline constexpr const char* compLvl = "comp_lvl";

    // Octaver (sub / octave-up / dry)
    inline constexpr const char* octSub = "oct_sub";
    inline constexpr const char* octUp  = "oct_up";
    inline constexpr const char* octDry = "oct_dry";

    // Drive / fuzz
    inline constexpr const char* driveGain = "drive_gain";
    inline constexpr const char* driveTone = "drive_tone";
    inline constexpr const char* driveLvl  = "drive_lvl";

    // Envelope filter
    inline constexpr const char* filtSens = "filt_sens";
    inline constexpr const char* filtQ    = "filt_q";
    inline constexpr const char* filtMix  = "filt_mix";

    // Chorus
    inline constexpr const char* choRate  = "cho_rate";
    inline constexpr const char* choDepth = "cho_depth";
    inline constexpr const char* choMix   = "cho_mix";

    // Delay
    inline constexpr const char* dlyTime = "dly_time";
    inline constexpr const char* dlyFb   = "dly_fb";
    inline constexpr const char* dlyMix  = "dly_mix";

    // Amp
    inline constexpr const char* ampGain   = "amp_gain";
    inline constexpr const char* ampBass   = "amp_bass";
    inline constexpr const char* ampMid    = "amp_mid";
    inline constexpr const char* ampTreble = "amp_treble";
    inline constexpr const char* ampMaster = "amp_master";
    inline constexpr const char* ampCircuit = "amp_circuit"; // choice: Tube / Solid
    inline constexpr const char* ampPower   = "amp_power";   // choice: 800W / 300W

    // Cab + mic
    inline constexpr const char* cabType  = "cab_type";  // choice: 8x10 / 4x10 / 1x15
    inline constexpr const char* micType  = "mic_type";  // choice: D112 / SM57 / U47
    inline constexpr const char* micDist  = "mic_dist";
    inline constexpr const char* micAngle = "mic_angle";
    inline constexpr const char* roomAmt  = "room_amt";

    // Master
    inline constexpr const char* inGain  = "in_gain";
    inline constexpr const char* diMix   = "di_mix";
    inline constexpr const char* outGain = "out_gain";
} // namespace ParamIDs
