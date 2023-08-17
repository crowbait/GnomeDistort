/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

DisplayComponent::DisplayComponent(GnomeDistortAudioProcessor& p) : audioProcessor(p),
leftPreFifo(&audioProcessor.leftPreProcessingFifo), leftPostFifo(&audioProcessor.leftPostProcessingFifo) {
    std::vector<juce::String> paramsToLink{
        "LoCutFreq",
        "LoCutSlope",
        "PeakFreq",
        "PeakGain",
        "PeakQ",
        "HiCutFreq",
        "HiCutSlope"
    };

    const auto& params = audioProcessor.getParameters();    // register as listener to reflect parameter changes in UI
    for (auto param : params) {
        // if (param->getName(64) == "LoCutFreq") param->addListener(this);
        // param->addListener(this);
        if (std::find(paramsToLink.begin(), paramsToLink.end(), param->getName(64)) != paramsToLink.end()) {
            param->addListener(this);
        }
    }

    preFFTDataGenerator.changeOrder(FFTOrder::order4096);
    postFFTDataGenerator.changeOrder(FFTOrder::order8192);
    preBuffer.setSize(1, preFFTDataGenerator.getFFTSize());
    postBuffer.setSize(1, postFFTDataGenerator.getFFTSize());

    updateSettings();
    startTimerHz(50);   // timer for repaint
}
DisplayComponent::~DisplayComponent() {
    const auto& params = audioProcessor.getParameters();

    for (auto param : params) {
        param->removeListener(this);
    }
}

void DisplayComponent::resized() {
    using namespace juce;
    auto bounds = getAnalysisArea();
    int left = bounds.getX();
    int right = bounds.getRight();
    int top = bounds.getY();
    int bottom = bounds.getBottom();
    int width = bounds.getWidth();
    int height = bounds.getHeight();
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
    Graphics g(background);

    g.setColour(COLOR_BG_VERYDARK);
    g.fillRect(getLocalBounds().toFloat());

    g.setColour(Colours::dimgrey);
    g.setFont(gridFontHeight);

    // draw frequencies
    Array<float> freqs{
        20, 30, 50, 100,
        200, 500, 1000,
        2000, 5000, 10000, 20000
    };
    Array<float> xs;
    for (float f : freqs) {
        float normalizedX = mapFromLog10(f, 20.f, 20000.f);  // same as in drawing filter response curve
        xs.add(left + width * normalizedX);
    }
    for (float x : xs) {
        g.drawVerticalLine(x, top, bottom);
    }
    for (int i = 0; i < freqs.size(); i++) {
        float f = freqs[i];
        float x = xs[i];

        bool addK = false;
        String str;
        if (f > 999.f) {
            addK = true;
            f /= 1000.f;
        }
        str << f;
        if (addK) str << "k";
        str << "Hz";

        Rectangle<int> r;
        r.setSize(g.getCurrentFont().getStringWidth(str), gridFontHeight);
        r.setCentre(x - 4, 0);
        r.setY(1);
        g.drawFittedText(str, r, Justification::centred, 1);
    }

    // draw gains
    Array<float> gains{
        -36, -24, -12, 0, 12, 24, 36
    };
    Array<float> ys;
    for (float gdB : gains) {
        auto normalizedY = jmap(gdB, -36.f, 36.f, (float)bottom, (float)top);
        ys.add(normalizedY);
        g.setColour(gdB == 0.f ? Colours::grey : Colours::dimgrey);
        g.drawHorizontalLine(normalizedY, left, right);

        String str;
        str << gdB;
        str << "dB";
        Rectangle<int> r;
        r.setSize(g.getCurrentFont().getStringWidth(str), gridFontHeight);
        r.setCentre(0, normalizedY);
        r.setX(2);
        g.drawFittedText(str, r, Justification::centred, 1);
    }

    g.setColour(COLOR_KNOB);
    g.drawRoundedRectangle(bounds.toFloat(), 2.f, 1.f);
}

