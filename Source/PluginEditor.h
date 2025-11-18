#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Component that draws a scrolling history-volume-curve using JUCE 2D Graphics.
// Supports zoom via mouse wheel.
class HistoryOpenGLComponent : public juce::Component,
                               private juce::Timer
{
public:
    explicit HistoryOpenGLComponent (HistoryVolumeCurveAudioProcessor& p);
    ~HistoryOpenGLComponent() override;

    void paint (juce::Graphics& g) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override;

private:
    void timerCallback() override;

    HistoryVolumeCurveAudioProcessor& processor;

    std::vector<float> sampleScratch;          // temp buffer for history samples
    std::atomic<float> samplesPerPixel { 64.0f }; // zoom factor

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HistoryOpenGLComponent)
};

//==============================================================================

class HistoryVolumeCurveAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit HistoryVolumeCurveAudioProcessorEditor (HistoryVolumeCurveAudioProcessor&);
    ~HistoryVolumeCurveAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    HistoryOpenGLComponent glComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HistoryVolumeCurveAudioProcessorEditor)
};