#pragma once

#include <string>

#include <ffgl/FFGLPluginSDK.h>
#include <ffglex/FFGLScreenQuad.h>
#include <ffglex/FFGLShader.h>

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
    CFFGLShader shader;
    ffglex::FFGLScreenQuad quad;
    GLint accentLocation = -1;
    std::string browserUrl = "https://vdo.ninja/";
    std::string status = "Browser renderer: pending";
    bool captureEnabled = false;
    bool showCursor = true;
    bool audioEnabled = true;
    bool refreshRequested = false;
    float fpsSelector = 0.0f;
};
