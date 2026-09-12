#include "BrowserRenderer.h"

#include <windows.h>
#include <shlwapi.h>
#include <wincodec.h>
#include <wrl.h>
#include <wrl/event.h>
#include <WebView2.h>

#include <algorithm>
#include <chrono>
#include <cstdio>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "windowscodecs.lib")

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace
{
constexpr UINT_PTR kTickTimer = 1;
constexpr UINT kTickMessage = WM_APP + 42;

std::wstring ToWide(const std::string& value)
{
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    std::wstring result(static_cast<size_t>(std::max(0, count)), L'\0');
    if (count > 1) MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), count);
    if (!result.empty()) result.pop_back();
    return result;
}

std::string HResultText(HRESULT hr)
{
    char result[32]{};
    std::snprintf(result, sizeof(result), "WebView2 error 0x%08lX", static_cast<unsigned long>(hr));
    return result;
}

} // namespace

class WebViewHost
{
public:
    explicit WebViewHost(BrowserRenderer& owner) : owner(owner) {}

    void Run()
    {
        const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(com)) { owner.SetStatus(HResultText(com)); return; }

        WNDCLASSW wc{};
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"BRBrowserSourceWebViewHost";
        wc.lpfnWndProc = WindowProc;
        RegisterClassW(&wc);

        // The host is off-screen and tool-window only. There is no visible browser UI.
        window = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, wc.lpszClassName,
            L"BR Browser Source", WS_POPUP, -32000, -32000, 1280, 720,
            nullptr, nullptr, wc.hInstance, this);
        if (!window) { owner.SetStatus("Cannot create hidden WebView2 host"); CoUninitialize(); return; }

        ShowWindow(window, SW_SHOWNOACTIVATE);
        owner.SetStatus("Starting WebView2 runtime...");
        const HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, UserDataPath().c_str(), nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [this](HRESULT result, ICoreWebView2Environment* created) -> HRESULT
                {
                    if (FAILED(result) || !created) { owner.SetStatus(HResultText(result)); return S_OK; }
                    environment = created;
                    return environment->CreateCoreWebView2Controller(window,
                        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                            [this](HRESULT controllerResult, ICoreWebView2Controller* createdController) -> HRESULT
                            {
                                if (FAILED(controllerResult) || !createdController) { owner.SetStatus(HResultText(controllerResult)); return S_OK; }
                                controller = createdController;
                                controller->get_CoreWebView2(&webview);
                                RECT bounds{0, 0, 1280, 720};
                                controller->put_Bounds(bounds);
                                controller->put_IsVisible(TRUE);
                                owner.SetStatus("Browser ready - enter URL then enable capture");
                                return S_OK;
                            }).Get());
                }).Get());
        if (FAILED(hr)) owner.SetStatus(HResultText(hr));

        SetTimer(window, kTickTimer, 16, nullptr);
        MSG message{};
        while (!owner.stopRequested.load() && GetMessageW(&message, nullptr, 0, 0) > 0)
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (window) DestroyWindow(window);
        CoUninitialize();
    }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        WebViewHost* self = reinterpret_cast<WebViewHost*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            self = static_cast<WebViewHost*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self && message == WM_TIMER && wParam == kTickTimer) { self->Tick(); return 0; }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    std::wstring UserDataPath() const
    {
        wchar_t temp[MAX_PATH]{};
        GetTempPathW(MAX_PATH, temp);
        std::wstring path = std::wstring(temp) + L"BRBrowserSource-WebView2";
        CreateDirectoryW(path.c_str(), nullptr);
        return path;
    }

    void Tick()
    {
        if (!webview) return;
        std::string url;
        bool enabled = false;
        bool reload = false;
        unsigned int fps = 30;
        {
            std::lock_guard<std::mutex> lock(owner.configMutex);
            url = owner.url; enabled = owner.captureEnabled; fps = owner.fps;
            reload = owner.reloadRequested; owner.reloadRequested = false;
        }
        if (url != loadedUrl)
        {
            loadedUrl = url;
            if (!url.empty()) { webview->Navigate(ToWide(url).c_str()); owner.SetStatus("Loading browser URL..."); }
        }
        if (reload && !url.empty()) { webview->Reload(); owner.SetStatus("Refreshing browser URL..."); }
        const auto now = std::chrono::steady_clock::now();
        const unsigned int interval = 1000 / std::max(1u, fps);
        if (!enabled || captureInFlight || now - lastCapture < std::chrono::milliseconds(interval)) return;
        lastCapture = now;
        Capture();
    }

    void Capture()
    {
        ComPtr<IStream> stream;
        stream.Attach(SHCreateMemStream(nullptr, 0));
        if (!stream) { owner.SetStatus("Cannot allocate browser capture stream"); return; }
        captureInFlight = true;
        const HRESULT hr = webview->CapturePreview(COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_PNG, stream.Get(),
            Callback<ICoreWebView2CapturePreviewCompletedHandler>(
                [this, stream](HRESULT result) -> HRESULT
                {
                    captureInFlight = false;
                    if (FAILED(result)) { owner.SetStatus(HResultText(result)); return S_OK; }
                    DecodePng(stream.Get());
                    return S_OK;
                }).Get());
        if (FAILED(hr)) { captureInFlight = false; owner.SetStatus(HResultText(hr)); }
    }

    void DecodePng(IStream* stream)
    {
        LARGE_INTEGER zero{}; stream->Seek(zero, STREAM_SEEK_SET, nullptr);
        ComPtr<IWICImagingFactory> factory;
        ComPtr<IWICBitmapDecoder> decoder;
        ComPtr<IWICBitmapFrameDecode> frame;
        ComPtr<IWICFormatConverter> converter;
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
        if (SUCCEEDED(hr)) hr = factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnLoad, &decoder);
        if (SUCCEEDED(hr)) hr = decoder->GetFrame(0, &frame);
        if (SUCCEEDED(hr)) hr = factory->CreateFormatConverter(&converter);
        if (SUCCEEDED(hr)) hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
        UINT width = 0, height = 0;
        if (SUCCEEDED(hr)) hr = converter->GetSize(&width, &height);
        std::vector<unsigned char> rgba(static_cast<size_t>(width) * height * 4);
        if (SUCCEEDED(hr)) hr = converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(rgba.size()), rgba.data());
        if (FAILED(hr)) { owner.SetStatus(HResultText(hr)); return; }
        {
            std::lock_guard<std::mutex> lock(owner.frameMutex);
            owner.latestFrame.rgba = std::move(rgba);
            owner.latestFrame.width = width;
            owner.latestFrame.height = height;
            ++owner.latestFrame.serial;
        }
        owner.SetStatus("Capturing browser frame");
    }

    BrowserRenderer& owner;
    HWND window = nullptr;
    ComPtr<ICoreWebView2Environment> environment;
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2> webview;
    std::string loadedUrl;
    bool captureInFlight = false;
    std::chrono::steady_clock::time_point lastCapture{};
};

