#include "PluginEditor.h"
#include "CrashLog.h"

#include <cmath>

namespace
{
    juce::String S (const std::string& s) { return juce::String (s.c_str()); }
}

GrooveSketchbookEditor::GrooveSketchbookEditor (GrooveSketchbookProcessor& p)
    : AudioProcessorEditor (p), proc (p),
      presetStrip (p),
      pedalBoard (p.apvts, [this] (int i) { pedalDetail.showPedal (i); }),
      pedalDetail (p.apvts),
      ampPanel (p),
      cabPanel (p),
      master (p.apvts, [this] { pluckTick = 0; }),
      notes (p)
{
    setLookAndFeel (&sketchLF);

    titleLabel.setText ("Groove Sketchbook", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (30.f)));
    companyLabel.setText ("by Hoonaar Audio  -  sketchbook vol.1", juce::dontSendNotification);
    companyLabel.setFont (juce::Font (juce::FontOptions (13.f)));
    companyLabel.setColour (juce::Label::textColourId, SketchColours::pencilSoft);
    presetNameLabel.setJustificationType (juce::Justification::centredRight);
    presetNameLabel.setFont (juce::Font (juce::FontOptions (24.f)));
    presetNameLabel.setColour (juce::Label::textColourId, SketchColours::red);
    presetSubLabel.setJustificationType (juce::Justification::centredRight);
    presetSubLabel.setFont (juce::Font (juce::FontOptions (12.5f)));
    presetSubLabel.setColour (juce::Label::textColourId, SketchColours::pencilSoft);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (companyLabel);
    addAndMakeVisible (presetNameLabel);
    addAndMakeVisible (presetSubLabel);
    addAndMakeVisible (presetStrip);
    addAndMakeVisible (pedalBoard);
    addAndMakeVisible (pedalDetail);
    addAndMakeVisible (ampPanel);
    addAndMakeVisible (cabPanel);
    addAndMakeVisible (master);
    addAndMakeVisible (notes);

    setSize (1120, 740);
    setResizable (true, true);
    setResizeLimits (920, 620, 1600, 1100);

    refreshPresetLabels();
    startTimerHz (30);
    CrashLog::write ("editor constructed");
}

GrooveSketchbookEditor::~GrooveSketchbookEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void GrooveSketchbookEditor::paint (juce::Graphics& g)
{
    static std::atomic<bool> paintLogged { false };
    if (! paintLogged.exchange (true))
        CrashLog::write ("first editor paint");
    Sketch::drawPaper (g, getLocalBounds());
}

void GrooveSketchbookEditor::resized()
{
    auto r = getLocalBounds().reduced (8);

    auto header = r.removeFromTop (62);
    auto left = header.removeFromLeft (480);
    titleLabel.setBounds (left.removeFromTop (40));
    companyLabel.setBounds (left);
    auto right = header.removeFromRight (440);
    presetNameLabel.setBounds (right.removeFromTop (34));
    presetSubLabel.setBounds (right);

    presetStrip.setBounds (r.removeFromTop (64).reduced (2));
    pedalBoard.setBounds (r.removeFromTop (94).reduced (2));

    auto main = r.removeFromTop (292);
    pedalDetail.setBounds (main.removeFromLeft (292).reduced (2));
    ampPanel.setBounds (main.removeFromLeft (430).reduced (2));
    cabPanel.setBounds (main.reduced (2));

    master.setBounds (r.removeFromLeft (392).reduced (2));
    notes.setBounds (r.reduced (2));
}

void GrooveSketchbookEditor::refreshPresetLabels()
{
    const auto& pr = proc.getFactoryPreset (proc.getCurrentPreset());
    presetNameLabel.setText (S (pr.label), juce::dontSendNotification);
    presetSubLabel.setText (S (pr.sub), juce::dontSendNotification);
    lastPresetShown = proc.getCurrentPreset();
}

void GrooveSketchbookEditor::timerCallback()
{
    presetStrip.refresh();
    ampPanel.refresh();
    cabPanel.refresh();
    notes.refresh();

    if (proc.getCurrentPreset() != lastPresetShown)
        refreshPresetLabels();

    // Meter: freeze detection via block counter (host may suspend idle plugins)
    const int bc = proc.getBlockCounter();
    float target = (bc == lastBlockCount) ? 0.f : proc.getOutputLevel();
    lastBlockCount = bc;

    if (pluckTick >= 0)
    {
        ++pluckTick;
        const float t = (float) pluckTick;
        target = juce::jmax (target, std::abs (std::sin (t * 0.9f)) * std::exp (-t * 0.12f));
        if (pluckTick > 40)
            pluckTick = -1;
    }

    meterDisplay = juce::jmax (target, meterDisplay * 0.90f);
    lastTarget = target;
    master.meter.setLevel (meterDisplay);
    master.meter.repaint();
}
