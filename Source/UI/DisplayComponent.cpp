/*
  ==============================================================================

    DisplayComponent.cpp
    Created: 17 Aug 2023 3:22:26pm
    Author:  traxx

  ==============================================================================
*/

#include "DisplayComponent.h"

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

    updateSettings();
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
        30, 50, 100,
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
    if (isEnabled) {
        g.setColour(COLOR_KNOB);
        postFFTPath.applyTransform(AffineTransform().translation(analysisArea.getX(), analysisArea.getY()));
        g.strokePath(postFFTPath, PathStrokeType(2.f));
        g.setColour(COLOR_BG_MID);
        preFFTPath.applyTransform(AffineTransform().translation(analysisArea.getX(), analysisArea.getY()));
        g.strokePath(preFFTPath, PathStrokeType(2.f));
    }

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

    if (hasQualityChanged.compareAndSetBool(false, true)) {
        preFFTDataGenerator.changeOrder(isHQ ? FFTOrder::order8192 : FFTOrder::order2048);
        postFFTDataGenerator.changeOrder(isHQ ? FFTOrder::order8192 : FFTOrder::order2048);
        preBuffer.setSize(1, preFFTDataGenerator.getFFTSize());
        postBuffer.setSize(1, postFFTDataGenerator.getFFTSize());

        stopTimer();
        startTimerHz(isHQ ? 60 : 24);   // timer for repaint
    }
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