BrowserRenderer::BrowserRenderer() = default;
BrowserRenderer::~BrowserRenderer() { Stop(); }
void BrowserRenderer::Start(unsigned int width, unsigned int height)
{
    if (running.exchange(true)) return;
    viewportWidth = width; viewportHeight = height; stopRequested = false;
    thread = std::thread([this] { Run(); });
}
void BrowserRenderer::Stop()
{
    stopRequested = true;
    const auto id = static_cast<DWORD>(threadId.load());
    if (id != 0) PostThreadMessageW(id, WM_QUIT, 0, 0);
    if (thread.joinable()) thread.join();
    running = false;
}
void BrowserRenderer::SetUrl(const std::string& value) { std::lock_guard<std::mutex> lock(configMutex); url = value; }
void BrowserRenderer::SetCaptureEnabled(bool value) { std::lock_guard<std::mutex> lock(configMutex); captureEnabled = value; }
void BrowserRenderer::SetFps(unsigned int value) { std::lock_guard<std::mutex> lock(configMutex); fps = value; }
void BrowserRenderer::RequestReload() { std::lock_guard<std::mutex> lock(configMutex); reloadRequested = true; }
bool BrowserRenderer::TakeLatestFrame(Frame& destination)
{
    std::lock_guard<std::mutex> lock(frameMutex);
    if (latestFrame.serial == 0 || latestFrame.serial == consumedSerial) return false;
    destination = latestFrame; consumedSerial = latestFrame.serial; return true;
}
std::string BrowserRenderer::Status() const { std::lock_guard<std::mutex> lock(statusMutex); return status; }
void BrowserRenderer::Run() { threadId = GetCurrentThreadId(); WebViewHost(*this).Run(); threadId = 0; }
void BrowserRenderer::SetStatus(const std::string& value) { std::lock_guard<std::mutex> lock(statusMutex); status = value; }
