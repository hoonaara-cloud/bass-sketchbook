#include "Panels.h"
#include "PluginProcessor.h"
#include "SketchLookAndFeel.h"

#include <cmath>

namespace
{
    juce::String S (const std::string& s) { return juce::String (s.c_str()); }
}

//==============================================================================
Knob::Knob (const juce::String& paramID, const juce::String& label)
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setName (label);
    getProperties().set ("paramID", paramID);
    setDoubleClickReturnValue (true, 0.5);
}

//==============================================================================
SketchPanel::SketchPanel (int seedIn) : seed (seedIn) {}

void SketchPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.f);
    g.setColour (juce::Colour (0x55ffffff));
    g.fillRect (r);
    Sketch::drawSketchRect (g, r, seed, SketchColours::pencil, 2.f);
}

//==============================================================================
PresetStrip::PresetStrip (GrooveSketchbookProcessor& p) : SketchPanel (7), proc (p)
{
    const auto presets = getFactoryPresets();
    for (size_t i = 0; i < presets.size(); ++i)
    {
        auto* b = new juce::TextButton (S (presets[i].label));
        b->setClickingTogglesState (true);
        b->setRadioGroupId (101);
        b->setTooltip (S (presets[i].sub));
        const int idx = (int) i;
        b->onClick = [this, idx] { proc.loadPreset (idx); refresh(); };
        buttons.add (b);
        addAndMakeVisible (b);
    }
    refresh();
}

void PresetStrip::resized()
{
    auto r = getLocalBounds().reduced (10);
    const int n = buttons.size();
    for (int i = 0; i < n; ++i)
        buttons[i]->setBounds (r.removeFromLeft (r.getWidth() / (n - i)).reduced (4));
}

void PresetStrip::refresh()
{
    const int cur = proc.getCurrentPreset();
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setToggleState (i == cur, juce::dontSendNotification);
}

//==============================================================================
PedalBoard::PedalBoard (APVTS& s, std::function<void (int)> cb)
    : SketchPanel (21), apvts (s), onSelect (std::move (cb)), metas (getPedalMetas())
{
    inLabel.setText ("BASS IN", juce::dontSendNotification);
    inLabel.setJustificationType (juce::Justification::centred);
    inLabel.setFont (juce::Font (juce::FontOptions (13.f)));
    ampLabel.setText ("AMP", juce::dontSendNotification);
    ampLabel.setJustificationType (juce::Justification::centred);
    ampLabel.setFont (juce::Font (juce::FontOptions (14.f)));
    cabLabel.setText ("CAB", juce::dontSendNotification);
    cabLabel.setJustificationType (juce::Justification::centred);
    cabLabel.setFont (juce::Font (juce::FontOptions (14.f)));
    addAndMakeVisible (inLabel);
    addAndMakeVisible (ampLabel);
    addAndMakeVisible (cabLabel);

    for (size_t i = 0; i < metas.size(); ++i)
    {
        const auto& m = metas[i];
        auto* b = new juce::TextButton (S (m.shortName));
        b->setClickingTogglesState (true);
        b->getProperties().set ("led", 1);
        b->setTooltip (S (m.fullName) + " - click to stomp + inspect");
        const int idx = (int) i;
        b->onClick = [this, idx]
        {
            selectPedal (idx);
            if (onSelect)
                onSelect (idx);
        };
        addAndMakeVisible (b);
        attachments.push_back (std::make_unique<ButtonAttachment> (apvts, S (m.onParam), *b));
        pedalButtons.add (b);
    }
}

void PedalBoard::selectPedal (int i)
{
    selected = i;
    repaint();
}

void PedalBoard::resized()
{
    auto r = getLocalBounds().reduced (10);
    inLabel.setBounds (r.removeFromLeft (64).reduced (3));
    auto right = r.removeFromRight (150);
    ampLabel.setBounds (right.removeFromLeft (75).reduced (3));
    cabLabel.setBounds (right.reduced (3));

    const int n = pedalButtons.size();
    for (int i = 0; i < n; ++i)
        pedalButtons[i]->setBounds (r.removeFromLeft (r.getWidth() / (n - i)).reduced (3));
}

