#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

//==============================================================================
// Helper: map linear 0..1 to a perceptual/log-like curve 0..1
static inline float logScale (float in)
{
    // log10(1 + 9x) / log10(10) maps 0 -> 0, 1 -> 1 and emphasizes low values
    return std::log10 (1.0f + 9.0f * in) / std::log10 (10.0f);
}

//==============================================================================
// HistoryOpenGLComponent (now purely 2D Graphics-based)

HistoryOpenGLComponent::HistoryOpenGLComponent (HistoryVolumeCurveAudioProcessor& p)
    : processor (p)
{
    setOpaque (true);
    startTimerHz (60); // repaint at ~60 fps
}

HistoryOpenGLComponent::~HistoryOpenGLComponent()
{
    stopTimer();
}

void HistoryOpenGLComponent::timerCallback()
{
    repaint();
}

void HistoryOpenGLComponent::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    float s = samplesPerPixel.load();

    if (wheel.deltaY > 0)
        s = std::max (1.0f, s * 0.8f);
    else
        s = std::min (8192.0f, s * 1.25f);

    samplesPerPixel.store (s);
}

void HistoryOpenGLComponent::paint (juce::Graphics& g)
{
    using namespace juce;

    auto bounds = getLocalBounds().toFloat();
    const int w = (int) bounds.getWidth();
    const int h = (int) bounds.getHeight();

    g.fillAll (Colours::black);

    if (w <= 1 || h <= 1)
        return;

    const float spp    = samplesPerPixel.load(); // samples per pixel
    const int   pixels = std::max (2, w);
    const int   need   = (int) std::ceil (spp * pixels);

    sampleScratch.resize (need);

    const int available     = processor.history.available();
    const int startRelative = std::max (0, available - need);

    // Copy history into a temporary buffer
    const int copied = processor.history.readBlock (sampleScratch.data(), startRelative, need);
    if (copied < need)
        std::fill (sampleScratch.begin() + copied, sampleScratch.end(), 0.0f);

    Path path;
    bool started = false;

    for (int px = 0; px < pixels; ++px)
    {
        int s0 = (int) std::floor (px * spp);
        int s1 = (int) std::floor ((px + 1) * spp);

        s0 = jlimit (0, need - 1, s0);
        s1 = jlimit (0, need - 1, s1);

        float maxv = 0.0f;
        for (int s = s0; s <= s1; ++s)
            maxv = std::max (maxv, sampleScratch[(size_t) s]);

        const float lv = logScale (maxv);
        const float ny = jlimit (0.0f, 1.0f, lv);

        const float x = (float) px;
        // JUCE y=0 is top; we want 0 at bottom, 1 at top
        const float y = jmap (ny, 0.0f, 1.0f, (float) h - 1.0f, 0.0f);

        if (! started)
        {
            path.startNewSubPath (x, y);
            started = true;
        }
        else
        {
            path.lineTo (x, y);
        }
    }

    g.setColour (Colour::fromFloatRGBA (0.1f, 0.9f, 0.4f, 1.0f));
    g.strokePath (path, PathStrokeType (2.0f));
}

//==============================================================================
// Editor

HistoryVolumeCurveAudioProcessorEditor::HistoryVolumeCurveAudioProcessorEditor (HistoryVolumeCurveAudioProcessor& p)
    : AudioProcessorEditor (&p), glComp (p)
{
    setOpaque (true);
    addAndMakeVisible (glComp);
    setSize (800, 200);
}

HistoryVolumeCurveAudioProcessorEditor::~HistoryVolumeCurveAudioProcessorEditor() = default;

void HistoryVolumeCurveAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void HistoryVolumeCurveAudioProcessorEditor::resized()
{
    glComp.setBounds (getLocalBounds());
}