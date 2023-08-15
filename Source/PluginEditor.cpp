/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GnomeDistortAudioProcessorEditor::GnomeDistortAudioProcessorEditor(GnomeDistortAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for (auto* comp : getComponents()) {
        addAndMakeVisible(comp);
    }

    setSize(400, 600);
}

//==============================================================================
void GnomeDistortAudioProcessorEditor::paint(juce::Graphics& g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    juce::Image background = juce::ImageCache::getFromMemory(BinaryData::bg_png, BinaryData::bg_pngSize);
    g.drawImageAt(background, 0, 0);

    // g.setColour(juce::Colours::white);
    // g.setFont(15.0f);
    // g.drawFittedText("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

GnomeDistortAudioProcessorEditor::~GnomeDistortAudioProcessorEditor() {}

void GnomeDistortAudioProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    const int padding = 12;
    const int selectHeight = 24;

    auto bounds = getLocalBounds();
    bounds.removeFromLeft(padding);
    bounds.removeFromTop(padding);
    bounds.removeFromRight(padding);
    bounds.removeFromBottom(padding);

    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.25);    // 25%

    auto filterArea = bounds.removeFromTop(bounds.getHeight() * 0.33);      // 75*0.33=25%
    auto leftFilterArea = filterArea.removeFromLeft(filterArea.getWidth() * 0.33);
    auto rightFilterArea = filterArea.removeFromRight(filterArea.getWidth() * 0.5);
    LoCutSlopeSelect.setBounds(leftFilterArea.removeFromBottom(selectHeight));
    LoCutFreqSlider.setBounds(leftFilterArea);
    HiCutSlopeSelect.setBounds(rightFilterArea.removeFromBottom(selectHeight));
    HiCutFreqSlider.setBounds(rightFilterArea);
    filterArea.removeFromLeft(padding);
    filterArea.removeFromRight(padding);
    PeakFreqSlider.setBounds(filterArea.removeFromTop(filterArea.getHeight() * 0.5));
    PeakQSlider.setBounds(filterArea.removeFromLeft(filterArea.getWidth() * 0.5));
    PeakGainSlider.setBounds(filterArea);

    auto preDistArea = bounds.removeFromLeft(bounds.getWidth() * 0.25);
    auto postDistArea = bounds.removeFromRight(bounds.getWidth() * 0.33);
    PreGainSlider.setBounds(preDistArea);
    PostGainSlider.setBounds(postDistArea);
    bounds.removeFromLeft(padding);
    bounds.removeFromRight(padding);
    BiasSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
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

        &LoCutSlopeSelect,
        &HiCutSlopeSelect,
        &WaveshapeSelect
    };
}
