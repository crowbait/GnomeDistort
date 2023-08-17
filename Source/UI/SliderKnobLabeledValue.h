/*
  ==============================================================================

    SliderKnobLabeledValue.h
    Created: 17 Aug 2023 3:21:12pm
    Author:  traxx

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Colors.h"

struct LookAndFeelSliderLabeledValues : juce::LookAndFeel_V4 {
    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle, float rotaryEndAngle, juce::Slider&);
};

struct SliderKnobLabeledValues : juce::Slider {
    SliderKnobLabeledValues(juce::RangedAudioParameter& rangedParam, const bool smallValue, const juce::String& label, const bool showDecimals, juce::Image& knobOverlay) :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(&rangedParam), smallValueText(smallValue), labelText(label), decimals(showDecimals), overlay(knobOverlay) {
        setLookAndFeel(&LNF);
    }

    ~SliderKnobLabeledValues() {
        setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds(juce::Rectangle<int>& bounds) const;
    int getTextHeight() const { if (getIsSmallText()) return 8; return 12; }
    bool getIsSmallText() const { return smallValueText; };
    bool getDecimals() const { return decimals; };
    juce::String getDisplayString() const;
    juce::String getLabelString() const { return labelText; };
    juce::Image& overlay;

private:
    LookAndFeelSliderLabeledValues LNF;
    juce::RangedAudioParameter* param;
    bool smallValueText;
    bool decimals;
    juce::String labelText;
};
