#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>

//==============================================================================
// Simple lock-free ring buffer storing floating-point samples
class LockFreeRingBuffer
{
public:
    explicit LockFreeRingBuffer (size_t capacity);

    void push (float v);
    int  readBlock (float* dest, int startIndex, int count);
    int  available() const noexcept;

private:
    std::vector<float>    buffer_;
    const size_t          capacity_;
    std::atomic<size_t>   writeIndex_   { 0 };
    std::atomic<int64_t>  totalWritten_ { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LockFreeRingBuffer)
};

//==============================================================================

class HistoryVolumeCurveAudioProcessor  : public juce::AudioProcessor
{
public:
    HistoryVolumeCurveAudioProcessor();
    ~HistoryVolumeCurveAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock; // for double-precision buffers

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // History buffer accessible from the editor
    LockFreeRingBuffer history;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HistoryVolumeCurveAudioProcessor)
};