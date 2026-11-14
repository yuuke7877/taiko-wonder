# Visual Studio SDL2 setup

The Visual Studio project is configured for the SDL2 development files at `C:\SDL2`:

- Include directories: `C:\SDL2\include` and `C:\SDL2\include\SDL2`
- x64 library directory: `C:\SDL2\lib\x64`
- Required libraries: `SDL2.lib` and `SDL2_ttf.lib`

Verify these files exist before building:

```text
C:\SDL2\include\SDL.h              (or include\SDL2\SDL.h)
C:\SDL2\include\SDL_ttf.h          (or include\SDL2\SDL_ttf.h)
C:\SDL2\lib\x64\SDL2.lib
C:\SDL2\lib\x64\SDL2_ttf.lib
```

If `SDL2_ttf.lib` or its headers are missing, install the SDL2_ttf development package for the same x64 architecture. In Visual Studio, select **x64**, reload `taiko-wonder.vcxproj`, then Clean and Rebuild. Japanese source is compiled with `/utf-8`.

The project defines `SDL_MAIN_HANDLED`, so it uses the standard `main()` entry point and does not require `SDL2main.lib`. Do not add a path ending in `.obj` to Additional Library Directories; it must be a directory path.