void PedalBoard::paint (juce::Graphics& g)
{
    SketchPanel::paint (g);

    // Pencil wires behind the nodes
    std::vector<juce::Point<float>> pts;
    pts.push_back (inLabel.getBounds().toFloat().getCentre());
    for (int i = 0; i < pedalButtons.size(); ++i)
        pts.push_back (pedalButtons[i]->getBounds().toFloat().getCentre());
    pts.push_back (ampLabel.getBounds().toFloat().getCentre());
    pts.push_back (cabLabel.getBounds().toFloat().getCentre());

    g.setColour (SketchColours::pencil.withAlpha (0.55f));
    for (size_t i = 1; i < pts.size(); ++i)
    {
        const auto a = pts[i - 1], b = pts[i];
        const int dashes = 9;
        for (int d = 0; d < dashes; ++d)
        {
            const float f0 = (float) d / (float) dashes;
            const float f1 = ((float) d + 0.55f) / (float) dashes;
            g.drawLine (a.x + (b.x - a.x) * f0, a.y + (b.y - a.y) * f0,
                        a.x + (b.x - a.x) * f1, a.y + (b.y - a.y) * f1, 2.f);
        }
    }

    if (selected >= 0 && selected < pedalButtons.size())
    {
        auto r = pedalButtons[selected]->getBounds().toFloat().expanded (3.f);
        Sketch::drawSketchRect (g, r, 99, SketchColours::red, 2.5f);
    }
}

//==============================================================================
PedalDetail::PedalDetail (APVTS& s) : SketchPanel (33), apvts (s), metas (getPedalMetas())
{
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (20.f)));
    hintLabel.setJustificationType (juce::Justification::centred);
    hintLabel.setFont (juce::Font (juce::FontOptions (12.f)));
    hintLabel.setColour (juce::Label::textColourId, SketchColours::pencilSoft);
    engageButton.setButtonText ("ENGAGED");
    engageButton.setClickingTogglesState (true);
    engageButton.getProperties().set ("led", 1);
    addAndMakeVisible (titleLabel);
    addAndMakeVisible (hintLabel);
    addAndMakeVisible (engageButton);
    showPedal (0);
}

void PedalDetail::showPedal (int index)
{
    index = juce::jlimit (0, (int) metas.size() - 1, index);
    shown = index;
    const auto& m = metas[(size_t) index];
    titleLabel.setText (S (m.fullName), juce::dontSendNotification);
    hintLabel.setText (S (m.hint), juce::dontSendNotification);

    knobs.clear();
    knobAttachments.clear();
    for (int k = 0; k < 3; ++k)
    {
        auto* knob = new Knob (S (m.knobParams[k]), S (m.knobLabels[k]));
        knobs.add (knob);
        addAndMakeVisible (knob);
        knobAttachments.push_back (
            std::make_unique<SliderAttachment> (apvts, S (m.knobParams[k]), *knob));
    }
    engageAttachment = std::make_unique<ButtonAttachment> (apvts, S (m.onParam), engageButton);
    resized();
}

void PedalDetail::resized()
{
    auto r = getLocalBounds().reduced (10);
    titleLabel.setBounds (r.removeFromTop (28));
    hintLabel.setBounds (r.removeFromTop (20));
    engageButton.setBounds (r.removeFromTop (36).withSizeKeepingCentre (150, 30));
    r.removeFromTop (4);
    const int n = knobs.size();
    for (int i = 0; i < n; ++i)
        knobs[i]->setBounds (r.removeFromLeft (r.getWidth() / juce::jmax (1, n - i)).reduced (2));
}

