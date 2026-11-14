# taiko wonder

C++17 / SDL2 rhythm-game prototype with an original colorful arcade UI. SimpleStyle and the supplied reference image are visual/organizational references only; no assets, fonts, sound, or charts are copied or bundled.

## Build

```sh
cmake -S . -B build
cmake --build build --parallel
./build/taiko-wonder
```

Requirements are CMake 3.16+, a C++17 compiler, SDL2, and SDL2_ttf.

## Features

- Arcade-style title screen, attract display, genre selection, local generated practice notes, profile screen, and pause/resume/restart flow.
- Seven genre categories; charts are generated original practice patterns, not official or licensed songs.
- Practice options: 1.0x–4.0x speed, Don/Ka swap, きまぐれ, でたらめ, and Auto. Auto is a solo-only option.
- Dojo menu with course/rank choices and generated multi-song practice. This is not official Dan-i Dojo certification.
- On Unix, `taiko-wonder-server` and a TCP room-directory client are built. Online UI supports creating public/private rooms, public room listing, joining with a room code, mode selection, and baton measure settings. Server/client details and limitations are in [server/README.md](server/README.md).
- Original synthesized UI/drum feedback and a configurable font path (where supported).

## Font

Set `TAIKO_WONDER_FONT` to a Japanese-capable `.ttf`, `.otf`, or `.ttc` file. On Unix, the client tries common font paths as a fallback. The game does not package font files.

## Online limitations

The server connection is real for room directory operations. This is not yet a full synchronized online match: notes are generated locally, gameplay events are not relayed or time-synchronized, and submitted scores are not server-validated. The server defaults to localhost and is not production-ready for public Internet exposure.
