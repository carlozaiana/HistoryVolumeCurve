#include "PluginProcessor.h"
#include "PluginEditor.h"

#if JUCE_WINDOWS
 #include <Windows.h>
#endif

// Simple vertex + fragment shader (GLSL)
static const char* vertexShaderSource = R"(
    #version 120
    attribute vec2 position;
    uniform float xOffset;
    uniform float xScale;
    void main()
    {
        float x = (position.x - xOffset) * xScale * 2.0 - 1.0;
        gl_Position = vec4(x, position.y, 0.0, 1.0);
    }
)";

static const char* fragmentShaderSource = R"(
    #version 120
    uniform vec4 colour;
    void main()
    {
        gl_FragColor = colour;
    }
)";

//==============================================================================
// HistoryOpenGLRenderer
HistoryOpenGLRenderer::HistoryOpenGLRenderer(HistoryVolumeAudioProcessor& p)
    : processor(p)
{
    setSize(800, 200);
    openGLContext.attachTo(*this);
    openGLContext.setRenderer(this);
    openGLContext.setContinuousRepainting(true); // continuous animation for smooth scrolling
    startTimerHz(60); // fallback timer for updates (60 Hz)
}

HistoryOpenGLRenderer::~HistoryOpenGLRenderer()
{
    stopTimer();
    openGLContext.detach();
}

void HistoryOpenGLRenderer::paint(juce::Graphics& g)
{
    // Nothing - GL paints
    g.fillAll(juce::Colours::black);
}

void HistoryOpenGLRenderer::resized()
{
}

void HistoryOpenGLRenderer::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    // zoom in/out with wheel: scale samplesPerPixel
    float s = samplesPerPixel.load();
    if (wheel.deltaY > 0)
        s = std::max(1.0f, s * 0.8f);
    else
        s = std::min(8192.0f, s * 1.25f);

    samplesPerPixel.store(s);
}

void HistoryOpenGLRenderer::newOpenGLContextCreated()
{
    // create shader program
    auto createShader = [](GLenum type, const char* src)->GLuint {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        // minimal error check (can expand)
        GLint compiled;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char buf[1024];
            glGetShaderInfoLog(shader, 1024, nullptr, buf);
            DBG("Shader compile error: " << buf);
        }
        return shader;
    };

    GLuint vert = createShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint frag = createShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vert);
    glAttachShader(shaderProgram, frag);
    glLinkProgram(shaderProgram);

    // attributes / uniforms
    attrPos = glGetAttribLocation(shaderProgram, "position");
    uniOffset = glGetUniformLocation(shaderProgram, "xOffset");
    uniScale = glGetUniformLocation(shaderProgram, "xScale");
    uniColor = glGetUniformLocation(shaderProgram, "colour");

    // VBO
    glGenBuffers(1, &vbo);

    // temp samples buffer
    tempSamples.resize(getWidth() * 2 + 16);
}

void HistoryOpenGLRenderer::openGLContextClosing()
{
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (shaderProgram) { glDeleteProgram(shaderProgram); shaderProgram = 0; }
}

void HistoryOpenGLRenderer::timerCallback()
{
    // nothing heavy here; openGLContext will call renderOpenGL
    // we could update scrollPositionSeconds here for smooth speed if needed
}

void HistoryOpenGLRenderer::renderOpenGL()
{
    jassert(OpenGLHelpers::isContextActive());
    const float w = (float) getWidth();
    const float h = (float) getHeight();
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // compute how many history samples correspond to viewport width given current zoom
    const double sr = processor.getSampleRate();
    const int historySize = processor.getHistorySize();
    const int totalWritten = processor.getNumWritten();
    const float spp = samplesPerPixel.load(); // samples per pixel
    const int pixels = juce::jmax(2, getWidth());
    const int samplesToShow = (int) std::ceil(spp * pixels);

    // allocate tempSamples for decimated data (one sample per pixel)
    if ((int)tempSamples.size() < pixels*2+16)
        tempSamples.resize(pixels*2+16);

    // read latest samples: we want the most recent samplesToShow, ending at totalWritten
    int available = std::min(totalWritten, historySize);
    int needed = samplesToShow;
    int startRelative = std::max(0, available - needed); // relative to oldest
    std::vector<float> readBuf;
    readBuf.resize(needed);

    int copied = processor.readHistorySamples(readBuf.data(), (int)readBuf.size(), startRelative, needed);
    if (copied < needed)
        for (int i = copied; i < needed; ++i) readBuf[i] = 0.0f;

    // Downsample into one value per pixel using max (min/max gives envelope)
    for (int px = 0; px < pixels; ++px)
    {
        int s0 = (int) std::floor(px * spp);
        int s1 = (int) std::floor((px + 1) * spp);
        s0 = juce::jlimit(0, needed-1, s0);
        s1 = juce::jlimit(0, needed-1, s1);
        float maxv = 0.0f;
        if (s1 >= s0)
        {
            for (int s = s0; s <= s1; ++s) maxv = std::max(maxv, readBuf[s]);
        }
        float nx = (float) px / (float) (pixels - 1); // 0..1
        float ny = juce::jlimit(-1.0f, 1.0f, (maxv * 2.0f - 1.0f)); // map to -1..1; tweak if you want log scale
        // store x,y
        tempSamples[px*2 + 0] = nx;
        tempSamples[px*2 + 1] = ny;
    }

    // Build vertex data: triangle strip for a filled curve or line strip for polyline
    // We'll do a line strip for simplicity (anti-alias via GL line smoothing if available)
    vertexData.clear();
    vertexData.insert(vertexData.end(), tempSamples.begin(), tempSamples.begin() + pixels*2);

    // Upload to VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertexData.size() * sizeof(float)), vertexData.data(), GL_DYNAMIC_DRAW);

    // Render with shader
    glUseProgram(shaderProgram);
    // uniforms: offset and scale to control horizontal mapping and scrolling
    float offset = 0.0f;
    float xScale = 1.0f; // we already normalized x to 0..1; scaling kept 1
    glUniform1f(uniOffset, offset);
    glUniform1f(uniScale, xScale);
    glUniform4f(uniColor, 0.1f, 0.9f, 0.4f, 1.0f); // greenish

    // Enable attribute
    glEnableVertexAttribArray((GLuint)attrPos);
    glVertexAttribPointer((GLuint)attrPos, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Draw line strip
    glLineWidth(2.0f);
    glDrawArrays(GL_LINE_STRIP, 0, pixels);

    glDisableVertexAttribArray((GLuint)attrPos);
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
