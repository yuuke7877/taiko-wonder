# Fixing Visual Studio compile errors

Japanese text in `src/main.cpp` is UTF-8. The Visual Studio project now passes `/utf-8` so MSVC does not interpret UTF-8 literals using the active Windows code page. This avoids cascading parse errors such as “constant contains newline” and invalid string-literal suffixes.

For `SDL_ttf.h` not found or undefined `TTF_*` functions:

1. Install Visual Studio's **Desktop development with C++** workload.
2. Install vcpkg and set `VCPKG_ROOT` to the vcpkg directory.
3. Run `vcpkg install --triplet x64-windows` from the repository root, or reopen the project so manifest mode can install dependencies.
4. Confirm the selected project configuration is `x64` and vcpkg manifest integration is enabled, then reload and rebuild.

The manifest lists both `sdl2` and `sdl2-ttf`. If Visual Studio was open before dependencies were installed, close and reopen the project so include paths are refreshed.

The earlier reported `genreColors`, `center`, and `panel` errors are likely parser cascades from Japanese string encoding. Rebuild after the `/utf-8` change; if any remain, share the first error and its source line because later diagnostics may be secondary.
