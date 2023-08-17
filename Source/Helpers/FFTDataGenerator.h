/*
  ==============================================================================

    FFTDataGenerator.h
    Created: 17 Aug 2023 1:38:09am
    Author:  traxx

    This provides a class to convert from FiFo audio buffer to FFT data (Fast Fourier Transform)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Fifo.h"

enum FFTOrder {
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

template<typename BlockType>
struct FFTDataGenerator {
    void produceFFTData(const juce::AudioBuffer<float>& audioData, const float negativeInfinity) {
        const auto fftSize = getFFTSize();
        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());

        window->multiplyWithWindowingTable(fftData.data(), fftSize);    // apply windowing function to data
        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());   // render
        int numBins = (int)fftSize / 2;
        for (int i = 0; i < numBins; i++) {
            fftData[i] /= (float)numBins;   // normalize
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);  // convert to dB
        }
        fftDataFifo.push(fftData);
    }

    void changeOrder(FFTOrder newOrder) {
        order = newOrder;
        auto fftSize = getFFTSize();
        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);
        fftData.clear();
        fftData.resize(fftSize * 2, 0);
        fftDataFifo.prepare(fftData.size());
    }

    int getFFTSize() const { return 1 << order; };
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }

private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    Fifo<BlockType> fftDataFifo;
};
