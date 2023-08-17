/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Helpers/FFTDataGenerator.h"
#include "UI/Colors.h"
#include "UI/SliderKnobLabeledValue.h"

struct CustomSelect : juce::ComboBox {
    CustomSelect() : juce::ComboBox() {

    }
};

// =======================================================
// =======================================================
// =======================================================

struct DisplayComponent : juce::Component, juce::AudioProcessorParameter::Listener, juce::Timer {
    DisplayComponent(GnomeDistortAudioProcessor&);
    ~DisplayComponent();

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {} // not implemented
    void timerCallback() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    GnomeDistortAudioProcessor& audioProcessor;

    juce::Atomic<bool> parametersChanged{ false };
    void updateSettings();
    MonoChain monoChain;

    juce::Image background;
    juce::Rectangle<int> getRenderArea();
    juce::Rectangle<int> getAnalysisArea();
    int gridFontHeight = 8;

    void generatePathFromIncomingAudio(SingleChannelSampleFifo<GnomeDistortAudioProcessor::BlockType>* fifo,
                                       juce::AudioBuffer<float>* buffer,
                                       FFTDataGenerator<std::vector<float>>* FFTGen,
                                       AnalyzerPathGenerator<juce::Path>* pathProducer,
                                       juce::Path* path, bool closedPath);
    SingleChannelSampleFifo<GnomeDistortAudioProcessor::BlockType>* leftPreFifo;
    SingleChannelSampleFifo<GnomeDistortAudioProcessor::BlockType>* leftPostFifo;
    juce::AudioBuffer<float> preBuffer, postBuffer;
    FFTDataGenerator<std::vector<float>> preFFTDataGenerator, postFFTDataGenerator;
    AnalyzerPathGenerator<juce::Path> prePathProducer, postPathProducer;
    juce::Path preFFTPath, postFFTPath;
};

// =======================================================
// =======================================================
// =======================================================

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

    juce::Image knobOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GnomeDistortAudioProcessorEditor)
};
