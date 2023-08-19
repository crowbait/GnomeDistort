/*
  ==============================================================================

    SimpleTextButton.h
    Created: 19 Aug 2023 9:17:05pm
    Author:  traxx

  ==============================================================================
*/

#pragma once
#include "GlobalConsts.h"

struct SimpleTextButton : juce::TextButton {
    SimpleTextButton(const juce::String& label, const juce::Colour& col, const bool small, const bool bold) :
        text(label), color(col), smallText(small), isBold(bold) {
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
        g.setColour(color);
        g.setFont(juce::Font(getTextHeight(), isBold ? juce::Font::FontStyleFlags::bold : juce::Font::FontStyleFlags::plain));
        g.drawFittedText(text, getLocalBounds(), juce::Justification::centred, 1);
    }

    int getTextHeight() const { if (smallText) return TEXT_SMALL; return TEXT_NORMAL; }
    bool getIsSmallText() const { return smallText; };
    bool getIsBold() const { return isBold; };
    juce::String getText(bool state) const { return text; };
    void setText(juce::String newStr) { text = newStr; }
    juce::Colour getColor() const { return color; };
    void setColor(juce::Colour newCol) { color = newCol; }

private:
    bool smallText;
    bool isBold;
    juce::String text;
    juce::Colour color;
};
