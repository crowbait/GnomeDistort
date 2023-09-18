/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/GlobalConsts.h"
#include "UI/Display.h"
#include "UI/SliderKnobLabeledValue.h"
#include "UI/SimpleTextSwitch.h"
#include "UI/SimpleTextButton.h"

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

    displayComp,
    DisplayONSwitch,
    DisplayHQSwitch,
    LinkGithubButton,
    LinkDonateButton
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

    juce::LookAndFeel_V4 ComboBoxLNF;
    Display displayComp;
    SliderKnobLabeledValues LoCutFreqSlider, PeakFreqSlider, PeakGainSlider, PeakQSlider, HiCutFreqSlider, PreGainSlider, BiasSlider, WaveShapeAmountSlider, PostGainSlider, DryWetSlider;
    juce::ComboBox LoCutSlopeSelect, HiCutSlopeSelect, WaveshapeSelect;
    SimpleTextSwitch DisplayONSwitch, DisplayHQSwitch;
    SimpleTextButton LinkGithubButton, LinkDonateButton;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS::SliderAttachment LoCutFreqSliderAttachment, PeakFreqSliderAttachment, PeakGainSliderAttachment, PeakQSliderAttachment,
        HiCutFreqSliderAttachment, PreGainSliderAttachment, BiasSliderAttachment, WaveShapeAmountSliderAttachment, PostGainSliderAttachment, DryWetSliderAttachment;
    APVTS::ComboBoxAttachment LoCutSlopeSelectAttachment, HiCutSlopeSelectAttachment, WaveshapeSelectAttachment;
    APVTS::ButtonAttachment DisplayONAttachment, DisplayHQAttachment;

    std::vector<juce::Component*> getComponents();

    void paintBackground();
    juce::Image background;
    juce::Image knobOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GnomeDistortAudioProcessorEditor)
};
