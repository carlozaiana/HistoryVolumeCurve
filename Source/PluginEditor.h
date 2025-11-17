\
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// OpenGL renderer component that draws a scrolling history-volume-curve.
// Supports zoom via mouse wheel and requests MSAA via pixel format.
class HistoryOpenGLComponent : public juce::Component,
                               private juce::OpenGLRenderer,
                               private juce::Timer
{
public:
    HistoryOpenGLComponent(HistoryVolumeCurveAudioProcessor& p);
    ~HistoryOpenGLComponent() override;

    void paint(juce::Graphics&) override {}
    void resized() override {}

    void newOpenGLContextCreated() override;
    void openGLContextClosing() override;
    void renderOpenGL() override;

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override;

private:
    HistoryVolumeCurveAudioProcessor& processor;
    juce::OpenGLContext openGLContext;

    std::vector<float> vertexBuffer; // x,y pairs per pixel
    std::vector<float> sampleScratch;

    std::atomic<float> samplesPerPixel { 64.0f }; // zoom
    int msaaLevel = 4; // default 4x MSAA

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HistoryOpenGLComponent)
};

class HistoryVolumeCurveAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    HistoryVolumeCurveAudioProcessorEditor (HistoryVolumeCurveAudioProcessor&);
    ~HistoryVolumeCurveAudioProcessorEditor() override;

    void paint (juce::Graphics&) override {}
    void resized() override {}

private:
    HistoryOpenGLComponent glComp;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HistoryVolumeCurveAudioProcessorEditor)
};
