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
void GnomeDistortAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
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

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);

        // ..do something to the data...
    }
}

//==============================================================================
bool GnomeDistortAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* GnomeDistortAudioProcessor::createEditor() {
    return new GnomeDistortAudioProcessorEditor(*this);
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
        "PreLoCutFreq", "PreLoCutFreq",                             // Parameter names
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.75f),  // Parameter range (20-20k, step-size 1, skew: <1 fills more of the slider with low range
        20.f));                                                     // default value

    layout.add(std::make_unique<juce::AudioParameterChoice>(        // Type: choice
        "PreLoCutSlope", "PreHiCutSlope",                           // Parameter names
        slopeOptions, 1));                                          // Choices StringArray, default index

    layout.add(std::make_unique<juce::AudioParameterFloat>("PreHiCutFreq", "PreHiCutFreq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.75f), 20000.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("PreHiCutSlope", "PreHiCutSlope", slopeOptions, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>("PrePeakFreq", "PrePeakFreq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.75f), 750.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("PrePeakGain", "PrePeakGain", juce::NormalisableRange<float>(-24.f, 64.f, 0.25f, 1.f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("PrePeakQ", "PrePeakQ", juce::NormalisableRange<float>(-0.1f, 10.f, 0.05f, 1.f), 1.f));

    // distortion specific parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>("PreGain", "PreGain", juce::NormalisableRange<float>(-48.f, 48.f, 0.5f, 1.f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("WaveShapeAmount", "WaveShapeAmount", juce::NormalisableRange<float>(0.f, 100.f, 0.1f, 0.5f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("ConvolutionAmount", "ConvolutionAmount", juce::NormalisableRange<float>(0.f, 100.f, 0.1f, 0.5f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("PostGain", "PostGain", juce::NormalisableRange<float>(-48.f, 48.f, 0.5f, 1.f), 0.f));

    // post-filter paramters
    layout.add(std::make_unique<juce::AudioParameterFloat>("PostLoCutFreq", "PostLoCutFreq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.75f), 20.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("PostLoCutSlope", "PostLoCutSlope", slopeOptions, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>("PostHiCutFreq", "PostHiCutFreq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.75f), 20000.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("PostHiCutSlope", "PostHiCutSlope", slopeOptions, 1));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new GnomeDistortAudioProcessor();
}
