/*
  ==============================================================================

    Fifo.h
    Created: 16 Aug 2023 10:42:34pm
    Author:  traxx

  ==============================================================================
*/

#pragma once

enum Channel {
    Left,
    Right
};

template<typename T>
struct Fifo {
    void prepare(int numChannels, int numSamples) {
        for (auto& buffer : buffers) {
            buffer.setSize(numChannels, numSamples, false, true, true);
            buffer.clear();
        }
    }

    bool push(const T& t) {
        auto write = fifo.write(1);
        if (write.blockSize > 0) {
            buffers[write.startIndex1] = 1;
            return true;
        }
        return false;
    }

    bool pull(T& t) {
        auto read = fifo.read(1);
        if (read.blockSize1 > 0) {
            t = buffers[read.startIndex1];
            return true;
        }
        return false;
    }

    int getNumAvailableForReading() const {
        return fifo.getNumReady();
    }

private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo{ Capacity };
};