void DisplayComponent::paint(juce::Graphics& g) {
    using namespace juce;

    auto displayArea = getLocalBounds();
    auto renderArea = getRenderArea();
    auto analysisArea = getAnalysisArea();

    const int width = analysisArea.getWidth();
    const double outputMin = analysisArea.getBottom();
    const double outputMax = analysisArea.getY();

    g.setColour(COLOR_BG_VERYDARK);
    g.fillRect(displayArea.toFloat());
    g.drawImage(background, displayArea.toFloat());

    // draw signals
    g.setColour(COLOR_KNOB);
    postFFTPath.applyTransform(AffineTransform().translation(analysisArea.getX(), analysisArea.getY()));
    g.strokePath(postFFTPath, PathStrokeType(2.f));
    g.setColour(COLOR_BG_MID);
    preFFTPath.applyTransform(AffineTransform().translation(analysisArea.getX(), analysisArea.getY()));
    g.strokePath(preFFTPath, PathStrokeType(2.f));

    auto& loCut = monoChain.get<ChainPositions::LoCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& hiCut = monoChain.get<ChainPositions::HiCut>();
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
    filterResponseCurve.startNewSubPath(analysisArea.getX(), map(magnitudes.front()));
    for (int i = 1; i < magnitudes.size(); i++) {   // set path for every pixel
        filterResponseCurve.lineTo(analysisArea.getX() + i, map(magnitudes[i]));
    }
    g.setColour(Colours::white);
    g.strokePath(filterResponseCurve, PathStrokeType(2));   // draw path
}

juce::Rectangle<int> DisplayComponent::getRenderArea() {
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(24);
    bounds.removeFromTop(12);
    return bounds;
}

juce::Rectangle<int> DisplayComponent::getAnalysisArea() {
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    bounds.removeFromRight(8);
    return bounds;
}

void DisplayComponent::updateSettings() {
    ChainSettings chainSettings = getChainSettings(audioProcessor.apvts);
    auto loCoefficients = generateLoCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::LoCut>(), loCoefficients, static_cast<FilterSlope>(chainSettings.LoCutSlope));
    Coefficients peakCoefficients = generatePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    auto hiCoefficients = generateHiCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::HiCut>(), hiCoefficients, static_cast<FilterSlope>(chainSettings.HiCutSlope));
}

void DisplayComponent::parameterValueChanged(int parameterIndex, float newValue) {
    parametersChanged.set(true);
}

void DisplayComponent::generatePathFromIncomingAudio(SingleChannelSampleFifo<GnomeDistortAudioProcessor::BlockType>* fifo, juce::AudioBuffer<float>* buffer, FFTDataGenerator<std::vector<float>>* FFTGen,
                                   AnalyzerPathGenerator<juce::Path>* pathProducer, juce::Path* path, bool closedPath) {
    const float negInfinity = -48.f;
    juce::AudioBuffer<float> tempIncomingBuffer;
    while (fifo->getNumCompletedBuffersAvailable() > 0) {
        if (fifo->getAudioBuffer(tempIncomingBuffer)) {  // read temp buffer, push into pre-processing buffer
            int size = tempIncomingBuffer.getNumSamples();
            juce::FloatVectorOperations::copy(buffer->getWritePointer(0, 0), buffer->getReadPointer(0, size), buffer->getNumSamples() - size);
            juce::FloatVectorOperations::copy(buffer->getWritePointer(0, buffer->getNumSamples() - size), tempIncomingBuffer.getReadPointer(0, 0), size);
            FFTGen->produceFFTData(*buffer, negInfinity);
        }
    }
    const auto fftBounds = getAnalysisArea().toFloat();
    const int fftSize = FFTGen->getFFTSize();
    const float binWidth = audioProcessor.getSampleRate() / (double)fftSize;

    while (FFTGen->getNumAvailableFFTDataBlocks() > 0) {    // generate paths from FFT data
        std::vector<float> fftData;
        if (FFTGen->getFFTData(fftData)) {
            pathProducer->generatePath(fftData, fftBounds, fftSize, binWidth, negInfinity, closedPath);
        }
    }

    while (pathProducer->getNumPathsAvailable() > 0) {    // pull paths as long as there are any, draw the most recent one
        pathProducer->getPath(*path);
    }
}

void DisplayComponent::timerCallback() {
    generatePathFromIncomingAudio(leftPreFifo, &preBuffer, &preFFTDataGenerator, &prePathProducer, &preFFTPath, false);
    generatePathFromIncomingAudio(leftPostFifo, &postBuffer, &postFFTDataGenerator, &postPathProducer, &postFFTPath, false);
    if (parametersChanged.compareAndSetBool(false, true)) {
        updateSettings();
    }
    repaint(); // repaint not only on param changes, because of paths
}


//==============================================================================
//==============================================================================
//==============================================================================


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