//==============================================================================
AmpPanel::AmpPanel (GrooveSketchbookProcessor& p) : SketchPanel (55), proc (p)
{
    titleLabel.setText ("AMP HEAD", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (19.f)));
    modelLabel.setJustificationType (juce::Justification::centred);
    modelLabel.setFont (juce::Font (juce::FontOptions (12.5f)));
    modelLabel.setColour (juce::Label::textColourId, SketchColours::blue);
    addAndMakeVisible (titleLabel);
    addAndMakeVisible (modelLabel);

    const auto ampKnobs = getAmpKnobs();
    for (size_t i = 0; i < ampKnobs.size(); ++i)
    {
        auto* knob = new Knob (S (ampKnobs[i].param), S (ampKnobs[i].label));
        knobs.add (knob);
        addAndMakeVisible (knob);
        attachments.push_back (
            std::make_unique<SliderAttachment> (proc.apvts, S (ampKnobs[i].param), *knob));
    }

    tubeButton.setButtonText ("TUBE");
    solidButton.setButtonText ("SOLID");
    w800Button.setButtonText ("800W");
    w300Button.setButtonText ("300W");
    tubeButton.setClickingTogglesState (true);
    solidButton.setClickingTogglesState (true);
    w800Button.setClickingTogglesState (true);
    w300Button.setClickingTogglesState (true);
    tubeButton.setRadioGroupId (102);
    solidButton.setRadioGroupId (102);
    w800Button.setRadioGroupId (103);
    w300Button.setRadioGroupId (103);
    tubeButton.onClick  = [this] { proc.setChoiceParam (ParamIDs::ampCircuit, 0, 2); refresh(); };
    solidButton.onClick = [this] { proc.setChoiceParam (ParamIDs::ampCircuit, 1, 2); refresh(); };
    w800Button.onClick  = [this] { proc.setChoiceParam (ParamIDs::ampPower, 0, 2); refresh(); };
    w300Button.onClick  = [this] { proc.setChoiceParam (ParamIDs::ampPower, 1, 2); refresh(); };
    addAndMakeVisible (tubeButton);
    addAndMakeVisible (solidButton);
    addAndMakeVisible (w800Button);
    addAndMakeVisible (w300Button);
    refresh();
}

void AmpPanel::refresh()
{
    const int c = (int) std::round (proc.getParam01 (ParamIDs::ampCircuit));
    const int w = (int) std::round (proc.getParam01 (ParamIDs::ampPower));
    tubeButton.setToggleState (c == 0, juce::dontSendNotification);
    solidButton.setToggleState (c == 1, juce::dontSendNotification);
    w800Button.setToggleState (w == 0, juce::dontSendNotification);
    w300Button.setToggleState (w == 1, juce::dontSendNotification);

    const auto& pr = proc.getFactoryPreset (proc.getCurrentPreset());
    modelLabel.setText (juce::String ("-- \"") + S (pr.ampLabel) + "\" draft",
                        juce::dontSendNotification);
}

void AmpPanel::resized()
{
    auto r = getLocalBounds().reduced (10);
    titleLabel.setBounds (r.removeFromTop (26));
    modelLabel.setBounds (r.removeFromTop (18));
    auto sw = r.removeFromBottom (36);
    const int bw = sw.getWidth() / 4;
    tubeButton.setBounds (sw.removeFromLeft (bw).reduced (3));
    solidButton.setBounds (sw.removeFromLeft (bw).reduced (3));
    w800Button.setBounds (sw.removeFromLeft (bw).reduced (3));
    w300Button.setBounds (sw.reduced (3));

    const int n = knobs.size();
    for (int i = 0; i < n; ++i)
        knobs[i]->setBounds (r.removeFromLeft (r.getWidth() / (n - i)).reduced (2));
}

//==============================================================================
MicDiagram::MicDiagram (GrooveSketchbookProcessor& p) : proc (p) {}

