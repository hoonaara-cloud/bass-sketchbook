#pragma once

//==============================================================================
// Groove Sketchbook — AudioProcessorEditor (the sketchbook page).
//==============================================================================
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "SketchLookAndFeel.h"
#include "Panels.h"

class GrooveSketchbookEditor : public juce::AudioProcessorEditor,
                               private juce::Timer
{
public:
    explicit GrooveSketchbookEditor (GrooveSketchbookProcessor&);
    ~GrooveSketchbookEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshPresetLabels();

    GrooveSketchbookProcessor& proc;
    SketchLookAndFeel sketchLF;
    juce::TooltipWindow tooltipWindow;

    juce::Label titleLabel, companyLabel, presetNameLabel, presetSubLabel;
    PresetStrip presetStrip;
    PedalBoard pedalBoard;
    PedalDetail pedalDetail;
    AmpPanel ampPanel;
    CabMicPanel cabPanel;
    MasterSection master;
    NotesPanel notes;

    int lastPresetShown = -1;
    int lastBlockCount = -1;
    int pluckTick = -1;
    float meterDisplay = 0.f;
    float lastTarget = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrooveSketchbookEditor)
};
