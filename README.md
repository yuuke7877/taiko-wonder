# taiko wonder

C++17 / SDL2 rhythm-game prototype with an original bright arcade-style UI. The broad layout is inspired by the provided reference screenshot and SimpleStyle organization, but no image, font, sound, chart, or other asset from SimpleStyle is copied or loaded.

## Build

Requirements: CMake 3.16+, C++17 compiler, SDL2, SDL2_ttf.

```sh
cmake -S . -B build
cmake --build build --parallel
./build/taiko-wonder
```

### Japanese fonts

SDL_ttf supports TrueType (`.ttf`), OpenType (`.otf`), and TrueType Collection (`.ttc`) fonts. Install a font with Japanese glyphs, such as Noto Sans CJK JP, then set the font path:

```sh
TAIKO_WONDER_FONT=/path/to/font.otf ./build/taiko-wonder
```

On Windows, pass a font path as the optional `--font` argument:

```powershell
.\build\taiko-wonder.exe --font C:\\Windows\\Fonts\\meiryo.ttc
```

Common system font paths are also checked automatically. Only the path is used; font files are not bundled.

## Controls

- Title/attract: any drum key (`F`, `J`, `D`, `K`) or Enter starts; title demo display changes every 10 seconds.
- Mode selection: Up/Down, Enter, Escape.
- Song selection: Left/Right changes genre when the first chart is selected; Up/Down selects charts; Tab selects option; Left/Right changes options; Enter starts.
- Drums: `F` / `J` = Don, `D` / `K` = Ka.
- During performance: Escape opens pause menu. Up/Down selects, Enter confirms. Pause choices are resume, restart current chart, and return to genre/song selection.
- Player settings: Up/Down selects field; Enter edits name/epithet; Left/Right changes avatar/title; Escape saves.

## Features and scope

- Arcade-style title screen with left/right drum prompts and a centered FREE PLAY label, plus an attract/demo view that alternates every 10 seconds.
- Start-to-mode-selection flow and colorful genre-based song selection: Pops, Anime, Vocaloid, Game Music, Variety, Classical, and Namco Original. Included tracks are named original practice-chart placeholders with generated notes, not licensed songs or official charts.
- Practice options: 1.0x–4.0x speed, Don/Ka swap, きまぐれ, でたらめ, and Auto.
- Three-stage generated practice course in the dojo menu.
- Pause menu supports resume, restart current song, or return to genre/song selection.
- Profile fields for player name, appearance, title, and custom epithet.
- Online settings are UI-only. Real multiplayer/server connection is not implemented in this game client.

The visuals are independently drawn with SDL primitives and SDL_ttf. SimpleStyle is used only as visual/organizational reference; the game does not package or access its assets.
