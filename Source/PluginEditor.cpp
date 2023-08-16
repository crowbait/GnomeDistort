/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeelSliderValues::drawRotarySlider(juce::Graphics& g,
                                                     int x, int y, int width, int height,
                                                     float sliderPosProportional,
                                                     float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) {
    using namespace juce;
    auto bounds = Rectangle<float>(x, y, width, height);
    auto center = bounds.getCentre();

    // DEBUG: BOUNDS
    // g.setColour(Colours::blue);
    // g.drawRect(bounds.toFloat(), 1);

    // knob body
    g.setColour(Colour(110u, 10u, 10u));
    g.fillEllipse(bounds);

    // overlay
    juce::Image overlay = juce::ImageCache::getFromMemory(BinaryData::knob_overlay_128_png, BinaryData::knob_overlay_128_pngSize);
    g.drawImage(overlay, bounds, RectanglePlacement::stretchToFit);

    if (auto* sldr = dynamic_cast<RotarySliderLabeledValues*>(&slider)) {
        bool isSmall = sldr->getIsSmallText();
        int textHeight = sldr->getTextHeight();

        // label box
        Rectangle<float> labelboxRect;
        Path labelboxP;
        String labeltext = sldr->getLabelString();
        g.setFont(textHeight);
        auto strWidthLabel = g.getCurrentFont().getStringWidth(labeltext); // replace with variable "text" for accuracy, 5 digits for all-same boxes
        if (isSmall) {
            labelboxRect.setSize(strWidthLabel + 2, sldr->getTextHeight() + 1);
        } else {
            labelboxRect.setSize(strWidthLabel + 4, sldr->getTextHeight() + 2);
        }
        labelboxRect.setCentre(center.getX(), center.getY() - bounds.getHeight() * 0.15);
        labelboxP.addRoundedRectangle(labelboxRect, 2.f);
        g.setColour(Colours::darkgrey);
        g.strokePath(labelboxP, PathStrokeType(1));
        g.setColour(Colours::black);
        g.fillPath(labelboxP);

        g.setColour(Colours::lightgrey);
        g.drawFittedText(labeltext, labelboxRect.toNearestInt(), Justification::centred, 1);

        // value box
        Rectangle<float> valueboxRect;
        Path valueboxP;
        String valuetext = sldr->getDisplayString();
        auto strWidthValue = g.getCurrentFont().getStringWidth("12345"); // replace with variable "text" for accuracy, 5 digits for all-same boxes
        if (isSmall) {
            valueboxRect.setSize(strWidthValue + 2, sldr->getTextHeight() + 1);
        } else {
            valueboxRect.setSize(strWidthValue + 4, sldr->getTextHeight() + 2);
        }
        valueboxRect.setCentre(center.getX(), center.getY() + bounds.getHeight() * 0.25);
        valueboxP.addRoundedRectangle(valueboxRect, 2.f);
        g.setColour(Colours::darkgrey);
        g.strokePath(valueboxP, PathStrokeType(1));
        g.setColour(Colours::black);
        g.fillPath(valueboxP);

        g.setColour(Colours::lightgrey);
        g.drawFittedText(valuetext, valueboxRect.toNearestInt(), Justification::centred, 1);
    }

    // indicator
    Rectangle<float> indicatorRect;
    Path indicatorP;
    indicatorRect.setLeft(center.getX() - 2);
    indicatorRect.setRight(center.getX() + 2);
    indicatorRect.setTop(bounds.getY());
    indicatorRect.setBottom(center.getY());
    indicatorP.addRectangle(indicatorRect);
    jassert(rotaryStartAngle < rotaryEndAngle);
    auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
    indicatorP.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));
    g.setColour(Colours::lightgrey);
    g.fillPath(indicatorP);
}


//==============================================================================
//==============================================================================
//==============================================================================


