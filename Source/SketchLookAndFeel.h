#pragma once

//==============================================================================
// Groove Sketchbook — pencil-on-paper LookAndFeel + sketch drawing helpers.
//==============================================================================
#include <juce_gui_basics/juce_gui_basics.h>

namespace SketchColours
{
    inline const juce::Colour paper      (0xfff5f0e1);
    inline const juce::Colour paperDark  (0xffefe7d3);
    inline const juce::Colour pencil     (0xff2e2c28);
    inline const juce::Colour pencilSoft (0xff5a564d);
    inline const juce::Colour pencilFaint(0xff8a8578);
    inline const juce::Colour red        (0xffb3402e);
    inline const juce::Colour blue       (0xff2c5da0);
    inline const juce::Colour tape       (0x8cd8bc82);
}

namespace Sketch
{
    void drawPaper (juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawSketchLine (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b,
                         int seed, juce::Colour colour, float thickness = 2.0f);
    void drawSketchRect (juce::Graphics& g, juce::Rectangle<float> r, int seed,
                         juce::Colour colour, float thickness = 2.0f);
    void drawSketchEllipse (juce::Graphics& g, juce::Rectangle<float> r, int seed,
                            juce::Colour colour, float thickness = 2.0f);
    juce::String formatParamValue (const juce::String& paramID, float v01);
}

class SketchLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SketchLookAndFeel();

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override;
    juce::Font getTextButtonFont (juce::TextButton& button, int buttonHeight) override;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override;
    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

private:
    juce::Typeface::Ptr handTypeface;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SketchLookAndFeel)
};
