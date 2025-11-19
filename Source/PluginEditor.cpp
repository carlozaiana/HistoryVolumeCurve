#include "PluginProcessor.h"
#include "PluginEditor.h"

SmoothScopeAudioProcessorEditor::SmoothScopeAudioProcessorEditor (SmoothScopeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Initialize history buffer
    historyBuffer.resize(historySize, 0.0f);

    setResizable(true, true);
    setResizeLimits(300, 200, 2000, 1000);
    setSize (600, 300);

    // 60 FPS refresh rate for smoothness
    startTimerHz(60);
}

SmoothScopeAudioProcessorEditor::~SmoothScopeAudioProcessorEditor()
{
    stopTimer();
}

void SmoothScopeAudioProcessorEditor::timerCallback()
{
    // Pull data from Audio Processor's FIFO
    bool newData = false;
    
    // We loop to drain the FIFO so we don't fall behind
    while (true)
    {
        int currentRead = audioProcessor.fifoReadIndex.load(std::memory_order_acquire);
        int currentWrite = audioProcessor.fifoWriteIndex.load(std::memory_order_acquire);

        if (currentRead == currentWrite)
            break; // Empty

        // Get value
        float val = audioProcessor.fifoBuffer[currentRead];

        // Update Read Index
        int nextRead = (currentRead + 1) % SmoothScopeAudioProcessor::fifoSize;
        audioProcessor.fifoReadIndex.store(nextRead, std::memory_order_release);

        // Add to GUI History Circular Buffer
        historyBuffer[historyWriteIndex] = val;
        historyWriteIndex = (historyWriteIndex + 1) % historySize;
        
        newData = true;
    }

    if (newData)
        repaint();
}

void SmoothScopeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    auto area = getLocalBounds();
    float width = (float)area.getWidth();
    float height = (float)area.getHeight();
    float midY = height / 2.0f;

    // Visual Grid
    g.setColour(juce::Colours::darkgrey.withAlpha(0.3f));
    g.drawHorizontalLine((int)midY, 0.0f, width);

    // Create the Waveform Path
    juce::Path path;
    
    // We draw from Right to Left.
    // x = width means "Now". x = 0 means "History".
    // The 'zoomX' acts as pixel-per-sample spacing.
    
    // We need to find the start point (most recent sample)
    // The most recent sample was written at (historyWriteIndex - 1)
    
    int idx = historyWriteIndex - 1;
    if (idx < 0) idx = historySize - 1;

    // Calculate starting Y
    float sampleVal = historyBuffer[idx];
    // Scale volume by ZoomY, centered around midY
    // We use a log-ish scaling visual or simple linear. 
    // Simple linear for volume curve is safer for "no jumping peaks".
    float yPos = midY - (sampleVal * midY * 0.9f * zoomY); 
    
    // Clamp visual to screen bounds to prevent artifacts
    yPos = juce::jlimit(0.0f, height, yPos);

    path.startNewSubPath(width, yPos);

    // Iterate backwards through history
    for (int i = 1; i < historySize; ++i)
    {
        // Move index backwards
        idx--;
        if (idx < 0) idx = historySize - 1;

        // Calculate X position
        // i is "samples ago". 
        // spacing is determined by zoomX (e.g., 2.0f = 2 pixels per sample)
        float xPos = width - ((float)i * zoomX);

        // Optimization: Stop drawing if we go off the left edge
        if (xPos < -50.0f) break;

        // Calculate Y position
        float val = historyBuffer[idx];
        float y = midY - (val * midY * 0.9f * zoomY);
        y = juce::jlimit(0.0f, height, y);

        path.lineTo(xPos, y);
    }

    // Draw the curve
    g.setColour (juce::Colours::cyan);
    g.strokePath (path, juce::PathStrokeType (2.0f));
    
    // Overlay info
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText("Zoom X: " + juce::String(zoomX, 2) + " | Zoom Y: " + juce::String(zoomY, 2), 
               10, 10, 200, 20, juce::Justification::topLeft);
}

void SmoothScopeAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // Sensitivity logic
    float scrollAmount = wheel.deltaY;
    
    // If Shift is held, zoom Y (Amplitude), otherwise zoom X (Time)
    // Or we can map Y-scroll to Y-zoom and X-scroll to X-zoom if trackpad.
    // Requirement says: "zoom in and out the x and y axis with the mouse scroll wheel"
    
    // Let's assume:
    // Ctrl/Cmd + Scroll = Zoom X
    // Alt/Option + Scroll = Zoom Y
    // Or just standard Vertical Scroll = Zoom X, Horizontal Scroll = Zoom Y
    
    // Implementation for typical mouse usage:
    if (event.mods.isCommandDown() || event.mods.isCtrlDown())
    {
        // Zoom Y (Amplitude)
        zoomY += (scrollAmount * 1.0f);
        zoomY = juce::jlimit(minZoomY, maxZoomY, zoomY);
    }
    else
    {
        // Zoom X (Time Stretching)
        // Note: Increasing zoomX makes points further apart (zoom in time)
        // Decreasing makes them closer (zoom out time)
        zoomX += (scrollAmount * 2.0f);
        zoomX = juce::jlimit(minZoomX, maxZoomX, zoomX);
    }
    
    repaint();
}

void SmoothScopeAudioProcessorEditor::resized()
{
    // Standard resize handling, paint deals with bounds automatically
}