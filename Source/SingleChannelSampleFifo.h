/*
  ==============================================================================

    SingleChannelSampleFifo.h
    Created: 16 Aug 2023 9:51:37pm
    Author:  traxx

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Fifo.h"

template<typename BlockType>
struct SingleChannelSampleFifo {
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch) {
        prepared.set(false);
    }

    void update(const BlockType& buffer) {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse);
        auto* channelPtr = buffer.getReadPointer(channelToUse);

        for (int i = 0; i < buffer.getNumSamples(); i++) {
            pushNextSample(channelPtr[i]);
        }
    }

    void prepare(int bufferSize) {
        prepared.set(false);
        size.set(bufferSize);

        bufferToFill.setSize(1, bufferSize, false, true, true);
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }

    int getNumCompletedBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get() }
    bool getAudioBuffer(BlockType& buf) { return audioBuffer.pull(buf); }

private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;

    void pushNextSample(float sample) {
        if (fifoIndex == bufferToFill.getNumSamples()) {
            auto ok = audioBuffer.push(bufferToFill);
            juce::ignoreUnused(ok);
            fifoIndex = 0;
        }
        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};
