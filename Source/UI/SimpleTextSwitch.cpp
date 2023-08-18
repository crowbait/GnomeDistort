/*
  ==============================================================================

    SimpleTextSwitch.cpp
    Created: 18 Aug 2023 12:53:17am
    Author:  traxx

  ==============================================================================
*/

#include "SimpleTextSwitch.h"

void LookAndFeelSimpleTextSwitch::drawToggleButton(juce::Graphics& g, juce::ToggleButton& toggleButton, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
    using namespace juce;

    if (auto* tggl = dynamic_cast<SimpleTextSwitch*>(&toggleButton)) {
        g.setColour(tggl->getColor(tggl->getToggleState()));
        g.setFont(juce::Font(tggl->getTextHeight(), juce::Font::FontStyleFlags::bold));
        g.drawFittedText(tggl->getText(tggl->getToggleState()), tggl->getLocalBounds(), juce::Justification::centred, 1);
    } else jassertfalse;
}
