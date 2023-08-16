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
    float LoCutFreq{ 0 }, PeakFreq{ 0 }, PeakGain{ 0 }, PeakQ{ 0 }, HiCutFreq{ 0 }, PreGain{ 0 }, Bias{ 0 }, WaveShapeAmount{ 0 }, PostGain{ 0 }, Mix{ 0 };
    int LoCutSlope{ FilterSlope::Slope12 }, HiCutSlope{ FilterSlope::Slope12 }, WaveShapeFunction{ WaveShaperFunction::HardClip };
};

using Filter = juce::dsp::IIR::Filter<float>;   // alias for Filters
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;     // ProcessorChain which allows to automatically run signal through all specified DSP instances (4 filter slope types)
using Gain = juce::dsp::Gain<float>;
using Bias = juce::dsp::Bias<float>;
using DistWaveShape = juce::dsp::WaveShaper<float, std::function<float(float)>>;

using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter, Gain, Bias, DistWaveShape, Gain, Gain>; // complete effect chain for one channel
enum ChainPositions {
    LoCut,
    Peak,
    HiCut,
    PreGain,
    DistBias,
    DistWaveshaper,
    WaveshaperMakeupGain,
    PostGain
};

using Coefficients = Filter::CoefficientsPtr;
void updateCoefficients(Coefficients& old, const Coefficients& replace);
Coefficients generatePeakFilter(const ChainSettings& chainSettings, double sampleRate);
inline auto generateLoCutFilter(const ChainSettings& chainSettings, double sampleRate) {
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(     // create array of filter coefficients for 4 possible slopes
                                                                                       chainSettings.LoCutFreq, sampleRate, (chainSettings.LoCutSlope + 1) * 2);
}
inline auto generateHiCutFilter(const ChainSettings& chainSettings, double sampleRate) {
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(     // create array of filter coefficients for 4 possible slopes
                                                                                      chainSettings.HiCutFreq, sampleRate, (chainSettings.HiCutSlope + 1) * 2);
}
// template function can be used on left AND right channel
template<typename ChainType, typename CoefficientsType> static void updateCutFilter(ChainType& filter, const CoefficientsType& cutCoefficients, const FilterSlope& slope) {
    filter.template setBypassed<0>(true);     // bypass all 4 possible filters (one for every possible slope)
    filter.template setBypassed<1>(true);
    filter.template setBypassed<2>(true);
    filter.template setBypassed<3>(true);
    switch (slope) {
        case Slope48: *filter.template get<3>().coefficients = *cutCoefficients[3]; filter.setBypassed<3>(false);
        case Slope36: *filter.template get<2>().coefficients = *cutCoefficients[2]; filter.setBypassed<2>(false);
        case Slope24: *filter.template get<1>().coefficients = *cutCoefficients[1]; filter.setBypassed<0>(false);
        case Slope12: *filter.template get<0>().coefficients = *cutCoefficients[0]; filter.setBypassed<0>(false);
    }
}

std::function<float(float)> getWaveshaperFunction(WaveShaperFunction& func, float& amount);
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);
void updateSettings(ChainSettings& chainSettings, double sampleRate, MonoChain& leftChain, MonoChain& rightChain);

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
    static juce::StringArray getSlopeOptions();
    static juce::StringArray getWaveshaperOptions();

private:
    MonoChain leftChain, rightChain;    // stereo
    juce::dsp::DryWetMixer<float> drywetL, drywetR;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GnomeDistortAudioProcessor)
};
