/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GnomeDistortAudioProcessor::GnomeDistortAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{}

GnomeDistortAudioProcessor::~GnomeDistortAudioProcessor() {}

//==============================================================================
const juce::String GnomeDistortAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool GnomeDistortAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool GnomeDistortAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool GnomeDistortAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double GnomeDistortAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int GnomeDistortAudioProcessor::getNumPrograms() {
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int GnomeDistortAudioProcessor::getCurrentProgram() {
    return 0;
}

void GnomeDistortAudioProcessor::setCurrentProgram(int index) {}

const juce::String GnomeDistortAudioProcessor::getProgramName(int index) {
    return {};
}

void GnomeDistortAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

//==============================================================================
//==============================================================================
//==============================================================================

// HELPERS

void GnomeDistortAudioProcessor::updateCoefficients(Coefficients& old, const Coefficients& replace) {
    *old = *replace;
}

void GnomeDistortAudioProcessor::updateSettings(ChainSettings& chainSettings, double sampleRate, MonoChain& leftChain, MonoChain& rightChain) {
    // link LoCut filter coefficients
    auto LoCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(     // create array of filter coefficients for 4 possible slopes
        chainSettings.LoCutFreq, sampleRate, (chainSettings.LoCutSlope + 1) * 2);
    auto& leftLoCut = leftChain.get<ChainPositions::LoCut>();
    auto& rightLoCut = rightChain.get<ChainPositions::LoCut>();
    updateCutFilter(leftLoCut, LoCutCoefficients, static_cast<FilterSlope>(chainSettings.LoCutSlope));
    updateCutFilter(rightLoCut, LoCutCoefficients, static_cast<FilterSlope>(chainSettings.LoCutSlope));

    // link peak filter coefficient
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, chainSettings.PeakFreq, chainSettings.PeakQ,
        juce::Decibels::decibelsToGain(chainSettings.PeakGain)); // convert decibels to gain value
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

    // link HiCut filter coefficients
    auto HiCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(     // create array of filter coefficients for 4 possible slopes
        chainSettings.HiCutFreq, sampleRate, (chainSettings.HiCutSlope + 1) * 2);
    auto& leftHiCut = leftChain.get<ChainPositions::HiCut>();
    auto& rightHiCut = rightChain.get<ChainPositions::HiCut>();
    updateCutFilter(leftHiCut, HiCutCoefficients, static_cast<FilterSlope>(chainSettings.HiCutSlope));
    updateCutFilter(rightHiCut, HiCutCoefficients, static_cast<FilterSlope>(chainSettings.HiCutSlope));
}

//==============================================================================
//==============================================================================
//==============================================================================

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts) {
    ChainSettings settings;

    settings.LoCutFreq = apvts.getRawParameterValue("LoCutFreq")->load();
    settings.LoCutSlope = static_cast<FilterSlope>(apvts.getRawParameterValue("LoCutSlope")->load());
    settings.PeakFreq = apvts.getRawParameterValue("PeakFreq")->load();
    settings.PeakGain = apvts.getRawParameterValue("PeakGain")->load();
    settings.PeakQ = apvts.getRawParameterValue("PeakQ")->load();
    settings.HiCutFreq = apvts.getRawParameterValue("HiCutFreq")->load();
    settings.HiCutSlope = static_cast<FilterSlope>(apvts.getRawParameterValue("HiCutSlope")->load());

    settings.PreGain = apvts.getRawParameterValue("PreGain")->load();
    settings.WaveShapeAmount = apvts.getRawParameterValue("WaveShapeAmount")->load();
    settings.ConvolutionAmount = apvts.getRawParameterValue("ConvolutionAmount")->load();
    settings.PostGain = apvts.getRawParameterValue("PostGain")->load();

    return settings;
}

void GnomeDistortAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    // initializing ProcessorChains with provided specifications
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    leftChain.prepare(spec);
    rightChain.prepare(spec);

    // init settings
    ChainSettings chainSettings = getChainSettings(apvts);
    updateSettings(chainSettings, sampleRate, leftChain, rightChain);

    // links low cut filter coefficient




}

void GnomeDistortAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GnomeDistortAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void GnomeDistortAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // prepare settings before processing audio
    ChainSettings chainSettings = getChainSettings(apvts);
    updateSettings(chainSettings, getSampleRate(), leftChain, rightChain);

    // links low cut filter coefficient
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(     // create array of filter coefficients for 4 possible slopes
        chainSettings.LoCutFreq, getSampleRate(), (chainSettings.LoCutSlope + 1) * 2);
    auto& leftLoCut = leftChain.get<ChainPositions::LoCut>();
    leftLoCut.setBypassed<0>(true);     // bypass all 4 possible filters (one for every possible slope)
    leftLoCut.setBypassed<1>(true);
    leftLoCut.setBypassed<2>(true);
    leftLoCut.setBypassed<3>(true);
    switch (chainSettings.LoCutSlope) {
        case Slope12:
            *leftLoCut.get<0>().coefficients = *cutCoefficients[0]; leftLoCut.setBypassed<0>(false); break;
        case Slope24:
            *leftLoCut.get<0>().coefficients = *cutCoefficients[0]; leftLoCut.setBypassed<0>(false);
            *leftLoCut.get<1>().coefficients = *cutCoefficients[1]; leftLoCut.setBypassed<0>(false); break;
        case Slope36:
            *leftLoCut.get<0>().coefficients = *cutCoefficients[0]; leftLoCut.setBypassed<0>(false);
            *leftLoCut.get<1>().coefficients = *cutCoefficients[1]; leftLoCut.setBypassed<1>(false);
            *leftLoCut.get<2>().coefficients = *cutCoefficients[2]; leftLoCut.setBypassed<2>(false); break;
        case Slope48:
            *leftLoCut.get<0>().coefficients = *cutCoefficients[0]; leftLoCut.setBypassed<0>(false);
            *leftLoCut.get<1>().coefficients = *cutCoefficients[1]; leftLoCut.setBypassed<1>(false);
            *leftLoCut.get<2>().coefficients = *cutCoefficients[2]; leftLoCut.setBypassed<2>(false);
    }

    // run audio through ProcessorChain
    juce::dsp::AudioBlock<float> block(buffer);                             // separating left and right channel
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);       // create ProcessContext for both channels
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);                                         // process
    rightChain.process(rightContext);
}

//==============================================================================
bool GnomeDistortAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* GnomeDistortAudioProcessor::createEditor() {
    // return new GnomeDistortAudioProcessorEditor(*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void GnomeDistortAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void GnomeDistortAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout GnomeDistortAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::StringArray slopeOptions;     // generate options for filter steepness
    for (int i = 0; i < 4; i++) {
        juce::String str;
        str << (12 + i * 12);           // generates options 12, 24 ...
        str << (" dB");
        slopeOptions.add(str);
    }

    layout.add(std::make_unique<juce::AudioParameterFloat>(         // Type: float (=range)
        "LoCutFreq", "LoCutFreq",                                   // Parameter names
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),  // Parameter range (20-20k, step-size 1, skew: <1 fills more of the slider with low range
        20.f));                                                     // default value
    layout.add(std::make_unique<juce::AudioParameterChoice>(        // Type: choice
        "LoCutSlope", "HiCutSlope",                                 // Parameter names
        slopeOptions, 1));                                          // Choices StringArray, default index

    layout.add(std::make_unique<juce::AudioParameterFloat>("PeakFreq", "PeakFreq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 750.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("PeakGain", "PeakGain", juce::NormalisableRange<float>(-24.f, 64.f, 0.25f, 1.f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("PeakQ", "PeakQ", juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f), 1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("HiCutFreq", "HiCutFreq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HiCutSlope", "HiCutSlope", slopeOptions, 1));
    // distortion specific parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>("PreGain", "PreGain", juce::NormalisableRange<float>(-48.f, 48.f, 0.5f, 1.f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("WaveShapeAmount", "WaveShapeAmount", juce::NormalisableRange<float>(0.f, 100.f, 0.1f, 0.5f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("ConvolutionAmount", "ConvolutionAmount", juce::NormalisableRange<float>(0.f, 100.f, 0.1f, 0.5f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("PostGain", "PostGain", juce::NormalisableRange<float>(-48.f, 48.f, 0.5f, 1.f), 0.f));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new GnomeDistortAudioProcessor();
}
