# taiko wonder

A small C++17 / SDL2 rhythm-game prototype inspired by arcade taiko games. It is an original project: it does not include copyrighted songs, artwork, charts, or game assets.

## Build

Requirements: CMake 3.16+, a C++17 compiler, SDL2, SDL2_ttf. On Debian/Ubuntu, install `libsdl2-dev` and `libsdl2-ttf-dev`.

```sh
cmake -S . -B build
cmake --build build --parallel
./build/taiko-wonder
```

The game tries common system font paths for Japanese text. If none are installed, install a Japanese-capable font (for example Noto Sans CJK) and add its path to `fontCandidates` in `src/main.cpp`.

## Controls

- Title/menu: Up/Down, Enter, Escape
- Song selection: Up/Down selects a song; Left/Right changes the highlighted setting; Enter starts
- Drum: `F` / `J` = Don, `D` / `K` = Ka
- Results: Enter or Escape returns to the title
- Player settings: Enter edits the selected item; type a name and press Enter to save

Options include 1.0x–4.0x speed, Don/Ka swap (あべこべ), two random chart modes, and Auto. Auto is a solo practice option only. The included charts are generated practice patterns, not official charts.

## Online room server (early prototype)

On Unix-like systems, CMake also builds `taiko-wonder-server`. Run `./build/taiko-wonder-server [port]` (default 24816). It accepts one command per line over TCP; see [server/README.md](server/README.md). This is only a room-directory prototype: the SDL game does not yet connect to it, and live play, moderation, persistence, reconnects, and secure authentication are not implemented. Expose it to the internet only behind suitable network and abuse protections.

## Current scope

The playable prototype includes the title modes, generated-chart solo play, practice options, a three-song dojo-style course, score/results, and local player profile settings. Online play, official licensed content, and full arcade-accurate Dan-i rules are future work.