void MicDiagram::paint (juce::Graphics& g)
{
    using namespace SketchColours;
    const int cab = juce::jlimit (0, 2, (int) std::round (proc.getParam01 (ParamIDs::cabType) * 2.f));
    const int mic = juce::jlimit (0, 2, (int) std::round (proc.getParam01 (ParamIDs::micType) * 2.f));
    const float dist = proc.getParam01 (ParamIDs::micDist);
    const float angDeg = proc.getParam01 (ParamIDs::micAngle) * 90.f - 45.f;

    auto b = getLocalBounds().toFloat().reduced (4.f);

    // Cabinet box + speakers
    juce::Rectangle<float> cabR (b.getX(), b.getY(), 100.f, b.getHeight());
    g.setColour (juce::Colour (0x44ffffff));
    g.fillRect (cabR);
    Sketch::drawSketchRect (g, cabR, 11, pencil, 2.5f);
    for (int s = 0; s < 2; ++s)
    {
        const float cy = cabR.getY() + cabR.getHeight() * (s == 0 ? 0.28f : 0.68f);
        juce::Rectangle<float> sp (cabR.getCentreX() - 20.f, cy - 20.f, 40.f, 40.f);
        Sketch::drawSketchEllipse (g, sp, 20 + s, pencil, 2.f);
        g.setColour (pencil);
        g.drawEllipse (sp.reduced (13.f), 1.5f);
    }
    const auto cabNames = getCabNames();
    g.setColour (pencilSoft);
    g.setFont (11.f);
    g.drawText (S (cabNames[(size_t) cab]), cabR.getX(), cabR.getBottom() - 15.f,
                cabR.getWidth(), 13.f, juce::Justification::centred);

    // Mic position from distance + angle
    const float cabRight = cabR.getRight();
    const float avail = b.getRight() - cabRight - 52.f;
    const float mx = cabRight + 14.f + dist * juce::jmax (10.f, avail);
    float my = b.getCentreY() + std::tan (angDeg * 3.14159265f / 180.f) * 60.f;
    my = juce::jlimit (b.getY() + 24.f, b.getBottom() - 36.f, my);

    // Dashed distance line
    g.setColour (red.withAlpha (0.8f));
    const int dashes = 10;
    for (int d = 0; d < dashes; ++d)
    {
        const float f0 = (float) d / (float) dashes;
        const float f1 = ((float) d + 0.5f) / (float) dashes;
        g.drawLine (cabRight + 2.f + (mx - cabRight - 2.f) * f0, b.getCentreY(),
                    cabRight + 2.f + (mx - cabRight - 2.f) * f1, b.getCentreY(), 1.5f);
    }
    g.setColour (red);
    g.setFont (11.f);
    g.drawText (juce::String ((int) std::round (dist * 30.f)) + " cm",
                cabRight + 4.f, b.getCentreY() - 26.f, 70.f, 14.f,
                juce::Justification::centredLeft);

    // Angle connector + mic drawing
    g.setColour (blue.withAlpha (0.8f));
    g.drawLine (mx, b.getCentreY(), mx, my, 1.5f);

    juce::Rectangle<float> head (mx - 9.f, my - 22.f, 18.f, 26.f);
    g.setColour (paper);
    g.fillEllipse (head);
    Sketch::drawSketchEllipse (g, head, 31, pencil, 2.5f);
    g.setColour (pencil);
    g.drawLine (mx - 9.f, my - 13.f, mx + 9.f, my - 13.f, 1.f);
    g.drawLine (mx - 9.f, my - 7.f, mx + 9.f, my - 7.f, 1.f);
    g.drawLine (mx, my + 4.f, mx, my + 20.f, 2.f);
    g.drawLine (mx - 11.f, my + 20.f, mx + 11.f, my + 20.f, 2.f);

    const auto micNames = getMicNames();
    g.setColour (blue);
    g.setFont (12.5f);
    g.drawText (S (micNames[(size_t) mic]), mx + 12.f, my - 28.f, 64.f, 14.f,
                juce::Justification::centredLeft);
    g.setColour (pencilSoft);
    g.setFont (11.f);
    g.drawText (juce::String ((int) std::round (angDeg)) + " deg", mx + 12.f, my - 14.f,
                64.f, 14.f, juce::Justification::centredLeft);
}

