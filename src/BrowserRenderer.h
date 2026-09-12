#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class WebViewHost;

// BrowserRenderer owns the WebView2 thread. It is deliberately separate from
// the OpenGL thread used by Resolume: WebView2 requires an STA message loop,
// while OpenGL uploads must happen on Resolume's render thread.
class BrowserRenderer
{
public:
    struct Frame
    {
        std::vector<unsigned char> rgba;
        unsigned int width = 0;
        unsigned int height = 0;
        unsigned long long serial = 0;
    };

    BrowserRenderer();
    ~BrowserRenderer();

    void Start(unsigned int width, unsigned int height);
    void Stop();
    void SetUrl(const std::string& url);
    void SetCaptureEnabled(bool enabled);
    void SetFps(unsigned int fps);
    void RequestReload();
    bool TakeLatestFrame(Frame& destination);
    std::string Status() const;
    // Used by the WebView2 STA host to publish human-readable diagnostics.
    void SetStatus(const std::string& value);

private:
    friend class WebViewHost;
    void Run();

    mutable std::mutex configMutex;
    std::string url;
    bool captureEnabled = false;
    unsigned int fps = 30;
    bool reloadRequested = false;
    unsigned int viewportWidth = 1280;
    unsigned int viewportHeight = 720;

    mutable std::mutex frameMutex;
    Frame latestFrame;
    unsigned long long consumedSerial = 0;

    mutable std::mutex statusMutex;
    std::string status = "WebView2 not started";
    std::thread thread;
    std::atomic<unsigned long> threadId{0};
    std::atomic<bool> running{false};
    std::atomic<bool> stopRequested{false};
};