void RotarySliderLabeledValues::paint(juce::Graphics& g) {
    using namespace juce;

    Rectangle<int> bounds = getLocalBounds();
    auto minAngle = degreesToRadians(-135.f);
    auto maxAngle = degreesToRadians(135.f);
    auto range = getRange();

    // DEBUG: BOUNDS
    // g.setColour(Colours::red);
    // g.drawRect(bounds.toFloat(), 1);
    // g.setColour(Colours::orange);
    // g.drawRect(bounds.toFloat(), 1);

    auto sliderBounds = getSliderBounds(bounds);
    getLookAndFeel().drawRotarySlider(
        g, sliderBounds.getX(), sliderBounds.getY(), sliderBounds.getWidth(), sliderBounds.getHeight(),
        jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),   // normalize
        minAngle, maxAngle,
        *this
    );
}
juce::Rectangle<int> RotarySliderLabeledValues::getSliderBounds(juce::Rectangle<int>& bounds) const {
    auto size = juce::jmin(bounds.getWidth()-12, bounds.getHeight()-12);
    juce::Rectangle<int> r;
    r.setSize(size, size);
    int textHeight = getTextHeight();
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(bounds.getY() + 2);
    return r;
}
juce::String RotarySliderLabeledValues::getDisplayString() const {
    return juce::String(getValue());
}



//==============================================================================
//==============================================================================
//==============================================================================


DisplayComponent::DisplayComponent(GnomeDistortAudioProcessor& p) : audioProcessor(p) {
    const auto& params = audioProcessor.getParameters();    // register as listener to reflect parameter changes in UI
    for (auto param : params) {
        // if (param->getName(64) == "LoCutFreq") param->addListener(this);
        // or simply audioProcessor.getParameter("LoCutFreq")->addListener(this);
        param->addListener(this);
    }
    startTimerHz(60);   // timer for repaint
}
DisplayComponent::~DisplayComponent() {
    const auto& params = audioProcessor.getParameters();

    for (auto param : params) {
        param->removeListener(this);
    }
}

void DisplayComponent::paint(juce::Graphics& g) {
    using namespace juce;

    // spectrum
    auto displayArea = getLocalBounds();

    const int width = displayArea.getWidth();
    const double outputMin = displayArea.getBottom();
    const double outputMax = displayArea.getY();

    g.setColour(Colours::darkgrey);
    g.fillRect(displayArea.toFloat());

    auto& loCut = monoChain.get<ChainPositions::LoCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& hiCut = monoChain.get<ChainPositions::HiCut>();
    auto& postWaveshaper = monoChain.get<ChainPositions::WaveshaperMakeupGain>();

    auto sampleRate = audioProcessor.getSampleRate();

    // get filter magnitudes
    std::vector<double> magnitudes;
    magnitudes.resize(width);
    for (int i = 0; i < width; i++) {   // compute magnitude per pixel
        double mag = 1.f;   // init gain magnitude
        double freq = juce::mapToLog10((double)i / (double)width, 20.0, 20000.0);

        if (!loCut.isBypassed<0>()) mag *= loCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!loCut.isBypassed<1>()) mag *= loCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!loCut.isBypassed<2>()) mag *= loCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!loCut.isBypassed<3>()) mag *= loCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!hiCut.isBypassed<0>()) mag *= hiCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!hiCut.isBypassed<1>()) mag *= hiCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!hiCut.isBypassed<2>()) mag *= hiCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!hiCut.isBypassed<3>()) mag *= hiCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        magnitudes[i] = Decibels::gainToDecibels(mag);
    }
    Path filterResponseCurve;
    auto map = [outputMin, outputMax](double input) { return jmap(input, -36.0, 36.0, outputMin, outputMax); };
    filterResponseCurve.startNewSubPath(displayArea.getX(), map(magnitudes.front()));
    for (int i = 1; i < magnitudes.size(); i++) {   // set path for every pixel
        filterResponseCurve.lineTo(displayArea.getX() + i, map(magnitudes[i]));
    }
    g.setColour(Colours::lightgoldenrodyellow);
    g.strokePath(filterResponseCurve, PathStrokeType(2));   // draw path
}