//==============================================================================
CabMicPanel::CabMicPanel (GrooveSketchbookProcessor& p)
    : SketchPanel (77), proc (p), diagram (p)
{
    titleLabel.setText ("CAB + MIC", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (19.f)));
    addAndMakeVisible (titleLabel);

    const auto cabNames = getCabNames();
    for (size_t i = 0; i < cabNames.size(); ++i)
    {
        auto* b = new juce::TextButton (S (cabNames[i]));
        b->setClickingTogglesState (true);
        b->setRadioGroupId (104);
        const int idx = (int) i;
        b->onClick = [this, idx] { proc.setChoiceParam (ParamIDs::cabType, idx, 3); refresh(); };
        cabButtons.add (b);
        addAndMakeVisible (b);
    }
    const auto micNames = getMicNames();
    for (size_t i = 0; i < micNames.size(); ++i)
    {
        auto* b = new juce::TextButton (S (micNames[i]));
        b->setClickingTogglesState (true);
        b->setRadioGroupId (105);
        const int idx = (int) i;
        b->onClick = [this, idx] { proc.setChoiceParam (ParamIDs::micType, idx, 3); refresh(); };
        micButtons.add (b);
        addAndMakeVisible (b);
    }

    struct LinMeta { const char* id; const char* name; double dbl; };
    const LinMeta lins[] = { { ParamIDs::micDist, "DISTANCE", 0.12 },
                             { ParamIDs::micAngle, "ANGLE", 0.5 },
                             { ParamIDs::roomAmt, "ROOM", 0.25 } };
    for (auto& m : lins)
    {
        auto* s = new juce::Slider();
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s->setName (juce::String (m.name));
        s->getProperties().set ("paramID", juce::String (m.id));
        s->setDoubleClickReturnValue (true, m.dbl);
        sliders.add (s);
        addAndMakeVisible (s);
        attachments.push_back (
            std::make_unique<SliderAttachment> (proc.apvts, juce::String (m.id), *s));
    }

    addAndMakeVisible (diagram);
    refresh();
}

void CabMicPanel::refresh()
{
    const int cab = juce::jlimit (0, 2, (int) std::round (proc.getParam01 (ParamIDs::cabType) * 2.f));
    const int mic = juce::jlimit (0, 2, (int) std::round (proc.getParam01 (ParamIDs::micType) * 2.f));
    for (int i = 0; i < cabButtons.size(); ++i)
        cabButtons[i]->setToggleState (i == cab, juce::dontSendNotification);
    for (int i = 0; i < micButtons.size(); ++i)
        micButtons[i]->setToggleState (i == mic, juce::dontSendNotification);
    diagram.repaint();
}

void CabMicPanel::resized()
{
    auto r = getLocalBounds().reduced (10);
    titleLabel.setBounds (r.removeFromTop (26));

    auto cabRow = r.removeFromTop (32);
    for (int i = 0; i < cabButtons.size(); ++i)
        cabButtons[i]->setBounds (cabRow.removeFromLeft (cabRow.getWidth() / (cabButtons.size() - i)).reduced (3));

    auto micRow = r.removeFromTop (32);
    for (int i = 0; i < micButtons.size(); ++i)
        micButtons[i]->setBounds (micRow.removeFromLeft (micRow.getWidth() / (micButtons.size() - i)).reduced (3));

    auto sl = r.removeFromBottom (3 * 38);
    for (int i = 0; i < sliders.size(); ++i)
        sliders[i]->setBounds (sl.removeFromTop (38).reduced (2));

    diagram.setBounds (r.reduced (2));
}

//==============================================================================
NotesPanel::NotesPanel (GrooveSketchbookProcessor& p) : SketchPanel (91), proc (p)
{
    notes.setMultiLine (true);
    notes.setReadOnly (true);
    notes.setScrollbarsShown (false);
    notes.setCaretVisible (false);
    notes.setPopupMenuEnabled (false);
    notes.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    notes.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    notes.setColour (juce::TextEditor::textColourId, SketchColours::pencil);
    notes.setFont (juce::Font (juce::FontOptions (14.5f)));
    addAndMakeVisible (notes);
    refresh();
}

