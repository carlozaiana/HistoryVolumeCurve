#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// LockFreeRingBuffer

LockFreeRingBuffer::LockFreeRingBuffer (size_t capacity)
    : buffer_ (capacity, 0.0f), capacity_ (capacity)
{
}

void LockFreeRingBuffer::push (float v)
{
    size_t idx = writeIndex_.fetch_add (1, std::memory_order_relaxed) % capacity_;
    buffer_[idx] = v;
    totalWritten_.fetch_add (1, std::memory_order_relaxed);
}

// Copies up to count samples starting at startIndex (0 = oldest available)
// Returns number of samples copied
int LockFreeRingBuffer::readBlock (float* dest, int startIndex, int count)
{
    int64_t written   = totalWritten_.load (std::memory_order_relaxed);
    int     available = (int) std::min<int64_t> (written, (int64_t) capacity_);

    if (count <= 0 || startIndex >= available)
        return 0;

    int toCopy       = std::min (count, available - startIndex);
    int oldestGlobal = (int) std::max<int64_t> (0, written - (int64_t) capacity_);

    for (int i = 0; i < toCopy; ++i)
    {
        int globalIdx = oldestGlobal + startIndex + i;
        int ringIdx   = globalIdx % (int) capacity_;
        dest[i]       = buffer_[ringIdx];
    }

    return toCopy;
}

int LockFreeRingBuffer::available() const noexcept
{
    int64_t written = totalWritten_.load (std::memory_order_relaxed);
    return (int) std::min<int64_t> (written, (int64_t) capacity_);
}

//==============================================================================
// HistoryVolumeCurveAudioProcessor

HistoryVolumeCurveAudioProcessor::HistoryVolumeCurveAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      history (2 * 48000 * 120) // reserve ~120 seconds at 48k
#endif
{
}

HistoryVolumeCurveAudioProcessor::~HistoryVolumeCurveAudioProcessor() {}

const juce::String HistoryVolumeCurveAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

void HistoryVolumeCurveAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    ignoreUnused (sampleRate, samplesPerBlock);
}

void HistoryVolumeCurveAudioProcessor::releaseResources() {}

bool HistoryVolumeCurveAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void HistoryVolumeCurveAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        float maxv = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
            maxv = std::max (maxv, std::abs (buffer.getReadPointer (ch)[i]));

        // push the per-sample peak to ring buffer
        history.push (maxv);
    }
}

juce::AudioProcessorEditor* HistoryVolumeCurveAudioProcessor::createEditor()
{
    return new HistoryVolumeCurveAudioProcessorEditor (*this);
}

bool HistoryVolumeCurveAudioProcessor::hasEditor() const
{
    return true;
}

//==============================================================================
// Factory function required by JUCE for all plugin formats

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HistoryVolumeCurveAudioProcessor();
}