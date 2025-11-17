#include "PluginProcessor.h"
#include "PluginEditor.h"

HistoryVolumeAudioProcessor::HistoryVolumeAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                       )
#endif
{
    historyBuffer.assign(historySize, 0.0f);
}

HistoryVolumeAudioProcessor::~HistoryVolumeAudioProcessor() {}

const juce::String HistoryVolumeAudioProcessor::getName() const { return JucePlugin_Name; }
bool HistoryVolumeAudioProcessor::acceptsMidi() const { return false; }
bool HistoryVolumeAudioProcessor::producesMidi() const { return false; }
double HistoryVolumeAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int HistoryVolumeAudioProcessor::getNumPrograms() { return 1; }
int HistoryVolumeAudioProcessor::getCurrentProgram() { return 0; }
void HistoryVolumeAudioProcessor::setCurrentProgram (int) {}
const juce::String HistoryVolumeAudioProcessor::getProgramName (int) { return {}; }
void HistoryVolumeAudioProcessor::changeProgramName (int, const juce::String&) {}

void HistoryVolumeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    // Optionally resize history to keep e.g. 120 s at sampleRate
    // historySize = (int)(sampleRate * 120.0);
    // historyBuffer.assign(historySize, 0.0f);
}

void HistoryVolumeAudioProcessor::releaseResources() {}

bool HistoryVolumeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Accept any layout
    return true;
}

void HistoryVolumeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // compute peak or absolute average per sample frame across channels
    for (int i = 0; i < numSamples; ++i)
    {
        float maxVal = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            maxVal = std::max(maxVal, std::abs(buffer.getReadPointer(ch)[i]));

        writeToHistory(maxVal);
    }
}

// lock-free single-value write helper
inline void HistoryVolumeAudioProcessor::writeToHistory(float value)
{
    int idx = writeIndex.fetch_add(1, std::memory_order_relaxed) % historySize;
    historyBuffer[idx] = value;
    totalWritten.fetch_add(1, std::memory_order_relaxed);
}

void HistoryVolumeAudioProcessor::pushSamplesToHistory (const float* samples, int numSamples, int numChannels)
{
    // Not used for now - processBlock handles it
    (void) samples; (void) numSamples; (void) numChannels;
}

int HistoryVolumeAudioProcessor::readHistorySamples (float* dest, int destSize, int startIndex, int sampleCount)
{
    // startIndex is relative to the oldest sample we can give (i.e. totalWritten - historySize)
    const int written = totalWritten.load(std::memory_order_relaxed);
    const int available = std::min(written, historySize);
    if (sampleCount <= 0 || destSize < sampleCount) return 0;

    int oldestGlobalIndex = std::max(0, written - historySize);
    int globalStart = oldestGlobalIndex + startIndex;
    if (globalStart < oldestGlobalIndex) globalStart = oldestGlobalIndex;

    int copied = 0;
    for (int i = 0; i < sampleCount && copied < destSize; ++i)
    {
        int globalIdx = globalStart + i;
        if (globalIdx >= written) break;
        int ringIdx = (globalIdx % historySize);
        dest[copied++] = historyBuffer[ringIdx];
    }
    return copied;
}

//==============================================================================
juce::AudioProcessorEditor* HistoryVolumeAudioProcessor::createEditor()
{
    return new class HistoryVolumeAudioProcessorEditor (*this);
}

bool HistoryVolumeAudioProcessor::hasEditor() const { return true; }

void HistoryVolumeAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {}
void HistoryVolumeAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {}
