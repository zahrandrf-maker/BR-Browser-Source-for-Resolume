#pragma once

#include <string>
#include <vector>

#include <ffgl/FFGLPluginSDK.h>
#include <ffglex/FFGLScreenQuad.h>
#include <ffglex/FFGLShader.h>

#include "BrowserRenderer.h"

class BRBrowserSource final : public CFFGLPlugin
{
public:
    BRBrowserSource();
    ~BRBrowserSource() override;

    FFResult InitGL(const FFGLViewportStruct* viewport) override;
    FFResult ProcessOpenGL(ProcessOpenGLStruct* processStruct) override;
    FFResult DeInitGL() override;
    FFResult SetFloatParameter(unsigned int index, float value) override;
    float GetFloatParameter(unsigned int index) override;
    FFResult SetTextParameter(unsigned int index, const char* value) override;
    char* GetTextParameter(unsigned int index) override;
    char* GetParameterDisplay(unsigned int index) override;

private:
    ffglex::FFGLShader shader;
    ffglex::FFGLScreenQuad quad;
    GLint textureLocation = -1;
    GLint frameAvailableLocation = -1;
    GLuint browserTexture = 0;
    std::string browserUrl = "https://vdo.ninja/";
    std::string status = "WebView2 initializing";
    bool captureEnabled = false;
    bool showCursor = true;
    bool audioEnabled = true;
    float fpsSelector = 30.0f;
    bool hasFrame = false;
    BrowserRenderer renderer;
};
