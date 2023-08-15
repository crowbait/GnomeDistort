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
Coefficients generatePeakFilter(const ChainSettings& chainSettings, double sampleRate) {
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, chainSettings.PeakFreq, chainSettings.PeakQ,
        juce::Decibels::decibelsToGain(chainSettings.PeakGain)); // convert decibels to gain value
}
void updateCoefficients(Coefficients& old, const Coefficients& replace) {
    *old = *replace;
}
juce::StringArray GnomeDistortAudioProcessor::getSlopeOptions() {
    juce::StringArray result;
    for (int i = 0; i < 4; i++) {
        juce::String str;
        str << (12 + i * 12);           // generates options 12, 24 ...
        str << (" dB");
        result.add(str);
    }
    return result;
}
juce::StringArray GnomeDistortAudioProcessor::getWaveshaperOptions() {
    return {
        "HardClip",
        "SoftClip",
        "Cracked",
        "Jericho",
        "Warm"
    };
}
std::function<float(float)> getWaveshaperFunction(WaveShaperFunction& func, float& amount) {
    switch (func) {
        case HardClip:
            return [amount](float x) { return juce::jlimit(0.f - (1.f - amount), 1.f - amount, x); };
            break;
        case SoftClip:  // x * sqrt(1+a²) - scaling factor 5
            return [amount](float x) { return juce::jlimit(-1.f, 1.f, x * sqrt(1 + ((amount * 5) * (amount * 5)))); };
            break;
        case Cracked:   // x³ * cos(x*a)³ - scaling factor 9.4
            return [amount](float x) { return pow(x, 3) * pow((cos(x * amount * 9.4f)), 3); };
            break;
        case Jericho:   // x - (a/t)
            return [amount](float x) { return juce::jlimit(-1.f, 1.f, x - (amount / x)); };
            break;
        case Warm:      // x < 0: x*a   --  x > 0: x*(1+a)
            return [amount](float x) {
                if (x <= 0) return juce::jlimit(-1.f, 1.f, x * (1.f - amount));
                return juce::jlimit(-1.f, 1.f, x * (1.f + amount));
                }; break;
    }
}


void updateSettings(ChainSettings& chainSettings, double sampleRate, MonoChain& leftChain, MonoChain& rightChain) {
    // link LoCut filter coefficients
    auto LoCutCoefficients = generateLoCutFilter(chainSettings, sampleRate);
    auto& leftLoCut = leftChain.get<ChainPositions::LoCut>();
    auto& rightLoCut = rightChain.get<ChainPositions::LoCut>();
    updateCutFilter(leftLoCut, LoCutCoefficients, static_cast<FilterSlope>(chainSettings.LoCutSlope));
    updateCutFilter(rightLoCut, LoCutCoefficients, static_cast<FilterSlope>(chainSettings.LoCutSlope));

    // link peak filter coefficient
    auto peakCoefficients = generatePeakFilter(chainSettings, sampleRate);
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

    // link HiCut filter coefficients
    auto HiCutCoefficients = generateHiCutFilter(chainSettings, sampleRate);
    auto& leftHiCut = leftChain.get<ChainPositions::HiCut>();
    auto& rightHiCut = rightChain.get<ChainPositions::HiCut>();
    updateCutFilter(leftHiCut, HiCutCoefficients, static_cast<FilterSlope>(chainSettings.HiCutSlope));
    updateCutFilter(rightHiCut, HiCutCoefficients, static_cast<FilterSlope>(chainSettings.HiCutSlope));

    // pre-gain
    leftChain.get<ChainPositions::PreGain>().setGainDecibels(chainSettings.PreGain);
    rightChain.get<ChainPositions::PreGain>().setGainDecibels(chainSettings.PreGain);

    // bias
    leftChain.get<ChainPositions::DistBias>().setBias(chainSettings.Bias);
    rightChain.get<ChainPositions::DistBias>().setBias(chainSettings.Bias);

    // waveshaper
    WaveShaperFunction waveShapeFunction = static_cast<WaveShaperFunction>(chainSettings.WaveShapeFunction);
    auto& waveShaperFunction = getWaveshaperFunction(waveShapeFunction, chainSettings.WaveShapeAmount);
    leftChain.get<ChainPositions::DistWaveshaper>().functionToUse = waveShaperFunction;
    rightChain.get<ChainPositions::DistWaveshaper>().functionToUse = waveShaperFunction;
    // makeup-gain after waveshaper for following functions
    if (waveShapeFunction == WaveShaperFunction::HardClip) {
        float makeupGain = 16.f * 2 * chainSettings.WaveShapeAmount * chainSettings.WaveShapeAmount;
        leftChain.get<ChainPositions::WaveshaperMakeupGain>().setGainDecibels(makeupGain);
        rightChain.get<ChainPositions::WaveshaperMakeupGain>().setGainDecibels(makeupGain);
    }

    // post-gain
    leftChain.get<ChainPositions::PostGain>().setGainDecibels(chainSettings.PostGain);
    rightChain.get<ChainPositions::PostGain>().setGainDecibels(chainSettings.PostGain);
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
    settings.Bias = apvts.getRawParameterValue("Bias")->load();
    settings.WaveShapeAmount = apvts.getRawParameterValue("WaveShapeAmount")->load();
    settings.WaveShapeFunction = static_cast<WaveShaperFunction>(apvts.getRawParameterValue("WaveShapeFunction")->load());
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
    // generic interface

    //return new juce::GenericAudioProcessorEditor(*this);
    return new GnomeDistortAudioProcessorEditor(*this);
}

//==============================================================================
void GnomeDistortAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void GnomeDistortAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    auto state = juce::ValueTree::readFromData(data, sizeInBytes);
    if (state.isValid()) {
        apvts.replaceState(state);
        updateSettings(getChainSettings(apvts), getSampleRate(), leftChain, rightChain);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout GnomeDistortAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::StringArray& slopeOptions = getSlopeOptions();

    layout.add(std::make_unique<juce::AudioParameterFloat>(         // Type: float (=range)
                                                           "LoCutFreq", "LoCutFreq",                                   // Parameter names
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),  // Parameter range (20-20k, step-size 1, skew: <1 fills more of the slider with low range
                                                           20.f));                                                     // default value
    layout.add(std::make_unique<juce::AudioParameterChoice>(        // Type: choice
                                                            "LoCutSlope", "LoCutSlope",                                 // Parameter names
                                                            slopeOptions, 1));                                          // Choices StringArray, default index

    layout.add(std::make_unique<juce::AudioParameterFloat>("PeakFreq", "PeakFreq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 1200.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("PeakGain", "PeakGain", juce::NormalisableRange<float>(-36.f, 36.f, 0.25f, 1.f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("PeakQ", "PeakQ", juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f), 1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("HiCutFreq", "HiCutFreq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HiCutSlope", "HiCutSlope", slopeOptions, 1));
    // distortion specific parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>("PreGain", "PreGain", juce::NormalisableRange<float>(-8.f, 32.f, 0.5f, 1.f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Bias", "Bias", juce::NormalisableRange<float>(-1.f, 1.f, 0.01f, 1.f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("WaveShapeAmount", "WaveShapeAmount", juce::NormalisableRange<float>(0.f, 0.990f, 0.01f, 0.75f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("WaveShapeFunction", "WaveShapeFunction", getWaveshaperOptions(), 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>("PostGain", "PostGain", juce::NormalisableRange<float>(-32.f, 8.f, 0.5f, 1.f), 0.f));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new GnomeDistortAudioProcessor();
}
