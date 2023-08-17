/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/Colors.h"
#include "UI/DisplayComponent.h"
#include "UI/SliderKnobLabeledValue.h"

struct CustomSelect : juce::ComboBox {
    CustomSelect() : juce::ComboBox() {

    }
};

// =======================================================
// =======================================================
// =======================================================

enum compIndex {
    LoCutFreqSlider,
    PeakFreqSlider,
    PeakGainSlider,
    PeakQSlider,
    HiCutFreqSlider,
    PreGainSlider,
    BiasSlider,
    WaveShapeAmountSlider,
    PostGainSlider,
    DryWetSlider,

    LoCutSlopeSelect,
    HiCutSlopeSelect,
    WaveshapeSelect,

    displayComp
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

    SliderKnobLabeledValues LoCutFreqSlider, PeakFreqSlider, PeakGainSlider, PeakQSlider, HiCutFreqSlider, PreGainSlider, BiasSlider, WaveShapeAmountSlider, PostGainSlider, DryWetSlider;
    CustomSelect LoCutSlopeSelect, HiCutSlopeSelect, WaveshapeSelect;
    DisplayComponent displayComp;

    using APVTS = juce::AudioProcessorValueTreeState;
    using SliderAttachment = APVTS::SliderAttachment;
    using SelectAttachment = APVTS::ComboBoxAttachment;
    SliderAttachment LoCutFreqSliderAttachment, PeakFreqSliderAttachment, PeakGainSliderAttachment, PeakQSliderAttachment,
        HiCutFreqSliderAttachment, PreGainSliderAttachment, BiasSliderAttachment, WaveShapeAmountSliderAttachment, PostGainSliderAttachment, DryWetSliderAttachment;
    SelectAttachment LoCutSlopeSelectAttachment, HiCutSlopeSelectAttachment, WaveshapeSelectAttachment;

    std::vector<juce::Component*> getComponents();

    void paintBackground();
    juce::Image background;
    juce::Image knobOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GnomeDistortAudioProcessorEditor)
};