void DisplayComponent::parameterValueChanged(int parameterIndex, float newValue) {
    parametersChanged.set(true);
}

void DisplayComponent::timerCallback() {
    if (parametersChanged.compareAndSetBool(false, true)) {
        // update monochain
        ChainSettings chainSettings = getChainSettings(audioProcessor.apvts);
        auto loCoefficients = generateLoCutFilter(chainSettings, audioProcessor.getSampleRate());
        updateCutFilter(monoChain.get<ChainPositions::LoCut>(), loCoefficients, static_cast<FilterSlope>(chainSettings.LoCutSlope));
        Coefficients peakCoefficients = generatePeakFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
        auto hiCoefficients = generateHiCutFilter(chainSettings, audioProcessor.getSampleRate());
        updateCutFilter(monoChain.get<ChainPositions::HiCut>(), hiCoefficients, static_cast<FilterSlope>(chainSettings.HiCutSlope));

        repaint();
    }
}


//==============================================================================
//==============================================================================
//==============================================================================


GnomeDistortAudioProcessorEditor::GnomeDistortAudioProcessorEditor(GnomeDistortAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    LoCutFreqSlider(*audioProcessor.apvts.getParameter("LoCutFreq"), "Hz", false, "LOW CUT"), // init sliders
    PeakFreqSlider(*audioProcessor.apvts.getParameter("PeakFreq"), "Hz", true, "FREQ"),
    PeakGainSlider(*audioProcessor.apvts.getParameter("PeakGain"), "", true, "GAIN"),
    PeakQSlider(*audioProcessor.apvts.getParameter("PeakQ"), "", true, "Q"),
    HiCutFreqSlider(*audioProcessor.apvts.getParameter("HiCutFreq"), "Hz", false, "HIGH CUT"),
    PreGainSlider(*audioProcessor.apvts.getParameter("PreGain"), "", false, "GAIN"),
    BiasSlider(*audioProcessor.apvts.getParameter("Bias"), "", false, "BIAS"),
    WaveShapeAmountSlider(*audioProcessor.apvts.getParameter("WaveShap"), "", false, "DIST AMOUNT"),
    PostGainSlider(*audioProcessor.apvts.getParameter("PostGain"), "", false, "GAIN"),

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

    auto displayArea = bounds.removeFromTop(bounds.getHeight() * 0.25);
    displayArea.removeFromLeft(padding);
    displayArea.removeFromRight(padding);
    displayArea.removeFromTop(padding);
    displayArea.removeFromBottom(padding * 2);
    displayComp.setBounds(displayArea);    // 25%

    auto filterArea = bounds.removeFromTop(bounds.getHeight() * 0.33);      // 75*0.33=25%
    auto leftFilterArea = filterArea.removeFromLeft(filterArea.getWidth() * 0.25);
    auto rightFilterArea = filterArea.removeFromRight(filterArea.getWidth() * 0.33);
    LoCutSlopeSelect.setBounds(leftFilterArea.removeFromBottom(selectHeight));
    LoCutFreqSlider.setBounds(leftFilterArea);
    HiCutSlopeSelect.setBounds(rightFilterArea.removeFromBottom(selectHeight));
    HiCutFreqSlider.setBounds(rightFilterArea);
    filterArea.removeFromLeft(padding);
    filterArea.removeFromRight(padding);
    PeakGainSlider.setBounds(filterArea.removeFromTop(filterArea.getHeight() * 0.5));
    PeakFreqSlider.setBounds(filterArea.removeFromLeft(filterArea.getWidth() * 0.5));
    PeakQSlider.setBounds(filterArea);

    bounds.removeFromTop(padding * 2);
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
        &WaveshapeSelect,

        &displayComp
    };
}
