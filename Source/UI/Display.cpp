/*
  ==============================================================================

    Display.cpp
    Created: 18 Sep 2023 4:55:35pm
    Author:  traxx

  ==============================================================================
*/

#include "Display.h"

Display::Display(GnomeDistortAudioProcessor& p) : audioProcessor(p), DisplayComp(p), Graph(p) {
    for (auto* comp : getComponents()) {
        addAndMakeVisible(comp);
    }
}
Display::~Display() {}

void Display::resized() {
    using namespace juce;

    Rectangle<int> bounds = getLocalBounds();
    Graph.setBounds(bounds.removeFromRight(bounds.getHeight()));
    DisplayComp.setBounds(bounds);
}

void Display::paint(juce::Graphics& g) {
    g.setColour(COLOR_BG_VERYDARK);
    g.fillAll();
}

std::vector<juce::Component*> Display::getComponents() {
    return {
        &DisplayComp,
        &Graph
    };
}
