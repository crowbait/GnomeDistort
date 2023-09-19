/*
  ==============================================================================

    DisplayGraph.h
    Created: 18 Sep 2023 5:38:26pm
    Author:  traxx

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../Helpers/FFTDataGenerator.h"
#include "GlobalConsts.h"

struct DisplayGraph : juce::Component, juce::AudioProcessorParameter::Listener {
    DisplayGraph(GnomeDistortAudioProcessor&);
    ~DisplayGraph();

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {} // not implemented

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    GnomeDistortAudioProcessor& audioProcessor;
    juce::Image background;
    std::function<float(float)> waveshaperFunction;

    juce::Rectangle<int> getRenderArea();
};
