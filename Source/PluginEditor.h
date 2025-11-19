#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SmoothScopeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Timer
{
public:
    SmoothScopeAudioProcessorEditor (SmoothScopeAudioProcessor&);
    ~SmoothScopeAudioProcessorEditor() override;

    // --- Component Lifecycle ---
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    // --- Interaction ---
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    SmoothScopeAudioProcessor& audioProcessor;

    // --- Visualization Data ---
    // A circular buffer to hold history for the GUI
    static constexpr int historySize = 5000; 
    std::vector<float> historyBuffer;
    int historyWriteIndex = 0;

    // --- Zoom Parameters ---
    float zoomX = 1.0f;
    float zoomY = 1.0f;

    // Constants for constraints
    const float minZoomX = 0.5f;
    const float maxZoomX = 50.0f;
    const float minZoomY = 0.5f;
    const float maxZoomY = 10.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmoothScopeAudioProcessorEditor)
};