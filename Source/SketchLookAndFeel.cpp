#include "SketchLookAndFeel.h"
#include "BinaryData.h"
#include "ParamIDs.h"
#include "CrashLog.h"

#include <cmath>

namespace Sketch
{
    void drawPaper (juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        using namespace SketchColours;
        g.fillAll (paper);

        // Dot grid
        g.setColour (pencil.withAlpha (0.10f));
        for (int yy = 12; yy < bounds.getHeight(); yy += 22)
            for (int xx = 12; xx < bounds.getWidth(); xx += 22)
                g.fillRect ((float) xx, (float) yy, 1.5f, 1.5f);

        // Paper speckle (fixed seed => stable across repaints)
        juce::Random rng (1234);
        g.setColour (pencil.withAlpha (0.045f));
        for (int i = 0; i < 350; ++i)
            g.fillRect (rng.nextFloat() * bounds.getWidth(),
                        rng.nextFloat() * bounds.getHeight(), 1.3f, 1.3f);

        // Red margin line + notebook holes
        g.setColour (juce::Colour (0x66be5a50));
        g.fillRect (56, 0, 2, bounds.getHeight());
        g.setColour (juce::Colour (0xff3a3835));
        for (int yy = 60; yy < bounds.getHeight(); yy += 130)
            g.fillEllipse (14.f, (float) yy, 12.f, 12.f);
    }

    static juce::uint32 hashSeed (int seed)
    {
        return (juce::uint32) (seed * 1664525 + 1013904223);
    }

    void drawSketchLine (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b,
                         int seed, juce::Colour colour, float thickness)
    {
        juce::Random rng (hashSeed (seed));
        const float dx = b.x - a.x, dy = b.y - a.y;
        const float len = std::sqrt (dx * dx + dy * dy) + 0.001f;
        const float nx = -dy / len, ny = dx / len;

        g.setColour (colour.withAlpha (0.92f));
        juce::Point<float> prev = a;
        const int segs = 6;
        for (int i = 1; i <= segs; ++i)
        {
            const float f = (float) i / (float) segs;
            juce::Point<float> pt (a.x + dx * f, a.y + dy * f);
            if (i < segs)
            {
                const float off = (rng.nextFloat() - 0.5f) * 2.4f;
                pt.x += nx * off;
                pt.y += ny * off;
            }
            g.drawLine (prev.x, prev.y, pt.x, pt.y, thickness);
            prev = pt;
        }
        // Faint pencil echo
        g.setColour (colour.withAlpha (0.22f));
        g.drawLine (a.x + 0.9f, a.y - 0.7f, b.x + 0.9f, b.y - 0.7f, 1.0f);
    }

    void drawSketchRect (juce::Graphics& g, juce::Rectangle<float> r, int seed,
                         juce::Colour colour, float thickness)
    {
        const auto x = r.getX(), y = r.getY(), w = r.getWidth(), h = r.getHeight();
        drawSketchLine (g, { x, y }, { x + w, y }, seed + 1, colour, thickness);
        drawSketchLine (g, { x + w, y }, { x + w, y + h }, seed + 2, colour, thickness);
        drawSketchLine (g, { x + w, y + h }, { x, y + h }, seed + 3, colour, thickness);
        drawSketchLine (g, { x, y + h }, { x, y }, seed + 4, colour, thickness);
    }

    void drawSketchEllipse (juce::Graphics& g, juce::Rectangle<float> r, int seed,
                            juce::Colour colour, float thickness)
    {
        juce::Random rng (hashSeed (seed));
        const auto c = r.getCentre();
        const float rx = r.getWidth() * 0.5f, ry = r.getHeight() * 0.5f;
        g.setColour (colour.withAlpha (0.92f));
        juce::Point<float> prev (c.x + rx, c.y);
        const int segs = 28;
        for (int i = 1; i <= segs; ++i)
        {
            const float a = (float) i / (float) segs * 6.2831853f;
            const float jr = 1.f + (rng.nextFloat() - 0.5f) * 0.035f;
            juce::Point<float> pt (c.x + std::cos (a) * rx * jr,
                                   c.y + std::sin (a) * ry * jr);
            g.drawLine (prev.x, prev.y, pt.x, pt.y, thickness);
            prev = pt;
        }
    }

