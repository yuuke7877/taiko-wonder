# Visual Studio 2022 build

1. Install Visual Studio 2022 with **Desktop development with C++** and **C++ CMake tools for Windows**.
2. Install vcpkg and set `VCPKG_ROOT` to its install folder.
3. From the repository root, run `vcpkg install --triplet x64-windows` or open this folder in Visual Studio and select the `windows-vcpkg` configure preset.
4. In Visual Studio choose the `windows-vcpkg` configure preset and build the `taiko-wonder` target in Release or Debug.

SDL2 and SDL2_ttf are acquired through the repository's vcpkg manifest. The game is Windows-compatible. The optional TCP room server currently builds on Unix-like systems only; its POSIX socket implementation is not part of the Windows game target.

When launching the executable, `songs/` and `skins/` are created/read next to the executable. Custom charts and skins should be placed there. See their README files.
