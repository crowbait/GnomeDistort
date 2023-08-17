/*
  ==============================================================================

    SliderKnobLabeledValue.cpp
    Created: 17 Aug 2023 3:22:49pm
    Author:  traxx

  ==============================================================================
*/

#include "SliderKnobLabeledValue.h"

void LookAndFeelSliderLabeledValues::drawRotarySlider(juce::Graphics& g,
                                               int x, int y, int width, int height,
                                               float sliderPosProportional,
                                               float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) {
    using namespace juce;
    auto bounds = Rectangle<float>(x, y, width, height);
    auto center = bounds.getCentre();

    // DEBUG: BOUNDS
    // g.setColour(Colours::blue);
    // g.drawRect(bounds.toFloat(), 1);

    // knob body
    g.setColour(COLOR_KNOB);
    g.fillEllipse(bounds);

    if (auto* sldr = dynamic_cast<SliderKnobLabeledValues*>(&slider)) {
        // overlay
        g.drawImage(sldr->overlay, bounds, RectanglePlacement::stretchToFit);

        bool isSmall = sldr->getIsSmallText();
        int textHeight = sldr->getTextHeight();

        // label box
        Rectangle<float> labelboxRect;
        Path labelboxP;
        String labeltext = sldr->getLabelString();
        g.setFont(textHeight);
        labelboxRect.setSize(g.getCurrentFont().getStringWidth(labeltext) + (isSmall ? 2 : 4), sldr->getTextHeight() + (isSmall ? 1 : 2));
        labelboxRect.setCentre(center.getX(), center.getY() - bounds.getHeight() * 0.15f);
        labelboxP.addRoundedRectangle(labelboxRect, 2.f);
        g.setColour(Colours::darkgrey);
        g.strokePath(labelboxP, PathStrokeType(1));
        g.setColour(COLOR_BG_VERYDARK);
        g.fillPath(labelboxP);

        g.setColour(Colours::lightgrey);
        g.drawFittedText(labeltext, labelboxRect.toNearestInt(), Justification::centred, 1);

        // value box
        Rectangle<float> valueboxRect;
        Path valueboxP;
        String valuetext = sldr->getDisplayString();
        auto strWidthValue = g.getCurrentFont().getStringWidth("12345"); // replace with variable "text" for accuracy, 5 digits for all-same boxes
        valueboxRect.setSize(strWidthValue + (isSmall ? 2 : 4), sldr->getTextHeight() + (isSmall ? 1 : 2));
        valueboxRect.setCentre(center.getX(), center.getY() + bounds.getHeight() * 0.25f);
        valueboxP.addRoundedRectangle(valueboxRect, 2.f);
        g.setColour(Colours::darkgrey);
        g.strokePath(valueboxP, PathStrokeType(1));
        g.setColour(COLOR_BG_VERYDARK);
        g.fillPath(valueboxP);

        g.setColour(Colours::lightgrey);
        g.drawFittedText(valuetext, valueboxRect.toNearestInt(), Justification::centred, 1);
    } else jassertfalse;

    // indicator
    Rectangle<float> indicatorRect;
    Path indicatorP;
    indicatorRect.setLeft(center.getX() - 2);
    indicatorRect.setRight(center.getX() + 2);
    indicatorRect.setTop(bounds.getY());
    indicatorRect.setBottom(center.getY());
    indicatorP.addRectangle(indicatorRect);
    jassert(rotaryStartAngle < rotaryEndAngle);
    auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
    indicatorP.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));
    g.setColour(Colours::lightgrey);
    g.fillPath(indicatorP);
}


//==============================================================================
//==============================================================================
//==============================================================================


void SliderKnobLabeledValues::paint(juce::Graphics& g) {
    using namespace juce;

    Rectangle<int> bounds = getLocalBounds();
    auto minAngle = degreesToRadians(-135.f);
    auto maxAngle = degreesToRadians(135.f);
    auto range = getRange();

    auto sliderBounds = getSliderBounds(bounds);
    getLookAndFeel().drawRotarySlider(
        g, sliderBounds.getX(), sliderBounds.getY(), sliderBounds.getWidth(), sliderBounds.getHeight(),
        valueToProportionOfLength(getValue()), // jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),   // normalize
        minAngle, maxAngle,
        *this
    );
}

juce::Rectangle<int> SliderKnobLabeledValues::getSliderBounds(juce::Rectangle<int>& bounds) const {
    auto size = juce::jmin(bounds.getWidth() - 12, bounds.getHeight() - 12);
    juce::Rectangle<int> r;
    r.setSize(size, size);
    int textHeight = getTextHeight();
    r.setCentre(bounds.getCentreX(), bounds.getCentreY());
    return r;
}

juce::String SliderKnobLabeledValues::getDisplayString() const {
    juce::String str;
    return juce::String((float)getValue(), getDecimals() ? 2 : 0);
}
