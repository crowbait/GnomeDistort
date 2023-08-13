/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class GnomeDistortAudioProcessor : public juce::AudioProcessor
#if JucePlugin_Enable_ARA
    , public juce::AudioProcessorARAExtension
#endif
{
public:
    //==============================================================================
    GnomeDistortAudioProcessor();
    ~GnomeDistortAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState treeState{ *this, nullptr, "Parameters", createParameterLayout() };

private:
    using Filter = juce::dsp::IIR::Filter<float>;   // alias for Filters
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;     // ProcessorChain which allows to automatically run signal through all specified DSP instances (4 filter slope types)
    using Gain = juce::dsp::Gain<float>;

    using MonoPreFilter = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;    // ProcessorChain automatically running all three pre-dist EQ Filters
    using MonoDistWaveShape = juce::dsp::WaveShaper<float>;
    using MonoDistConv = juce::dsp::Convolution;
    using MonoPostFilter = juce::dsp::ProcessorChain<CutFilter, CutFilter>;

    using MonoChain = juce::dsp::ProcessorChain<MonoPreFilter, Gain, MonoDistWaveShape, MonoDistConv, Gain, MonoPostFilter>;    // complete effect chain for one channel
    MonoChain leftChain, rightChain;    // stereo

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GnomeDistortAudioProcessor)
};
