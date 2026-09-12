# BR Browser Source for Resolume

## v0.2.0 — WebView2 capture build

This is a Windows x64 Resolume **Video Source** plugin. It hosts Microsoft Edge
WebView2 in an off-screen tool window, captures the rendered page as RGBA
frames, then outputs those frames directly to Resolume. No Chrome window and no
custom helper `.exe` are shown.

The settings panel contains the watermark **Belajar Resolume Browser Capture**,
a Browser URL field, Capture Enable, an FFGL dropdown for **15 FPS / 30 FPS /
60 FPS**, Show Cursor, Audio Enable, Refresh, and live Status.

### Important limits in this first runtime build

- **Microsoft Edge WebView2 Runtime is required** on the Windows computer. It is
  normally present on current Windows 10/11 installs. If it is absent, the
  Status field reports a WebView2 error instead of silently outputting black.
- The 60 FPS choice is exposed, but preview-frame capture can be CPU-heavy;
  30 FPS is the recommended setting for VDO.Ninja.
- FFGL transports video frames only. The **Audio Enable** control is retained
  for the final audio-routing layer, but it cannot put browser audio into
  Resolume by itself. Route VDO.Ninja audio separately (for example through a
  virtual audio device or your mixer).
- **Show Cursor** is included in the panel for compatibility. Native desktop
  cursor compositing is not implemented in the WebView2 capture path yet;
  webpage-rendered cursor states still appear normally.

### Build and install

1. Upload this folder to the root of a new GitHub repository.
2. In **Actions**, run **Build Windows x64 DLL** and download its artifact.
3. Copy `BR_Browser_Source_FFGL21.dll` to Resolume's **Extra Effects** folder,
   then restart Resolume.
4. Add **BR Browser Source** from **Sources**, paste a VDO.Ninja *view* URL,
   choose 30 FPS, tick Capture Enable, and wait for Status to read
   `Capturing browser frame`.

Windows FFGL 2.1 Video Source plugin for Resolume Arena/Avenue 7.

## Current milestone

This first repository build creates a loadable **BR Browser Source** with the agreed Resolume parameter panel:

- Browser URL
- Capture Enable
- FPS (30 / 60)
- Show Cursor
- Audio Enable
- Refresh
- Status

It currently renders a labelled placeholder texture. The next milestone replaces that placeholder with the embedded Chromium browser renderer, then connects browser audio to the Windows audio route.

## Build output

GitHub Actions builds `BR_Browser_Source_FFGL21.dll` for Windows x64. Put the DLL in:

`%USERPROFILE%\Documents\Resolume\Extra Effects`

Restart Resolume, then add **BR Browser Source** as a Video Source to a clip.

## Notes

- Target: Windows 10/11 x64, Resolume Arena/Avenue 7, FFGL 2.1.
- The browser itself is embedded in the plugin; Chrome/OBS will not be required after the renderer milestone is implemented.
- Audio is intentionally exposed as a parameter now but needs a separate browser-audio route; FFGL transports video texture only.
