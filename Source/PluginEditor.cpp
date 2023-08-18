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
    DisplayONSwitch(*audioProcessor.apvts.getParameter("DisplayON"), false, "ON", juce::Colours::white, COLOR_BG_VERYDARK),
    DisplayHQSwitch(*audioProcessor.apvts.getParameter("DisplayHQ"), false, "HQ", "LQ", juce::Colours::white, COLOR_BG_VERYDARK),

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
    WaveshapeSelectAttachment(audioProcessor.apvts, "WaveShapeFunction", WaveshapeSelect),
    DisplayONAttachment(audioProcessor.apvts, "DisplayON", DisplayONSwitch),
    DisplayHQAttachment(audioProcessor.apvts, "DisplayHQ", DisplayHQSwitch) {

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

    // add onclicks for switches
    auto safePtr = juce::Component::SafePointer<GnomeDistortAudioProcessorEditor>(this);
    DisplayONSwitch.onClick = [safePtr]() {
        if (auto* comp = safePtr.getComponent()) comp->displayComp.isEnabled = comp->DisplayONSwitch.getToggleState();
    };
    DisplayHQSwitch.onClick = [safePtr]() {
        if (auto* comp = safePtr.getComponent()) {
            bool state = comp->DisplayHQSwitch.getToggleState();
            comp->displayComp.isHQ = state;
            comp->displayComp.parametersChanged.set(true);
            comp->displayComp.hasQualityChanged.set(true);
        }
    };

    setSize(400, 600);
}

//==============================================================================
void GnomeDistortAudioProcessorEditor::paint(juce::Graphics& g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // juce::Image background = juce::ImageCache::getFromMemory(BinaryData::bg_png, BinaryData::bg_pngSize);
    g.drawImageAt(background, 0, 0);
}

GnomeDistortAudioProcessorEditor::~GnomeDistortAudioProcessorEditor() {}

