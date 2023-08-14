/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
enum FilterSlope {
    Slope12,
    Slope24,
    Slope36,
    Slope48
};

enum WaveShaperFunction {
    HardClip,
    SoftClip,
    Cracked,
    Jericho,
    Warm
};

struct ChainSettings {
    float LoCutFreq{ 0 }, PeakFreq{ 0 }, PeakGain{ 0 }, PeakQ{ 0 }, HiCutFreq{ 0 }, PreGain{ 0 }, Bias{ 0 }, WaveShapeAmount{ 0 }, ConvolutionAmount{ 0 }, PostGain{ 0 };
    int LoCutSlope{ FilterSlope::Slope12 }, HiCutSlope{ FilterSlope::Slope12 }, WaveShapeFunction{ WaveShaperFunction::HardClip };
};

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
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };

private:
    using Filter = juce::dsp::IIR::Filter<float>;   // alias for Filters
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;     // ProcessorChain which allows to automatically run signal through all specified DSP instances (4 filter slope types)
    using Gain = juce::dsp::Gain<float>;
    using Bias = juce::dsp::Bias<float>;
    using DistWaveShape = juce::dsp::WaveShaper<float, std::function<float(float)>>;
    using DistConv = juce::dsp::Convolution;

    // using MonoChain = juce::dsp::ProcessorChain<MonoPreFilter, Gain, DistWaveShape, DistConv, Gain, MonoPostFilter>;    // complete effect chain for one channel
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter, Gain, Bias, DistWaveShape, Gain>;
    MonoChain leftChain, rightChain;    // stereo

    enum ChainPositions {
        LoCut,
        Peak,
        HiCut,
        PreGain,
        DistBias,
        DistWaveshaper,
        WaveshaperMakeupGain,
        DistConvolution,
        PostGain,
        PostFilter
    };

    using Coefficients = Filter::CoefficientsPtr;
    static void updateCoefficients(Coefficients& old, const Coefficients& replace);
    static std::function<float(float)> getWaveshaperFunction(WaveShaperFunction& func, float& amount);
    static void updateSettings(ChainSettings& chainSettings, double sampleRate, MonoChain& leftChain, MonoChain& rightChain);
    // template function can be used on left AND right channel
    template<typename ChainType, typename CoefficientsType> static void updateCutFilter(ChainType& leftLoCut, const CoefficientsType& cutCoefficients, const FilterSlope& slope) {
        leftLoCut.template setBypassed<0>(true);     // bypass all 4 possible filters (one for every possible slope)
        leftLoCut.template setBypassed<1>(true);
        leftLoCut.template setBypassed<2>(true);
        leftLoCut.template setBypassed<3>(true);
        switch (slope) {
            case Slope48: *leftLoCut.template get<3>().coefficients = *cutCoefficients[3]; leftLoCut.setBypassed<3>(false);
            case Slope36: *leftLoCut.template get<2>().coefficients = *cutCoefficients[2]; leftLoCut.setBypassed<2>(false);
            case Slope24: *leftLoCut.template get<1>().coefficients = *cutCoefficients[1]; leftLoCut.setBypassed<0>(false);
            case Slope12: *leftLoCut.template get<0>().coefficients = *cutCoefficients[0]; leftLoCut.setBypassed<0>(false);
        }
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GnomeDistortAudioProcessor)
};
