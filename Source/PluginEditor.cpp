/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

GnomeDistortAudioProcessorEditor::GnomeDistortAudioProcessorEditor(GnomeDistortAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    LoCutFreqSlider(*audioProcessor.apvts.getParameter("LoCutFreq"), false, "LOW CUT", false, knobOverlay), // init sliders
    PeakFreqSlider(*audioProcessor.apvts.getParameter("PeakFreq"), true, "FREQ", false, knobOverlay),
    PeakGainSlider(*audioProcessor.apvts.getParameter("PeakGain"), true, "GAIN", true, knobOverlay),
    PeakQSlider(*audioProcessor.apvts.getParameter("PeakQ"), true, "Q", true, knobOverlay),
    HiCutFreqSlider(*audioProcessor.apvts.getParameter("HiCutFreq"), false, "HIGH CUT", false, knobOverlay),
    PreGainSlider(*audioProcessor.apvts.getParameter("PreGain"), false, "GAIN", true, knobOverlay),
    BiasSlider(*audioProcessor.apvts.getParameter("Bias"), false, "BIAS", true, knobOverlay),
    WaveShapeAmountSlider(*audioProcessor.apvts.getParameter("WaveShapeAmount"), false, "DIST AMOUNT", true, knobOverlay),
    PostGainSlider(*audioProcessor.apvts.getParameter("PostGain"), false, "GAIN", true, knobOverlay),
    DryWetSlider(*audioProcessor.apvts.getParameter("DryWet"), false, "MIX", true, knobOverlay),

    displayComp(audioProcessor),    // init display

    LoCutFreqSliderAttachment(audioProcessor.apvts, "LoCutFreq", LoCutFreqSlider),  // attach UI controls to parameters
    PeakFreqSliderAttachment(audioProcessor.apvts, "PeakFreq", PeakFreqSlider),
    PeakGainSliderAttachment(audioProcessor.apvts, "PeakGain", PeakGainSlider),
    PeakQSliderAttachment(audioProcessor.apvts, "PeakQ", PeakQSlider),
    HiCutFreqSliderAttachment(audioProcessor.apvts, "HiCutFreq", HiCutFreqSlider),
    PreGainSliderAttachment(audioProcessor.apvts, "PreGain", PreGainSlider),
    BiasSliderAttachment(audioProcessor.apvts, "Bias", BiasSlider),
    WaveShapeAmountSliderAttachment(audioProcessor.apvts, "WaveShapeAmount", WaveShapeAmountSlider),
    PostGainSliderAttachment(audioProcessor.apvts, "PostGain", PostGainSlider),
    DryWetSliderAttachment(audioProcessor.apvts, "DryWet", DryWetSlider),
    LoCutSlopeSelectAttachment(audioProcessor.apvts, "LoCutSlope", LoCutSlopeSelect),
    HiCutSlopeSelectAttachment(audioProcessor.apvts, "HiCutSlope", HiCutSlopeSelect),
    WaveshapeSelectAttachment(audioProcessor.apvts, "WaveShapeFunction", WaveshapeSelect) {

    juce::StringArray slopeOptions = GnomeDistortAudioProcessor::getSlopeOptions();
    LoCutSlopeSelect.addItemList(slopeOptions, 1);
    LoCutSlopeSelect.setSelectedId(1);
    HiCutSlopeSelect.addItemList(slopeOptions, 1);
    HiCutSlopeSelect.setSelectedId(1);
    WaveshapeSelect.addItemList(GnomeDistortAudioProcessor::getWaveshaperOptions(), 1);
    WaveshapeSelect.setSelectedId(1);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    knobOverlay = juce::ImageCache::getFromMemory(BinaryData::knob_overlay_128_png, BinaryData::knob_overlay_128_pngSize);

    for (auto* comp : getComponents()) {
        addAndMakeVisible(comp);
    }

    setSize(400, 600);
}

//==============================================================================
void GnomeDistortAudioProcessorEditor::paint(juce::Graphics& g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // juce::Image background = juce::ImageCache::getFromMemory(BinaryData::bg_png, BinaryData::bg_pngSize);
    // g.drawImageAt(background, 0, 0);
}

GnomeDistortAudioProcessorEditor::~GnomeDistortAudioProcessorEditor() {}

void GnomeDistortAudioProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    const int padding = 12;
    const int selectHeight = 24;

    auto bounds = getLocalBounds();
    bounds.removeFromLeft(padding * 2);
    bounds.removeFromTop(padding * 2);
    bounds.removeFromRight(padding * 2);
    bounds.removeFromBottom(padding * 2);

    auto displayArea = bounds.removeFromTop(bounds.getHeight() * 0.25f);
    // displayArea.removeFromLeft(padding);
    // displayArea.removeFromRight(padding);
    displayArea.removeFromTop(padding);
    displayArea.removeFromBottom(padding * 2);
    displayComp.setBounds(displayArea);    // 25%

    auto filterArea = bounds.removeFromTop(bounds.getHeight() * 0.33f);      // 75*0.33=25%
    auto leftFilterArea = filterArea.removeFromLeft(filterArea.getWidth() * 0.25f);
    auto rightFilterArea = filterArea.removeFromRight(filterArea.getWidth() * 0.33f);
    LoCutSlopeSelect.setBounds(leftFilterArea.removeFromBottom(selectHeight));
    LoCutFreqSlider.setBounds(leftFilterArea);
    HiCutSlopeSelect.setBounds(rightFilterArea.removeFromBottom(selectHeight));
    HiCutFreqSlider.setBounds(rightFilterArea);
    filterArea.removeFromLeft(padding);
    filterArea.removeFromRight(padding);
    PeakGainSlider.setBounds(filterArea.removeFromTop(filterArea.getHeight() * 0.5f));
    PeakFreqSlider.setBounds(filterArea.removeFromLeft(filterArea.getWidth() * 0.5f));
    PeakQSlider.setBounds(filterArea);

    bounds.removeFromTop(padding * 2);
    auto preDistArea = bounds.removeFromLeft(bounds.getWidth() * 0.25f);
    auto postDistArea = bounds.removeFromRight(bounds.getWidth() * 0.33f);
    PreGainSlider.setBounds(preDistArea);
    PostGainSlider.setBounds(postDistArea.removeFromTop(postDistArea.getHeight() * 0.5f));
    DryWetSlider.setBounds(postDistArea);
    bounds.removeFromLeft(padding);
    bounds.removeFromRight(padding);
    BiasSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33f));
    WaveshapeSelect.setBounds(bounds.removeFromBottom(selectHeight));
    WaveShapeAmountSlider.setBounds(bounds);
}

std::vector<juce::Component*> GnomeDistortAudioProcessorEditor::getComponents() {
    return {
        &LoCutFreqSlider,
        &PeakFreqSlider,
        &PeakGainSlider,
        &PeakQSlider,
        &HiCutFreqSlider,
        &PreGainSlider,
        &BiasSlider,
        &WaveShapeAmountSlider,
        &PostGainSlider,
        &DryWetSlider,

        &LoCutSlopeSelect,
        &HiCutSlopeSelect,
        &WaveshapeSelect,

        &displayComp
    };
}
