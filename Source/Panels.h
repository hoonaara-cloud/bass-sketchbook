#pragma once

//==============================================================================
// Groove Sketchbook — UI panels (preset strip, pedal board, amp, cab+mic...).
//==============================================================================
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Presets.h"

#include <functional>
#include <memory>
#include <vector>

class GrooveSketchbookProcessor;

using APVTS = juce::AudioProcessorValueTreeState;
using SliderAttachment = APVTS::SliderAttachment;
using ButtonAttachment = APVTS::ButtonAttachment;

// Hand-drawn rotary knob bound to a 0..1 parameter.
class Knob : public juce::Slider
{
public:
    Knob (const juce::String& paramID, const juce::String& label);
};

// Base panel: translucent fill + sketch border.
class SketchPanel : public juce::Component
{
public:
    explicit SketchPanel (int seedIn = 1);
    void paint (juce::Graphics& g) override;

protected:
    int seed;
};

// Six player preset buttons.
class PresetStrip : public SketchPanel
{
public:
    explicit PresetStrip (GrooveSketchbookProcessor& proc);
    void resized() override;
    void refresh();

private:
    GrooveSketchbookProcessor& proc;
    juce::OwnedArray<juce::TextButton> buttons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetStrip)
};

// Signal chain: BASS IN -> 6 stomps -> AMP -> CAB.
class PedalBoard : public SketchPanel
{
public:
    PedalBoard (APVTS& state, std::function<void (int)> onSelect);
    void resized() override;
    void paint (juce::Graphics& g) override;
    void selectPedal (int i);
    int getSelected() const { return selected; }

private:
    APVTS& apvts;
    std::function<void (int)> onSelect;
    juce::Label inLabel, ampLabel, cabLabel;
    juce::OwnedArray<juce::TextButton> pedalButtons;
    std::vector<std::unique_ptr<ButtonAttachment>> attachments;
    std::vector<PedalMeta> metas;
    int selected = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PedalBoard)
};

// Big-knob detail view of the selected pedal.
class PedalDetail : public SketchPanel
{
public:
    explicit PedalDetail (APVTS& state);
    void showPedal (int index);
    void resized() override;

private:
    APVTS& apvts;
    juce::Label titleLabel, hintLabel;
    juce::TextButton engageButton;
    juce::OwnedArray<Knob> knobs;
    std::vector<std::unique_ptr<SliderAttachment>> knobAttachments;
    std::unique_ptr<ButtonAttachment> engageAttachment;
    std::vector<PedalMeta> metas;
    int shown = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PedalDetail)
};

// Amp head: 5 knobs + circuit/power switches.
class AmpPanel : public SketchPanel
{
public:
    explicit AmpPanel (GrooveSketchbookProcessor& proc);
    void resized() override;
    void refresh();

private:
    GrooveSketchbookProcessor& proc;
    juce::Label titleLabel, modelLabel;
    juce::OwnedArray<Knob> knobs;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;
    juce::TextButton tubeButton, solidButton, w800Button, w300Button;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmpPanel)
};

// Top-view mic placement diagram.
class MicDiagram : public juce::Component
{
public:
    explicit MicDiagram (GrooveSketchbookProcessor& proc);
    void paint (juce::Graphics& g) override;

private:
    GrooveSketchbookProcessor& proc;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicDiagram)
};

// Cabinet + microphone + placement sliders.
class CabMicPanel : public SketchPanel
{
public:
    explicit CabMicPanel (GrooveSketchbookProcessor& proc);
    void resized() override;
    void refresh();

private:
    GrooveSketchbookProcessor& proc;
    juce::Label titleLabel;
    juce::OwnedArray<juce::TextButton> cabButtons, micButtons;
    juce::OwnedArray<juce::Slider> sliders;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;
    MicDiagram diagram;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabMicPanel)
};

// Handwritten recipe notes of the active preset.
class NotesPanel : public SketchPanel
{
public:
    explicit NotesPanel (GrooveSketchbookProcessor& proc);
    void resized() override;
    void refresh();
    void paint (juce::Graphics& g) override;

private:
    GrooveSketchbookProcessor& proc;
    juce::TextEditor notes;
    int lastPreset = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NotesPanel)
};

// Output meter with a pencil needle.
class MeterComp : public juce::Component
{
public:
    MeterComp() = default;
    void setLevel (float v) { level = v; }
    void paint (juce::Graphics& g) override;

private:
    float level = 0.f, shown = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterComp)
};

// Input / DI mix / output + pluck button + meter.
class MasterSection : public SketchPanel
{
public:
    MasterSection (APVTS& state, std::function<void()> onPluck);
    void resized() override;

    MeterComp meter;

private:
    juce::OwnedArray<Knob> knobs;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;
    juce::TextButton pluckButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterSection)
};