    juce::String formatParamValue (const juce::String& paramID, float v01)
    {
        using namespace ParamIDs;
        v01 = juce::jlimit (0.f, 1.f, v01);

        if (paramID == inGain)  return juce::String ((int) std::round (v01 * 24.f - 12.f)) + " dB";
        if (paramID == outGain) return juce::String ((int) std::round (v01 * 54.f - 48.f)) + " dB";
        if (paramID == micDist) return juce::String ((int) std::round (v01 * 30.f)) + " cm";
        if (paramID == micAngle) return juce::String ((int) std::round (v01 * 90.f - 45.f)) + " deg";

        const bool pct = (paramID == diMix || paramID == roomAmt || paramID == filtSens
                       || paramID == filtQ || paramID.endsWith ("mix") || paramID.endsWith ("lvl")
                       || paramID.endsWith ("dry") || paramID.endsWith ("sub")
                       || paramID.endsWith ("up") || paramID.endsWith ("depth")
                       || paramID.endsWith ("fb"));
        if (pct)
            return juce::String ((int) std::round (v01 * 100.f)) + "%";

        return juce::String (v01 * 10.f, 1);
    }
}

//==============================================================================
SketchLookAndFeel::SketchLookAndFeel()
{
    using namespace SketchColours;
    setColour (juce::TextButton::textColourOnId, paper);
    setColour (juce::TextButton::textColourOffId, pencil);
    setColour (juce::Label::textColourId, pencil);
    setColour (juce::Slider::textBoxTextColourId, pencil);
    setColour (juce::TextEditor::textColourId, pencil);

       handTypeface = juce::Typeface::createSystemTypefaceFor (BinaryData::CaveatRegular_ttf,
                                                            (size_t) BinaryData::CaveatRegular_ttfSize);
    const bool customFontOk = (handTypeface != nullptr);
    CrashLog::write ("font load attempted");
    if (! customFontOk)
        handTypeface = juce::Typeface::createSystemTypefaceFor (
            juce::Font (juce::FontOptions ("Segoe Print", 20.f, juce::Font::plain)));
    CrashLog::write (customFontOk ? "font: Caveat loaded" : "font: system fallback");
}

juce::Typeface::Ptr SketchLookAndFeel::getTypefaceForFont (const juce::Font& font)
{
    if (handTypeface != nullptr)
        return handTypeface;

    return LookAndFeel_V4::getTypefaceForFont (font);
}

juce::Font SketchLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return juce::Font (juce::FontOptions (juce::jlimit (10.f, 19.f, (float) buttonHeight * 0.40f)));
}

void SketchLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    using namespace SketchColours;
    const juce::Rectangle<float> bounds ((float) x, (float) y, (float) width, (float) height);
    const float dialSize = juce::jmax (20.f, juce::jmin (bounds.getWidth() - 6.f,
                                                        bounds.getHeight() - 32.f));
    const juce::Rectangle<float> dial (bounds.getCentreX() - dialSize * 0.5f,
                                       bounds.getY() + 2.f, dialSize, dialSize);
    const int seed = (int) (dial.getCentreX() * 13 + dial.getCentreY() * 7 + dialSize);

    g.setColour (juce::Colour (0x99ffffff));
    g.fillEllipse (dial.reduced (2.f));
    Sketch::drawSketchEllipse (g, dial.reduced (2.f), seed, pencil, 2.5f);

    const float angle = rotaryStartAngle
                      + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const auto c = dial.getCentre();
    const float r = dialSize * 0.5f - 7.f;
    const juce::Point<float> tip (c.x + std::sin (angle) * r,
                                  c.y - std::cos (angle) * r);
    g.setColour (pencil);
    g.drawLine (c.x, c.y, tip.x, tip.y, 3.f);
    g.setColour (red);
    g.fillEllipse (tip.x - 3.f, tip.y - 3.f, 6.f, 6.f);

    g.setColour (pencil);
    g.setFont (12.5f);
    g.drawText (slider.getName(), bounds.getX(), dial.getBottom() + 1.f,
                bounds.getWidth(), 14.f, juce::Justification::centred);

    const juce::String pid = slider.getProperties()
                                   .getWithDefault ("paramID", juce::String()).toString();
    g.setColour (blue);
    g.setFont (12.f);
    g.drawText (Sketch::formatParamValue (pid, sliderPosProportional),
                bounds.getX(), dial.getBottom() + 15.f,
                bounds.getWidth(), 14.f, juce::Justification::centred);
}

void SketchLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float, float,
                                         juce::Slider::SliderStyle, juce::Slider& slider)
{
    using namespace SketchColours;
    const juce::Rectangle<float> area ((float) x, (float) y, (float) width, (float) height);
    const int seed = x * 13 + y * 7 + width;

    g.setColour (pencil);
    g.setFont (12.f);
    g.drawText (slider.getName(), area.getX(), area.getY(),
                area.getWidth() * 0.5f, 14.f, juce::Justification::centredLeft);

    const juce::String pid = slider.getProperties()
                                   .getWithDefault ("paramID", juce::String()).toString();
    // Normalised value back-calculated from thumb position for display
    const float v01 = juce::jlimit (0.f, 1.f, (float) slider.getValue());
    g.setColour (blue);
    g.drawText (Sketch::formatParamValue (pid, v01),
                area.getX() + area.getWidth() * 0.5f, area.getY(),
                area.getWidth() * 0.5f, 14.f, juce::Justification::centredRight);

    const float trackY = area.getY() + 26.f;
    Sketch::drawSketchLine (g, { area.getX() + 9.f, trackY },
                            { area.getRight() - 9.f, trackY }, seed, pencil, 2.f);

    const juce::Rectangle<float> thumb (sliderPos - 9.f, trackY - 9.f, 18.f, 18.f);
    g.setColour (paper);
    g.fillEllipse (thumb);
    Sketch::drawSketchEllipse (g, thumb, seed + 5, pencil, 2.5f);
}

void SketchLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                             const juce::Colour&, bool, bool)
{
    using namespace SketchColours;
    const auto r = button.getLocalBounds().toFloat().reduced (1.5f);
    const int seed = button.getX() * 13 + button.getY() * 7 + button.getWidth() * 3;
    const bool on = button.getToggleState();
    const bool accent = ((int) button.getProperties().getWithDefault ("accent", 0)) == 1;

    g.setColour (on ? pencil : (accent ? red : juce::Colour (0x55ffffff)));
    g.fillRect (r);
    Sketch::drawSketchRect (g, r, seed, on ? red : pencil, on ? 2.5f : 2.f);

    const bool showLed = ((int) button.getProperties().getWithDefault ("led", 0)) == 1;
    if (showLed)
    {
        const juce::Rectangle<float> led (r.getRight() - 15.f, r.getY() + 5.f, 10.f, 10.f);
        g.setColour (on ? red : pencilFaint.withAlpha (0.5f));
        g.fillEllipse (led);
        g.setColour (pencil);
        g.drawEllipse (led, 1.5f);
    }
}

void SketchLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                         bool, bool)
{
    using namespace SketchColours;
    const auto b = button.getLocalBounds().toFloat();
    const juce::Rectangle<float> box (6.f, b.getCentreY() - 8.f, 16.f, 16.f);
    g.setColour (juce::Colour (0x55ffffff));
    g.fillRect (box);
    Sketch::drawSketchRect (g, box, 9, pencil, 2.f);
    if (button.getToggleState())
        Sketch::drawSketchLine (g, { box.getX() + 3.f, box.getY() + 3.f },
                                { box.getRight() - 3.f, box.getBottom() - 3.f }, 10, red, 3.f);

    g.setColour (pencil);
    g.setFont (14.f);
    g.drawText (button.getButtonText(), 30.f, 0.f, b.getWidth() - 32.f, b.getHeight(),
                juce::Justification::centredLeft);
}
