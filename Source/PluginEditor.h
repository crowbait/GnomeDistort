/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider {
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox) {

    }
};

struct CustomSelect : juce::ComboBox {
    CustomSelect() : juce::ComboBox() {

    }
};

//==============================================================================
/**
*/
class GnomeDistortAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    GnomeDistortAudioProcessorEditor (GnomeDistortAudioProcessor&);
    ~GnomeDistortAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    GnomeDistortAudioProcessor& audioProcessor;

    CustomRotarySlider LoCutFreqSlider, PeakFreqSlider, PeakGainSlider, PeakQSlider, HiCutFreqSlider, PreGainSlider, BiasSlider, WaveShapeAmountSlider, PostGainSlider;
    CustomSelect LoCutSlopeSelect, HiCutSlopeSelect, WaveshapeSelect;
    std::vector<juce::Component*> getComponents();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GnomeDistortAudioProcessorEditor)
};
