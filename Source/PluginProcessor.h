\
#pragma once
#include <JuceHeader.h>
#include <vector>
#include <atomic>

// Simple lock-free circular buffer for floats (single-producer single-consumer style)
class LockFreeRingBuffer
{
public:
    LockFreeRingBuffer(size_t capacity = 4800000);
    void push(float v);            // push single sample from audio thread
    int readBlock(float* dest, int startIndex, int count); // copies up to count samples starting at startIndex (relative to oldest)
    int available() const noexcept;
    int capacity() const noexcept { return (int)capacity_; }
    int64_t totalWritten() const noexcept { return totalWritten_.load(); }

private:
    std::vector<float> buffer_;
    const size_t capacity_;
    std::atomic<size_t> writeIndex_{0};
    std::atomic<int64_t> totalWritten_{0};
};

//==============================================================================
class HistoryVolumeCurveAudioProcessor  : public juce::AudioProcessor
{
public:
    HistoryVolumeCurveAudioProcessor();
    ~HistoryVolumeCurveAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    // history access
    LockFreeRingBuffer history;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HistoryVolumeCurveAudioProcessor)
};
