#include "PluginEditor.h"

VolumeGraph::VolumeGraph (SmoothVolumeHistoryProcessor& p) : processor (p)
{
    setOpaque (true);
    startTimerHz (60);                                   // ultra-smooth repaint
}

void VolumeGraph::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0c0e12));

    auto area = getLocalBounds().reduced (20, 10);
    const int w = area.getWidth();
    const int h = area.getHeight();
    if (w < 20 || h < 20) return;

    std::deque<float> levels;
    double sr = 48000.0;

    {
        juce::ScopedLock sl (processor.levelLock);
        levels = processor.levelHistory;
        sr = processor.sampleRate > 0 ? processor.sampleRate : 48000.0;
    }

    if (levels.size() < 2) return;

    const double pointsPerSec = sr / subBlockSize;
    const double targetPoints  = visibleSeconds * pointsPerSec;
    const double pixelsPerPoint = (double)w / targetPoints;

    const int numPoints = (int)levels.size();
    const int pointsToShow = juce::jmin (numPoints, (int)std::ceil(targetPoints) + 100);

    juce::Path envelope;
    bool first = true;
    float lastY = (float)h;

    for (int i = 0; i < pointsToShow; ++i)
    {
        const int idx = numPoints - pointsToShow + i;
        const float db = levels[idx];

        const float distFromRight = (float)(pointsToShow - 1 - i);
        float x = (float)w - distFromRight * (float)pixelsPerPoint;

        if (x < -50) continue;
        if (x > w + 50) break;

        const float y = juce::jmap (db, yMinDb, yMaxDb, (float)h, 0.0f);

        if (first)
        {
            envelope.startNewSubPath (x, (float)h);
            first = false;
        }
        envelope.lineTo (x, y);
        lastY = y;
    }

    envelope.lineTo ((float)w, (float)h);
    envelope.closeSubPath();

    // Filled envelope
    g.setGradientFill (juce::ColourGradient (juce::Colour(0xff00ff41).withAlpha(0.65f), 0, (float)h,
                                             juce::Colour(0xff004d1c).withAlpha(0.75f), 0, 0, false));
    g.fillPath (envelope);

    // Bright top line
    g.setColour (juce::Colour(0xff00ff80));
    g.strokePath (envelope, juce::PathStrokeType (2.5f, juce::PathStrokeType::beveled, juce::PathStrokeType::rounded));
}

void VolumeGraph::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (std::abs (wheel.deltaY) < 0.01f) return;

    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        // ==== Horizontal (time) zoom ====
        const double factor = wheel.deltaY > 0 ? 0.8 : 1.25;
        visibleSeconds = juce::jlimit (0.05, 600.0, visibleSeconds * factor);
    }
    else
    {
        // ==== Vertical (dB) zoom at mouse position ====
        const float pos = (float)e.y / getHeight();   // 0.0 = top, 1.0 = bottom
        const double currentDb = juce::jmap (1.0 - pos, yMinDb, yMaxDb);

        double range = yMaxDb - yMinDb;
        const double factor = wheel.deltaY > 0 ? 0.8 : 1.25;
        range = juce::jlimit (3.0, 150.0, range * factor);

        yMinDb = currentDb - (1.0 - pos) * range;
        yMaxDb = currentDb + pos * range;
    }

    repaint();
}

//==============================================================================
SmoothVolumeHistoryEditor::SmoothVolumeHistoryEditor (SmoothVolumeHistoryProcessor& p)
    : AudioProcessorEditor (&p), graph (p)
{
    addAndMakeVisible (graph);
    setSize (900, 400);
    setResizable (true, false);
}

void SmoothVolumeHistoryEditor::resized()
{
    graph.setBounds (getLocalBounds());
}