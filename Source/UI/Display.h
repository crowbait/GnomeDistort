/*
  ==============================================================================

    Display.h
    Created: 18 Sep 2023 4:55:44pm
    Author:  traxx

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "GlobalConsts.h"
#include "DisplayComponent.h"

struct Display : juce::Component {
    Display(GnomeDistortAudioProcessor&);
    ~Display();

    DisplayComponent DisplayComp;

    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    GnomeDistortAudioProcessor& audioProcessor;

    std::vector<juce::Component*> getComponents();
};
