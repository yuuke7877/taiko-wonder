# taiko wonder

C++17 / SDL2 rhythm-game prototype with an original colorful arcade UI. SimpleStyle and the supplied reference image are visual/organizational references only; no assets, fonts, sound, or charts are copied or bundled.

## Build

Requirements: CMake 3.21+, C++17 compiler, SDL2, and SDL2_ttf. Visual Studio 2022 users: see [BUILDING.md](BUILDING.md), then open the repository as a CMake project. A vcpkg manifest and Windows configure preset are included.

```sh
cmake --preset windows-vcpkg
cmake --build --preset windows-release
```

On Linux/macOS, use your SDL2 CMake packages: `cmake -S . -B build && cmake --build build --parallel`.

## Content folders

The game resolves content relative to the executable, not the current working directory. It creates `songs/` and `skins/` if absent. Add custom `.twn` charts to `songs/` and skin folders containing `skin.ini` to `skins/`; details and formats are in [songs/README.md](songs/README.md) and [skins/README.md](skins/README.md). With no custom skin, the generated default theme is used. SimpleStyle asset data is never loaded or copied.

## Features

- Arcade-style title, genre selection, local generated practice notes, profile screen, and pause/resume/restart.
- Seven genre categories; included chart names are original practice placeholders.
- Practice options: 1.0x–4.0x speed, Don/Ka swap, きまぐれ, でたらめ, and solo-only Auto.
- Dojo menu with generated practice course; not official Dan-i Dojo certification.
- UI screens and a local room directory prototype. Full real-time multiplayer synchronization and authoritative scoring are not implemented.

## Font

Set `TAIKO_WONDER_FONT` to a Japanese-capable `.ttf`, `.otf`, or `.ttc` file. A skin may also reference a relative font file in `skin.ini`. Font files are not bundled.
