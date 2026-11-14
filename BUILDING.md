## SDL2 folder layout

The supplied SDL2 headers are located directly in `C:\SDL2` (for example `C:\SDL2\SDL.h`), not under `C:\SDL2\include`. The Visual Studio project now searches `C:\SDL2` as well as common nested include paths. The x64 libraries are expected under `C:\SDL2\lib\x64`.

Confirm these files exist:

```text
C:\SDL2\SDL.h
C:\SDL2\SDL_ttf.h
C:\SDL2\lib\x64\SDL2.lib
C:\SDL2\lib\x64\SDL2_ttf.lib
```

SDL2_ttf may be a separate development package; if any of its files are missing, install that package and set the include/library paths to its actual locations. In Visual Studio select x64, close and reopen the project, then Clean and Rebuild.
