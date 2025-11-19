#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class VolumeGraph : public juce::Component, private juce::Timer
{
public:
    VolumeGraph (SmoothVolumeHistoryProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override {}
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override { repaint(); }

    SmoothVolumeHistoryProcessor& processor;

    double visibleSeconds = 15.0;      // default time window
    double yMinDb = -90.0;
    double yMaxDb = 6.0;

    const int subBlockSize = 32;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VolumeGraph)
};

class SmoothVolumeHistoryEditor : public juce::AudioProcessorEditor
{
public:
    SmoothVolumeHistoryEditor (SmoothVolumeHistoryProcessor&);
    void resized() override;

private:
    VolumeGraph graph;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmoothVolumeHistoryEditor)
};