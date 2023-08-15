/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
struct CustomRotarySlider : juce::Slider {
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox) {

    }
};

struct CustomSelect : juce::ComboBox {
    CustomSelect() : juce::ComboBox() {

    }
};

struct DisplayComponent : juce::Component, juce::AudioProcessorParameter::Listener, juce::Timer {
    DisplayComponent(GnomeDistortAudioProcessor&);
    ~DisplayComponent();

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {} // not implemented
    void timerCallback() override;

    void paint(juce::Graphics& g) override;

private:
    GnomeDistortAudioProcessor& audioProcessor;

    juce::Atomic<bool> parametersChanged{ false };
    MonoChain monoChain;
};


class GnomeDistortAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
    GnomeDistortAudioProcessorEditor(GnomeDistortAudioProcessor&);
    ~GnomeDistortAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    GnomeDistortAudioProcessor& audioProcessor;

    CustomRotarySlider LoCutFreqSlider, PeakFreqSlider, PeakGainSlider, PeakQSlider, HiCutFreqSlider, PreGainSlider, BiasSlider, WaveShapeAmountSlider, PostGainSlider;
    CustomSelect LoCutSlopeSelect, HiCutSlopeSelect, WaveshapeSelect;
    DisplayComponent displayComp;

    using APVTS = juce::AudioProcessorValueTreeState;
    using SliderAttachment = APVTS::SliderAttachment;
    using SelectAttachment = APVTS::ComboBoxAttachment;
    SliderAttachment LoCutFreqSliderAttachment, PeakFreqSliderAttachment, PeakGainSliderAttachment, PeakQSliderAttachment,
        HiCutFreqSliderAttachment, PreGainSliderAttachment, BiasSliderAttachment, WaveShapeAmountSliderAttachment, PostGainSliderAttachment;
    SelectAttachment LoCutSlopeSelectAttachment, HiCutSlopeSelectAttachment, WaveshapeSelectAttachment;

    std::vector<juce::Component*> getComponents();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GnomeDistortAudioProcessorEditor)
};