void NotesPanel::resized()
{
    notes.setBounds (getLocalBounds().reduced (14));
}

void NotesPanel::refresh()
{
    const int cur = proc.getCurrentPreset();
    if (cur == lastPreset)
        return;

    lastPreset = cur;
    notes.setText (S (proc.getFactoryPreset (cur).notes), false);
}

void NotesPanel::paint (juce::Graphics& g)
{
    SketchPanel::paint (g);

    // Washi tape on top
    auto b = getLocalBounds().toFloat();
    g.saveState();
    g.addTransform (juce::AffineTransform::rotation (-0.05f, b.getCentreX(), 14.f));
    g.setColour (SketchColours::tape);
    g.fillRect (b.getCentreX() - 45.f, 2.f, 90.f, 22.f);
    g.restoreState();
}

//==============================================================================
void MeterComp::paint (juce::Graphics& g)
{
    using namespace SketchColours;
    shown += (level - shown) * 0.45f;

    auto b = getLocalBounds().toFloat().reduced (4.f);
    g.setColour (juce::Colour (0x66ffffff));
    g.fillRect (b);
    Sketch::drawSketchRect (g, b, 41, pencil, 2.f);

    const juce::Point<float> pivot (b.getCentreX(), b.getBottom() - 12.f);
    const float len = juce::jmin (b.getWidth(), b.getHeight()) * 0.42f;

    g.setColour (pencilSoft);
    for (int i = 0; i <= 8; ++i)
    {
        const float a = -1.1f + (float) i / 8.f * 2.2f;
        g.drawLine (pivot.x + std::sin (a) * (len - 6.f), pivot.y - std::cos (a) * (len - 6.f),
                    pivot.x + std::sin (a) * len, pivot.y - std::cos (a) * len, 1.5f);
    }

    const float a = -1.1f + juce::jlimit (0.f, 1.f, shown) * 2.2f;
    g.setColour (red);
    g.drawLine (pivot.x, pivot.y,
                pivot.x + std::sin (a) * len, pivot.y - std::cos (a) * len, 3.f);
    g.setColour (pencil);
    g.fillEllipse (pivot.x - 3.f, pivot.y - 3.f, 6.f, 6.f);

    g.setColour (pencilSoft);
    g.setFont (10.f);
    g.drawText ("GROOVE", b.getX(), b.getBottom() - 12.f, b.getWidth(), 11.f,
                juce::Justification::centred);
}

//==============================================================================
MasterSection::MasterSection (APVTS& s, std::function<void()> onPluck)
    : SketchPanel (111)
{
    struct M { const char* id; const char* label; };
    const M ms[] = { { ParamIDs::inGain, "INPUT" },
                     { ParamIDs::diMix, "MIX" },
                     { ParamIDs::outGain, "OUTPUT" } };
    for (auto& m : ms)
    {
        auto* knob = new Knob (juce::String (m.id), juce::String (m.label));
        knobs.add (knob);
        addAndMakeVisible (knob);
        attachments.push_back (
            std::make_unique<SliderAttachment> (s, juce::String (m.id), *knob));
    }

    pluckButton.setButtonText ("PLUCK!");
    pluckButton.getProperties().set ("accent", 1);
    pluckButton.setColour (juce::TextButton::textColourOffId, SketchColours::paper);
    pluckButton.onClick = std::move (onPluck);
    addAndMakeVisible (pluckButton);
    addAndMakeVisible (meter);
}

void MasterSection::resized()
{
    auto r = getLocalBounds().reduced (10);
    const int n = knobs.size();
    for (int i = 0; i < n; ++i)
        knobs[i]->setBounds (r.removeFromLeft (86).reduced (2));

    pluckButton.setBounds (r.removeFromTop (44).reduced (4));
    meter.setBounds (r.reduced (2));
}