void GnomeDistortAudioProcessorEditor::paintBackground() {
    using namespace juce;

    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
    Graphics g(background);

    g.setColour(COLOR_BG);
    g.fillRect(getLocalBounds().toFloat());

    g.drawImageWithin(
        juce::ImageCache::getFromMemory(BinaryData::grundge_overlay_png, BinaryData::grundge_overlay_pngSize),
        0, 0, getWidth(), getHeight(), RectanglePlacement::fillDestination, false);

    auto comps = getComponents();
    std::vector<Point<float>> compCenters;
    compCenters.resize(comps.size());
    for (int i = 0; i < comps.size(); i++) {
        float x = comps[i]->getX() + (comps[i]->getWidth() / 2);
        float y = comps[i]->getY() + (comps[i]->getHeight() / 2);
        compCenters[i] = Point(x, y);
    }


    // draw circuit
    auto getMidX = [comps](compIndex left, compIndex right) {
        return comps[left]->getX() + comps[left]->getWidth() + ((comps[right]->getX() - (comps[left]->getX() + comps[left]->getWidth())) / 2);
    };
    auto getMidY = [comps](compIndex top, compIndex bottom) {
        return comps[top]->getY() + comps[top]->getHeight() + ((comps[bottom]->getY() - (comps[top]->getY() + comps[top]->getHeight())) / 2);
    };

    Path circuit;
    circuit.startNewSubPath(0, compCenters[compIndex::LoCutFreqSlider].getY());
    circuit.lineTo(compCenters[compIndex::LoCutFreqSlider]);

    float midXLoCutPeakFreq = getMidX(compIndex::LoCutFreqSlider, compIndex::PeakFreqSlider);
    circuit.lineTo(midXLoCutPeakFreq, compCenters[compIndex::LoCutFreqSlider].getY());
    circuit.lineTo(midXLoCutPeakFreq, compCenters[compIndex::PeakFreqSlider].getY());

    circuit.lineTo(compCenters[compIndex::PeakFreqSlider]);
    circuit.lineTo(compCenters[compIndex::PeakQSlider]);
    float midYQPeakGain = getMidY(compIndex::PeakGainSlider, compIndex::PeakQSlider);
    circuit.lineTo(compCenters[compIndex::PeakQSlider].getX(), midYQPeakGain);
    circuit.lineTo(compCenters[compIndex::PeakGainSlider].getX(), midYQPeakGain);
    circuit.lineTo(compCenters[compIndex::PeakGainSlider]);

    float midXQHiCut = getMidX(compIndex::PeakQSlider, compIndex::HiCutFreqSlider);
    circuit.lineTo(midXQHiCut, compCenters[compIndex::PeakGainSlider].getY());
    circuit.lineTo(midXQHiCut, compCenters[compIndex::HiCutFreqSlider].getY());

    circuit.lineTo(compCenters[compIndex::HiCutFreqSlider]);

    float midYQBias = getMidY(compIndex::PeakQSlider, compIndex::BiasSlider);
    circuit.lineTo(compCenters[compIndex::HiCutFreqSlider].getX(), midYQBias);
    circuit.lineTo(compCenters[compIndex::PreGainSlider].getX(), midYQBias);
    circuit.lineTo(compCenters[compIndex::PreGainSlider]);

    float midXPreDist = getMidX(compIndex::PreGainSlider, compIndex::WaveShapeAmountSlider);
    circuit.lineTo(midXPreDist, compCenters[compIndex::PreGainSlider].getY());
    circuit.lineTo(midXPreDist, compCenters[compIndex::BiasSlider].getY());
    circuit.lineTo(compCenters[compIndex::BiasSlider]);
    circuit.lineTo(compCenters[compIndex::WaveShapeAmountSlider]);

    float midXDistPost = getMidX(compIndex::WaveShapeAmountSlider, compIndex::PostGainSlider);
    circuit.lineTo(midXDistPost, compCenters[compIndex::WaveShapeAmountSlider].getY());
    circuit.lineTo(midXDistPost, compCenters[compIndex::PostGainSlider].getY());
    circuit.lineTo(compCenters[compIndex::PostGainSlider]);
    circuit.lineTo(compCenters[compIndex::DryWetSlider]);
    circuit.lineTo(getWidth(), compCenters[compIndex::DryWetSlider].getY());

    g.setColour(COLOR_KNOB);
    g.strokePath(circuit, PathStrokeType(5.f));

    if (auto* BiasSlider = dynamic_cast<SliderKnobLabeledValues*>(comps[compIndex::BiasSlider])) {
        auto logoGnome = Drawable::createFromImageData(BinaryData::logo_gnome_svg, BinaryData::logo_gnome_svgSize);
        Rectangle<float> logoGnomeBox(
            Point<float>(compCenters[compIndex::PreGainSlider].getX() + 4,
                         midYQBias + 4),
            Point<float>(compCenters[compIndex::BiasSlider].getX() - (BiasSlider->getSliderBounds(BiasSlider->getLocalBounds()).getWidth() / 4),
                         compCenters[compIndex::BiasSlider].getY() - (comps[compIndex::BiasSlider]->getHeight() / 8)));
        logoGnome->drawWithin(g, logoGnomeBox, RectanglePlacement::stretchToFit, 1.f);

        if (auto* PostGain = dynamic_cast<SliderKnobLabeledValues*>(comps[compIndex::PostGainSlider])) {
            auto logoDistort = Drawable::createFromImageData(BinaryData::logo_distort_svg, BinaryData::logo_distort_svgSize);
            Rectangle<float> logoDistBox(
                Point<float>(compCenters[compIndex::PeakQSlider].getX() - 8,
                             midYQBias + 4),
                Point<float>(compCenters[compIndex::PostGainSlider].getX() + (PostGain->getSliderBounds(PostGain->getLocalBounds()).getWidth() / 2),
                             compCenters[compIndex::PostGainSlider].getY() - (PostGain->getSliderBounds(PostGain->getLocalBounds()).getHeight() / 2) - 8));
            logoDistort->drawWithin(g, logoDistBox, RectanglePlacement::stretchToFit, 1.f);
        }
    }

    // draw display 3D
    auto get3DCorners = [](Rectangle<int> bounds, float edgeLength) {
        int left = bounds.getX();
        int top = bounds.getY();
        int right = left + bounds.getWidth();
        int bottom = top + bounds.getHeight();

        Path topEdge;
        topEdge.startNewSubPath(left, top);
        topEdge.lineTo(left - edgeLength, top - edgeLength);
        topEdge.lineTo(right + edgeLength, top - edgeLength);
        topEdge.lineTo(right, top);
        topEdge.closeSubPath();

        Path rightEdge;
        rightEdge.startNewSubPath(right, top);
        rightEdge.lineTo(right + edgeLength, top - edgeLength);
        rightEdge.lineTo(right + edgeLength, bottom + edgeLength);
        rightEdge.lineTo(right, bottom);
        rightEdge.closeSubPath();

        Path bottomEdge;
        bottomEdge.startNewSubPath(left, bottom);
        bottomEdge.lineTo(left - edgeLength, bottom + edgeLength);
        bottomEdge.lineTo(right + edgeLength, bottom + edgeLength);
        bottomEdge.lineTo(right, bottom);
        bottomEdge.closeSubPath();

        Path leftEdge;
        leftEdge.startNewSubPath(left, top);
        leftEdge.lineTo(left - edgeLength, top - edgeLength);
        leftEdge.lineTo(left - edgeLength, bottom + edgeLength);
        leftEdge.lineTo(left, bottom);
        leftEdge.closeSubPath();

        return std::vector<Path> { topEdge, rightEdge, bottomEdge, leftEdge };
    };

    auto displayCorners = get3DCorners(comps[compIndex::displayComp]->getBounds(), comps[compIndex::displayComp]->getX() / 2);
    g.setColour(COLOR_BG_DARK);
    g.fillPath(displayCorners[0]);
    g.setColour(COLOR_BG_MID);
    g.fillPath(displayCorners[1]);
    g.setColour(COLOR_BG_LIGHT);
    g.fillPath(displayCorners[2]);
    g.setColour(COLOR_BG_MIDDARK);
    g.fillPath(displayCorners[3]);



    // draw controls 3D
    for (auto comp : comps) {
        if (auto* sldr = dynamic_cast<SliderKnobLabeledValues*>(comp)) {
            Rectangle<int> outerBounds = sldr->getSliderBounds(sldr->getBounds());
            outerBounds.expand(6, 6);
            Path outer;
            float distCornerCircle = (outerBounds.getWidth() / 2) * (sqrt(2) - 1);
            outer.addEllipse(outerBounds.toFloat());
            ColourGradient darken(
                Colour(COLOR_BG_DARK.getRed(), COLOR_BG_DARK.getGreen(), COLOR_BG_DARK.getBlue(), 1.f),
                outerBounds.getX() + (outerBounds.getWidth() / 4), outerBounds.getY(),
                Colour(COLOR_BG_DARK.getRed(), COLOR_BG_DARK.getGreen(), COLOR_BG_DARK.getBlue(), 0.f),
                outerBounds.getRight() - (outerBounds.getWidth() / 4), outerBounds.getBottom(), true);
            g.setGradientFill(darken);
            g.fillPath(outer);
            ColourGradient lighten(
                Colour(COLOR_BG_MID.getRed(), COLOR_BG_MID.getGreen(), COLOR_BG_MID.getBlue(), 1.f),
                outerBounds.getRight() - (outerBounds.getWidth() / 4), outerBounds.getBottom(),
                Colour(COLOR_BG_MID.getRed(), COLOR_BG_MID.getGreen(), COLOR_BG_MID.getBlue(), 0.f),
                outerBounds.getX() + (outerBounds.getWidth() / 4), outerBounds.getY(), true);
            g.setGradientFill(lighten);
            g.fillPath(outer);
        } else if (auto* slct = dynamic_cast<CustomSelect*>(comp)) {
            auto displayCorners = get3DCorners(slct->getBounds(), 4.f);
            g.setColour(COLOR_BG_DARK);
            g.fillPath(displayCorners[0]);
            g.setColour(COLOR_BG_MID);
            g.fillPath(displayCorners[1]);
            g.setColour(COLOR_BG_LIGHT);
            g.fillPath(displayCorners[2]);
            g.setColour(COLOR_BG_MIDDARK);
            g.fillPath(displayCorners[3]);
        }
    }

}

void GnomeDistortAudioProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    const int padding = 12;
    const int selectHeight = 24;

    auto bounds = getLocalBounds();
    bounds.removeFromLeft(padding * 2);
    auto switchesArea = bounds.removeFromTop(padding * 2);
    bounds.removeFromRight(padding * 2);
    bounds.removeFromBottom(padding * 2);

    auto displayArea = bounds.removeFromTop(bounds.getHeight() * 0.25f);
    // displayArea.removeFromLeft(padding);
    // displayArea.removeFromRight(padding);
    displayArea.removeFromTop(padding);
    displayArea.removeFromBottom(padding * 2);
    displayComp.setBounds(displayArea);    // 25%
    DisplayONSwitch.setBounds(switchesArea.removeFromLeft(padding * 2));
    switchesArea.removeFromLeft(padding / 2);
    DisplayHQSwitch.setBounds(switchesArea.removeFromLeft(padding * 2));

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
    
    paintBackground();
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

        &displayComp,
        &DisplayONSwitch,
        &DisplayHQSwitch
    };
}
