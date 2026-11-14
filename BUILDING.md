# Visual Studio SDL2 setup

The `.vcxproj` is configured for an SDL2 development bundle installed at `C:\SDL2`:

- Headers: `C:\SDL2\include` (also checks `include\SDL2`)
- x64 libraries: `C:\SDL2\lib\x64`
- Link inputs: `SDL2.lib` and `SDL2_ttf.lib`

Ensure these files actually exist:

```text
C:\SDL2\include\SDL.h
C:\SDL2\include\SDL_ttf.h
C:\SDL2\lib\x64\SDL2.lib
C:\SDL2\lib\x64\SDL2_ttf.lib
```

Some SDL2 bundles put headers in `include\SDL2`; the project includes both layouts. SDL_ttf is a separate development package from SDL2, so if `SDL_ttf.h` or `SDL2_ttf.lib` is missing, install SDL2_ttf for the same x64 toolchain and place its include/lib files under `C:\SDL2`, or edit the corresponding project include/library paths to its actual installation location. Do not enter a directory path such as `C:\SDL2\lib\x64.obj` as a library: the library directory must be a folder, and linker inputs are filenames ending in `.lib`.

The project uses `SDL_MAIN_HANDLED`, so it does not link `SDL2main.lib`. Save Japanese C++ source as UTF-8; `/utf-8` is enabled in the project.
