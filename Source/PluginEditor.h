#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class HistoryOpenGLRenderer : public juce::Component,
                              private juce::OpenGLRenderer,
                              private juce::Timer
{
public:
    HistoryOpenGLRenderer(HistoryVolumeAudioProcessor&);
    ~HistoryOpenGLRenderer() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // zoom controls
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override;

    // OpenGLRenderer
    void newOpenGLContextCreated() override;
    void openGLContextClosing() override;
    void renderOpenGL() override;

private:
    HistoryVolumeAudioProcessor& processor;
    juce::OpenGLContext openGLContext;

    // GL resources
    GLuint vbo = 0;
    GLuint shaderProgram = 0;
    GLint attrPos = -1;
    GLint uniOffset = -1;
    GLint uniScale = -1;
    GLint uniColor = -1;

    std::vector<float> vertexData; // x,y pairs
    std::vector<float> tempSamples;

    // Zoom: samples per pixel (lower = more zoomed-in)
    std::atomic<float> samplesPerPixel { 64.0f };
    std::atomic<float> scrollPositionSeconds { 0.0f };

    // Timer to update scrollPositionSeconds smoothly
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HistoryOpenGLRenderer)
};

//==============================================================================
class HistoryVolumeAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    HistoryVolumeAudioProcessorEditor (HistoryVolumeAudioProcessor&);
    ~HistoryVolumeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HistoryVolumeAudioProcessor& processor;
    HistoryOpenGLRenderer glView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HistoryVolumeAudioProcessorEditor)
};
