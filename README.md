# taiko wonder

A small C++17 / SDL2 rhythm-game prototype inspired by the overall layout and structure of the reference skin set in `SimpleStyle` without copying its assets or textures. The project intentionally keeps to original code, original rectangles, and original text rendering while using the reference layout as a stylistic guide.

## Build

Requirements: CMake 3.16+, a C++17 compiler, SDL2, and SDL2_ttf. On Debian/Ubuntu: `sudo apt install libsdl2-dev libsdl2-ttf-dev`

```sh
cmake -S . -B build
cmake --build build --parallel
./build/taiko-wonder
```

## Controls

- Title menu: Up/Down, Enter
- Song select: Up/Down, Tab, Left/Right, Enter, Escape
- Dojo course: Up/Down, Enter, Escape
- Drum play: `F` / `J` = Don, `D` / `K` = Ka, `Esc` ends the song
- Player settings: Up/Down changes fields; Enter edits the selected field; Left/Right changes avatar/title

## Included features

- Title menu with modes: 演奏モード, 段位道場, オンライン, プレイヤー設定
- Practice mode with speed control from 1.0x to 4.0x in 0.5x steps
- `あべこべ` note swap mode
- Random chart modes: `きまぐれ` and `でたらめ`
- Auto practice for local play only
- Dojo-style multi-stage progression with generated practice charts
- Player customization screen: name, appearance, title, epithet
- Online room setup screen with public/private room options and baton interval config

## SimpleStyle reference note

The project uses the folder structure and visual ideas from the reference skin set under `SimpleStyle` as inspiration only. No assets, textures, music, or font files from that directory are copied, redistributed, or re-used. This implementation keeps the game fully self-contained and original.
