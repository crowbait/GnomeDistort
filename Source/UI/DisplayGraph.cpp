/*
  ==============================================================================

    DisplayGraph.cpp
    Created: 18 Sep 2023 5:38:19pm
    Author:  traxx

  ==============================================================================
*/

#include "DisplayGraph.h"

DisplayGraph::DisplayGraph(GnomeDistortAudioProcessor& p) : audioProcessor(p) {
    for (auto param : audioProcessor.getParameters()) {
        if (param->getName(64) == "WaveShapeAmount") { param->addListener(this); continue; }
        if (param->getName(64) == "WaveShapeFunction") { param->addListener(this); continue; }
    }
    parameterValueChanged(0, 0.f);
}
DisplayGraph::~DisplayGraph() {
    for (auto param : audioProcessor.getParameters()) {
        param->removeListener(this);
    }
}

void DisplayGraph::resized() {
    using namespace juce;
    auto bounds = getLocalBounds();
    auto renderArea = getRenderArea();
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
    Graphics g(background);
    g.setColour(COLOR_BG_VERYDARK);
    g.fillAll();
    g.setColour(Colours::dimgrey);
    g.drawRoundedRectangle(renderArea.toFloat(), 2.f, 1.f);
    g.drawHorizontalLine(renderArea.getY() + (renderArea.getHeight() / 2), renderArea.getX(), renderArea.getRight());
    g.drawVerticalLine(renderArea.getX() + (renderArea.getWidth() / 2), renderArea.getY(), renderArea.getBottom());
}

void DisplayGraph::paint(juce::Graphics& g) {
    using namespace juce;
    g.drawImage(background, getLocalBounds().toFloat());

    auto renderArea = getRenderArea();
    int left = renderArea.getX();
    int right = renderArea.getRight();
    int top = renderArea.getY();
    int bottom = renderArea.getBottom();
    auto mapX = [left, right](int x) {return jmap((float)x, (float)left, (float)right, -1.f, 1.f); };
    auto mapY = [top, bottom](float y) {return jmap(y, -1.f, 1.f, (float)bottom, (float)top); };
    g.setColour(COLOR_KNOB);
    Path graph;
    graph.startNewSubPath(renderArea.getX(), mapY(waveshaperFunction(-1)));
    for (int x = left + 1; x < right; x++) {
        graph.lineTo(x, mapY(waveshaperFunction(mapX(x))));
    }
    g.strokePath(graph, PathStrokeType(2));   // draw path
}

void DisplayGraph::parameterValueChanged(int parameterIndex, float newValue) {
    ChainSettings chainSettings = getChainSettings(audioProcessor.apvts);
    float amount = chainSettings.WaveShapeAmount;
    int func = chainSettings.WaveShapeFunction;
    if (parameterIndex == TreeParameter::PosWaveShapeAmount) amount = newValue;
    if (parameterIndex == TreeParameter::PosWaveShapeFunction) func = juce::jmap(newValue, 0.f, (float)WaveShaperOptions.size() - 1);
    WaveShaperFunction waveShapeFunction = static_cast<WaveShaperFunction>(func);
    waveshaperFunction = getWaveshaperFunction(waveShapeFunction, chainSettings.WaveShapeAmount);
    repaint();
}

juce::Rectangle<int> DisplayGraph::getRenderArea() {
    auto bounds = getLocalBounds();
    const int remove = 4;
    bounds.removeFromLeft(remove * 2);
    bounds.removeFromTop(remove * 4);
    bounds.removeFromRight(remove * 3);
    bounds.removeFromBottom(remove);
    return bounds;
}
