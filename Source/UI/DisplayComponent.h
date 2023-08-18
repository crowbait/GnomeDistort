/*
  ==============================================================================

    DisplayComponent.h
    Created: 17 Aug 2023 3:21:25pm
    Author:  traxx

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../Helpers/FFTDataGenerator.h"
#include "GlobalConsts.h"

struct DisplayComponent : juce::Component, juce::AudioProcessorParameter::Listener, juce::Timer {
    DisplayComponent(GnomeDistortAudioProcessor&);
    ~DisplayComponent();

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {} // not implemented
    juce::Atomic<bool> parametersChanged{ false };
    void timerCallback() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool isEnabled = true;
    bool isHQ = true;
    juce::Atomic<bool> hasQualityChanged{ true };
private:
    GnomeDistortAudioProcessor& audioProcessor;

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
