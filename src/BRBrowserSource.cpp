#include "BRBrowserSource.h"

#include <cstdio>

using namespace ffglex;

namespace
{
    enum Parameter : FFUInt32
    {
        PT_BROWSER_URL = 0,
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
        "Embedded browser source for Resolume",
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
        uniform vec3 accent;
        out vec4 fragColor;
        void main()
        {
            float grid = step(0.985, fract(uv.x * 16.0)) + step(0.985, fract(uv.y * 9.0));
            vec3 base = mix(vec3(0.035, 0.05, 0.07), vec3(0.07, 0.12, 0.13), uv.y);
            fragColor = vec4(base + accent * grid * 0.42, 1.0);
        }
    )GLSL";
}

BRBrowserSource::BRBrowserSource()
{
    SetMinInputs(0);
    SetMaxInputs(0);
    SetParamInfo(PT_BROWSER_URL, "Browser URL", FF_TYPE_TEXT, browserUrl.c_str());
    SetParamInfo(PT_CAPTURE_ENABLE, "Capture Enable", FF_TYPE_BOOLEAN, false);
    SetParamInfo(PT_FPS, "FPS", FF_TYPE_STANDARD, 0.0f);
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
    accentLocation = shader.FindUniform("accent");
    return CFFGLPlugin::InitGL(viewport);
}

FFResult BRBrowserSource::ProcessOpenGL(ProcessOpenGLStruct*)
{
    ScopedShaderBinding binding(shader.GetGLID());
    const float intensity = captureEnabled ? 1.0f : 0.35f;
    glUniform3f(accentLocation, 0.15f * intensity, 0.85f * intensity, 0.55f * intensity);
    quad.Draw();
    return FF_SUCCESS;
}

FFResult BRBrowserSource::DeInitGL()
{
    accentLocation = -1;
    shader.FreeGLResources();
    quad.Release();
    return FF_SUCCESS;
}

FFResult BRBrowserSource::SetFloatParameter(unsigned int index, float value)
{
    switch (index)
    {
        case PT_CAPTURE_ENABLE: captureEnabled = IsPressed(value); status = captureEnabled ? "Capture enabled - renderer pending" : "Capture disabled"; return FF_SUCCESS;
        case PT_FPS: fpsSelector = value >= 0.5f ? 1.0f : 0.0f; return FF_SUCCESS;
        case PT_SHOW_CURSOR: showCursor = IsPressed(value); return FF_SUCCESS;
        case PT_AUDIO_ENABLE: audioEnabled = IsPressed(value); return FF_SUCCESS;
        case PT_REFRESH: if (IsPressed(value)) { refreshRequested = true; status = "Refresh queued"; } return FF_SUCCESS;
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
    status = browserUrl.empty() ? "Enter a browser URL" : "URL ready";
    return FF_SUCCESS;
}

char* BRBrowserSource::GetTextParameter(unsigned int index)
{
    if (index == PT_BROWSER_URL) return const_cast<char*>(browserUrl.c_str());
    if (index == PT_STATUS) return const_cast<char*>(status.c_str());
    return nullptr;
}

char* BRBrowserSource::GetParameterDisplay(unsigned int index)
{
    static char buffer[32]{};
    if (index == PT_FPS)
    {
        std::snprintf(buffer, sizeof(buffer), "%s", fpsSelector >= 0.5f ? "60 FPS" : "30 FPS");
        return buffer;
    }
    return CFFGLPlugin::GetParameterDisplay(index);
}
