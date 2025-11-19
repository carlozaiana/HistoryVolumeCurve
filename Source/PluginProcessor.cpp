#include "PluginProcessor.h"
#include "PluginEditor.h"

SmoothVolumeHistoryProcessor::SmoothVolumeHistoryProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void SmoothVolumeHistoryProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
}

void SmoothVolumeHistoryProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    const int subBlockSize = 32;                      // ← this is what makes it jitter-free

    int pos = 0;
    while (pos < numSamples)
    {
        int blockSize = juce::jmin (subBlockSize, numSamples - pos);
        float peak = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* channelData = buffer.getReadPointer (ch, pos);
            for (int i = 0; i < blockSize; ++i)
                peak = juce::jmax (peak, std::abs (channelData[i]));
        }

        float db = peak > 0.00001f ? 20.0f * std::log10f (peak) : -100.0f;

        {
            juce::ScopedLock sl (levelLock);
            levelHistory.push_back (db);
            if (levelHistory.size() > 1'500'000)          // ~5–6 min at 48 kHz, more than enough
                levelHistory.pop_front();
        }

        pos += blockSize;
    }
    // audio is passed through unchanged
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SmoothVolumeHistoryProcessor();
}

juce::AudioProcessorEditor* SmoothVolumeHistoryProcessor::createEditor()
{
    return new SmoothVolumeHistoryEditor (*this);
}