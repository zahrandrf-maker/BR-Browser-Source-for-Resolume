#include "BRBrowserSource.h"
#include <ffglex/FFGLScopedShaderBinding.h>

#include <cstdio>

using namespace ffglex;

namespace
{
    enum Parameter : FFUInt32
    {
        PT_BRANDING = 0,
        PT_BROWSER_URL,
        PT_CAPTURE_ENABLE,
        PT_FPS,
        PT_SHOW_CURSOR,
        PT_AUDIO_ENABLE,
        PT_REFRESH,
        PT_STATUS,
    };

    CFFGLPluginInfo PluginInfo(
        PluginFactory<BRBrowserSource>,
        "BRB1",
        "BR Browser Source",
        2, 1, 0, 100,
        FF_SOURCE,
        "Hidden WebView2 browser source for Resolume",
        "Belajar Resolume");

    bool IsPressed(float value) noexcept { return value >= 0.5f; }

    constexpr const char* vertexShaderCode = R"GLSL(
        #version 410 core
        layout(location = 0) in vec4 vPosition;
        layout(location = 1) in vec2 vUV;
        out vec2 uv;
        void main() { gl_Position = vPosition; uv = vUV; }
    )GLSL";

    constexpr const char* fragmentShaderCode = R"GLSL(
        #version 410 core
        in vec2 uv;
        uniform sampler2D browserTexture;
        uniform bool frameAvailable;
        out vec4 fragColor;
        void main()
        {
            if (frameAvailable) fragColor = texture(browserTexture, uv);
            else {
                float grid = step(0.985, fract(uv.x * 16.0)) + step(0.985, fract(uv.y * 9.0));
                vec3 base = mix(vec3(0.035, 0.05, 0.07), vec3(0.07, 0.12, 0.13), uv.y);
                fragColor = vec4(base + vec3(0.05, 0.42, 0.25) * grid, 1.0);
            }
        }
    )GLSL";
}

BRBrowserSource::BRBrowserSource()
{
    SetMinInputs(0);
    SetMaxInputs(0);
    SetParamInfo(PT_BRANDING, "BR Browser Source", FF_TYPE_TEXT, "Belajar Resolume Browser Capture");
    SetParamInfo(PT_BROWSER_URL, "Browser URL", FF_TYPE_TEXT, browserUrl.c_str());
    SetParamInfo(PT_CAPTURE_ENABLE, "Capture Enable", FF_TYPE_BOOLEAN, false);
    SetOptionParamInfo(PT_FPS, "FPS", 3, 30.0f);
    SetParamElementInfo(PT_FPS, 0, "15 FPS", 15.0f);
    SetParamElementInfo(PT_FPS, 1, "30 FPS", 30.0f);
    SetParamElementInfo(PT_FPS, 2, "60 FPS", 60.0f);
    SetParamInfo(PT_SHOW_CURSOR, "Show Cursor", FF_TYPE_BOOLEAN, true);
    SetParamInfo(PT_AUDIO_ENABLE, "Audio Enable", FF_TYPE_BOOLEAN, true);
    SetParamInfo(PT_REFRESH, "Refresh", FF_TYPE_EVENT, false);
    SetParamInfo(PT_STATUS, "Status", FF_TYPE_TEXT, status.c_str());
}

BRBrowserSource::~BRBrowserSource() = default;

FFResult BRBrowserSource::InitGL(const FFGLViewportStruct* viewport)
{
    if (!shader.Compile(vertexShaderCode, fragmentShaderCode) || !quad.Initialise())
        return FF_FAIL;

    ScopedShaderBinding binding(shader.GetGLID());
    textureLocation = shader.FindUniform("browserTexture");
    frameAvailableLocation = shader.FindUniform("frameAvailable");
    glGenTextures(1, &browserTexture);
    glBindTexture(GL_TEXTURE_2D, browserTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    renderer.SetUrl(browserUrl);
    renderer.SetFps(30);
    renderer.Start(viewport->width, viewport->height);
    return CFFGLPlugin::InitGL(viewport);
}

FFResult BRBrowserSource::ProcessOpenGL(ProcessOpenGLStruct*)
{
    BrowserRenderer::Frame frame;
    if (renderer.TakeLatestFrame(frame))
    {
        glBindTexture(GL_TEXTURE_2D, browserTexture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(frame.width),
            static_cast<GLsizei>(frame.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.rgba.data());
        hasFrame = true;
    }
    status = renderer.Status();
    ScopedShaderBinding binding(shader.GetGLID());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, browserTexture);
    glUniform1i(textureLocation, 0);
    glUniform1i(frameAvailableLocation, hasFrame ? 1 : 0);
    quad.Draw();
    return FF_SUCCESS;
}

FFResult BRBrowserSource::DeInitGL()
{
    renderer.Stop();
    if (browserTexture) glDeleteTextures(1, &browserTexture);
    browserTexture = 0;
    textureLocation = -1;
    frameAvailableLocation = -1;
    shader.FreeGLResources();
    quad.Release();
    return FF_SUCCESS;
}

FFResult BRBrowserSource::SetFloatParameter(unsigned int index, float value)
{
    switch (index)
    {
        case PT_CAPTURE_ENABLE: captureEnabled = IsPressed(value); renderer.SetCaptureEnabled(captureEnabled); return FF_SUCCESS;
        case PT_FPS:
            fpsSelector = value <= 15.0f ? 15.0f : (value >= 60.0f ? 60.0f : 30.0f);
            renderer.SetFps(static_cast<unsigned int>(fpsSelector)); return FF_SUCCESS;
        case PT_SHOW_CURSOR: showCursor = IsPressed(value); return FF_SUCCESS;
        case PT_AUDIO_ENABLE: audioEnabled = IsPressed(value); return FF_SUCCESS;
        case PT_REFRESH: if (IsPressed(value)) renderer.RequestReload(); return FF_SUCCESS;
        default: return FF_FAIL;
    }
}

float BRBrowserSource::GetFloatParameter(unsigned int index)
{
    switch (index)
    {
        case PT_CAPTURE_ENABLE: return captureEnabled ? 1.0f : 0.0f;
        case PT_FPS: return fpsSelector;
        case PT_SHOW_CURSOR: return showCursor ? 1.0f : 0.0f;
        case PT_AUDIO_ENABLE: return audioEnabled ? 1.0f : 0.0f;
        case PT_REFRESH: return 0.0f;
        default: return 0.0f;
    }
}

FFResult BRBrowserSource::SetTextParameter(unsigned int index, const char* value)
{
    if (index != PT_BROWSER_URL) return FF_FAIL;
    browserUrl = value ? value : "";
    renderer.SetUrl(browserUrl);
    status = browserUrl.empty() ? "Enter a browser URL" : "Browser URL queued";
    return FF_SUCCESS;
}

char* BRBrowserSource::GetTextParameter(unsigned int index)
{
    if (index == PT_BRANDING) return const_cast<char*>("Belajar Resolume Browser Capture");
    if (index == PT_BROWSER_URL) return const_cast<char*>(browserUrl.c_str());
    if (index == PT_STATUS) return const_cast<char*>(status.c_str());
    return nullptr;
}

char* BRBrowserSource::GetParameterDisplay(unsigned int index)
{
    static char buffer[32]{};
    if (index == PT_FPS)
    {
        std::snprintf(buffer, sizeof(buffer), "%u FPS", static_cast<unsigned int>(fpsSelector));
        return buffer;
    }
    return CFFGLPlugin::GetParameterDisplay(index);
}
