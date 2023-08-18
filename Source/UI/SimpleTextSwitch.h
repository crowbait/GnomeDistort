/*
  ==============================================================================

    SimpleTextSwitch.h
    Created: 18 Aug 2023 12:53:07am
    Author:  traxx

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GlobalConsts.h"

struct LookAndFeelSimpleTextSwitch : juce::LookAndFeel_V4 {
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& toggleButton, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

struct SimpleTextSwitch : juce::ToggleButton {
    SimpleTextSwitch(juce::RangedAudioParameter& parameter, const bool small, const juce::String& labelOn, const juce::Colour& colOn, const juce::Colour& colOff) :
        juce::ToggleButton(), param(&parameter), smallText(small), textOn(labelOn), textOff(labelOn), colorOn(colOn), colorOff(colOff) {
        setLookAndFeel(&LNF);
    }
    SimpleTextSwitch(juce::RangedAudioParameter& parameter, const bool small, const juce::String& labelOn, const juce::String& labelOff, const juce::Colour& colOn, const juce::Colour& colOff) :
        juce::ToggleButton(), param(&parameter), smallText(small), textOn(labelOn), textOff(labelOff), colorOn(colOn), colorOff(colOff) {
        setLookAndFeel(&LNF);
    }

    ~SimpleTextSwitch() {
        setLookAndFeel(nullptr);
    }

    int getTextHeight() const { if (smallText) return TEXT_SMALL; return TEXT_NORMAL; }
    bool getIsSmallText() const { return smallText; };
    juce::String getText(bool state) const { return state ? textOn : textOff; };
    void setOnText(juce::String newStr) { textOn = newStr; }
    void setOffText(juce::String newStr) { textOff = newStr; }
    juce::Colour getColor(bool state) const { return state ? colorOn : colorOff; };

private:
    LookAndFeelSimpleTextSwitch LNF;
    juce::RangedAudioParameter* param;
    bool smallText;
    juce::String textOn, textOff;
    juce::Colour colorOn, colorOff;
};
