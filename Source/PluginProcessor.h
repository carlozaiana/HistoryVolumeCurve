#pragma once

#include <JuceHeader.h>

//==============================================================================
class HistoryVolumeAudioProcessor  : public juce::AudioProcessor
{
public:
    HistoryVolumeAudioProcessor();
    ~HistoryVolumeAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
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

    // ---- history buffer API ----
    void pushSamplesToHistory (const float* samples, int numSamples, int numChannels);

    // Read-only access for GUI: will copy out a requested range (thread-safe)
    // startIndex = oldest sample index available (0-based), sampleCount = how many
    // Returns number of samples copied into dest.
    int readHistorySamples (float* dest, int destSize, int startIndex, int sampleCount);

    int getHistorySize() const noexcept { return historySize; }
    int getNumWritten() const noexcept { return totalWritten.load(); }
    double getSampleRate() const noexcept { return currentSampleRate; }

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HistoryVolumeAudioProcessor)

    // ring buffer for absolute amplitude history (mono)
    std::vector<float> historyBuffer;
    int historySize = 4800000; // default ~ 108 seconds @ 44.1k - adjust as needed (big buffer)
    std::atomic<int> writeIndex { 0 };
    std::atomic<int> totalWritten { 0 };
    double currentSampleRate = 44100.0;

    // helper
    inline void writeToHistory(float value);
};
