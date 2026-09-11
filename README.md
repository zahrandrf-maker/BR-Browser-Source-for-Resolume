# BR Browser Source for Resolume

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
