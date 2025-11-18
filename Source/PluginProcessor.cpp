#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

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
      history (2 * 48000 * 120) // ~120 seconds at 48 kHz (per-sample envelope)
#else
    : history (2 * 48000 * 120)
#endif
{
}

HistoryVolumeCurveAudioProcessor::~HistoryVolumeCurveAudioProcessor() = default;

const juce::String HistoryVolumeCurveAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

//==============================================================================
// Required overrides describing plugin capabilities

bool HistoryVolumeCurveAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool HistoryVolumeCurveAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool HistoryVolumeCurveAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double HistoryVolumeCurveAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================
// Programs (single dummy program)

int HistoryVolumeCurveAudioProcessor::getNumPrograms()
{
    return 1;
}

int HistoryVolumeCurveAudioProcessor::getCurrentProgram()
{
    return 0;
}

void HistoryVolumeCurveAudioProcessor::setCurrentProgram (int)
{
}

const juce::String HistoryVolumeCurveAudioProcessor::getProgramName (int)
{
    return {};
}

void HistoryVolumeCurveAudioProcessor::changeProgramName (int, const juce::String&)
{
}

//==============================================================================

void HistoryVolumeCurveAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    currentSampleRate = sampleRate;

    // Envelope time constants (tweak to taste)
    const float attackMs  = 5.0f;    // fast attack
    const float releaseMs = 250.0f;  // slow-ish release

    const float attackTimeSamples  = (float) (attackMs  * 0.001 * currentSampleRate);
    const float releaseTimeSamples = (float) (releaseMs * 0.001 * currentSampleRate);

    // Exponential envelope coefficients
    attackCoeff  = std::exp (-1.0f / attackTimeSamples);
    releaseCoeff = std::exp (-1.0f / releaseTimeSamples);

    levelEnv = 0.0f;
}

void HistoryVolumeCurveAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HistoryVolumeCurveAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Only allow symmetrical input/output layouts
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}
#endif

void HistoryVolumeCurveAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer&)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        float maxv = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
            maxv = std::max (maxv, std::abs (buffer.getReadPointer (ch)[i]));

        // Envelope follower: smooth volume curve
        const float coeff = (maxv > levelEnv ? attackCoeff : releaseCoeff);
        levelEnv = coeff * levelEnv + (1.0f - coeff) * maxv;

        // Push smoothed envelope value into history
        history.push (levelEnv);
    }
}

//==============================================================================

juce::AudioProcessorEditor* HistoryVolumeCurveAudioProcessor::createEditor()
{
    return new HistoryVolumeCurveAudioProcessorEditor (*this);
}

bool HistoryVolumeCurveAudioProcessor::hasEditor() const
{
    return true;
}

//==============================================================================
// State (no parameters yet, so we leave these empty)

void HistoryVolumeCurveAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
    // If you add parameters, save them here.
}

void HistoryVolumeCurveAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
    // If you add parameters, restore them here.
}

//==============================================================================
// Factory function required by JUCE for all plugin formats

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HistoryVolumeCurveAudioProcessor();
}