\
#include "PluginEditor.h"
#include <cmath>

HistoryOpenGLComponent::HistoryOpenGLComponent(HistoryVolumeCurveAudioProcessor& p)
    : processor(p)
{
    setOpaque(true);
    openGLContext.setRenderer(this);

    // ask for MSAA via pixel format if supported
    juce::OpenGLPixelFormat pf;
    pf.multisamplingLevel = msaaLevel; // request 4x or 8x
    openGLContext.setPixelFormat(pf);

    openGLContext.attachTo(*this);
    openGLContext.setContinuousRepainting(true);
    startTimerHz(60);
}

HistoryOpenGLComponent::~HistoryOpenGLComponent()
{
    stopTimer();
    openGLContext.detach();
}

void HistoryOpenGLComponent::newOpenGLContextCreated()
{
    // nothing heavy to init for this simple example
}

void HistoryOpenGLComponent::openGLContextClosing()
{
    // cleanup if needed
}

void HistoryOpenGLComponent::timerCallback()
{
    // repaint continuously for smooth motion
    repaint();
}

void HistoryOpenGLComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    float s = samplesPerPixel.load();
    if (wheel.deltaY > 0)
        s = std::max(1.0f, s * 0.8f);
    else
        s = std::min(8192.0f, s * 1.25f);
    samplesPerPixel.store(s);
}

static inline float logScale(float in)
{
    // Map linear 0..1 to a perceptual/log-like curve still in 0..1.
    // Using log10(1 + 9*x) / log10(10) maps 0->0, 1->1 and emphasizes low values.
    return std::log10(1.0f + 9.0f * in) / std::log10(10.0f);
}

void HistoryOpenGLComponent::renderOpenGL()
{
    using namespace juce;
    OpenGLHelpers::clear(Colours::black);

    int w = getWidth();
    int h = getHeight();
    if (w <= 0 || h <= 0) return;

    float spp = samplesPerPixel.load();
    int pixels = std::max(2, w);
    int samplesToShow = (int)std::ceil(spp * pixels);

    sampleScratch.resize(samplesToShow);
    vertexBuffer.resize(pixels * 2);

    int available = processor.history.available();
    int need = samplesToShow;
    int startRelative = std::max(0, available - need);

    // read block (thread-safe copy)
    int copied = processor.history.readBlock(sampleScratch.data(), startRelative, need);
    if (copied < need)
        std::fill(sampleScratch.begin() + copied, sampleScratch.end(), 0.0f);

    // decimate (max) per pixel and apply log-scale amplitude
    for (int px = 0; px < pixels; ++px)
    {
        int s0 = (int)std::floor(px * spp);
        int s1 = (int)std::floor((px + 1) * spp);
        s0 = juce::jlimit(0, need - 1, s0);
        s1 = juce::jlimit(0, need - 1, s1);
        float maxv = 0.0f;
        for (int s = s0; s <= s1; ++s)
            maxv = std::max(maxv, sampleScratch[s]);

        float lv = logScale(maxv);
        float nx = (float)px / (float)(pixels - 1); // 0..1
        float ny = juce::jlimit(0.0f, 1.0f, lv);
        // map to -1..1 for GL coordinate later
        vertexBuffer[px*2 + 0] = nx;
        vertexBuffer[px*2 + 1] = ny;
    }

    // Setup simple orthographic projection and draw a smooth line using GL
    // We'll draw using immediate mode compatible functions for portability.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glLineWidth(2.0f);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.1f, 0.9f, 0.4f, 1.0f);
    glBegin(GL_LINE_STRIP);
    for (int px = 0; px < pixels; ++px)
    {
        float x = vertexBuffer[px*2 + 0] * (float)(w - 1);
        float y = vertexBuffer[px*2 + 1] * (float)(h - 1);
        glVertex2f(x, y);
    }
    glEnd();

    glPopMatrix(); // MODELVIEW
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Editor

HistoryVolumeCurveAudioProcessorEditor::HistoryVolumeCurveAudioProcessorEditor (HistoryVolumeCurveAudioProcessor& p)
    : AudioProcessorEditor (&p), glComp(p)
{
    setOpaque(true);
    addAndMakeVisible(glComp);
    setSize (800, 200);
}

HistoryVolumeCurveAudioProcessorEditor::~HistoryVolumeCurveAudioProcessorEditor()
{
}
