# Visual Studio SDL2 setup

The Visual Studio project file is valid XML and configured for an SDL2 development bundle at `C:\SDL2`:

- Headers: `C:\SDL2\include\SDL2` and `C:\SDL2\include`
- x64 libraries: `C:\SDL2\lib\x64`
- Link inputs: `SDL2.lib` and `SDL2_ttf.lib`

Check that these exact files exist (some packages put headers directly in `include`):

```text
C:\SDL2\include\SDL2\SDL.h   (or C:\SDL2\include\SDL.h)
C:\SDL2\include\SDL2\SDL_ttf.h (or C:\SDL2\include\SDL_ttf.h)
C:\SDL2\lib\x64\SDL2.lib
C:\SDL2\lib\x64\SDL2_ttf.lib
```

Use the x64 configuration. The project enables `/utf-8` for Japanese source literals and does not link `SDL2main.lib` because `SDL_MAIN_HANDLED` is used.
