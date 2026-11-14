# taiko wonder

C++17 / SDL2 rhythm-game prototype with original generated practice charts and original graphics. It does not include copyrighted songs, official charts, or Taiko no Tatsujin assets. The `simplestyle` skin was not included in the repository at implementation time; the UI uses a simple original palette and can be adapted when the reference files are provided.

## Build

Requirements: CMake 3.16+, C++17 compiler, SDL2, SDL2_ttf. On Debian/Ubuntu install `libsdl2-dev` and `libsdl2-ttf-dev`.

```sh
cmake -S . -B build
cmake --build build --parallel
./build/taiko-wonder
```

The game searches common Japanese font locations. Install a Japanese-capable font (such as Noto Sans CJK) if Japanese text is missing.

## Controls

- Title: Up/Down, Enter
- Song selection: Up/Down chooses a practice chart; Tab selects an option; Left/Right changes it; Enter starts
- Drum: `F` / `J` = Don, `D` / `K` = Ka; Escape exits a song
- Player settings: Up/Down chooses a field; Left/Right cycles the avatar/title; Enter edits name or epithet; Escape saves
- Online settings: Up/Down selects; Left/Right changes visibility/mode/baton measure; Escape returns

## Included prototype features

- Title choices for play, dojo practice, online settings, and player settings. The AI battle mode was omitted per the request's explicit “なしで” note.
- Solo chart practice with 1.0x–4.0x speed, Don/Ka swap (あべこべ), きまぐれ / でたらめ pattern variation, and Auto. Auto is explicitly disabled for online play.
- Three-stage generated dojo practice course. It is not an official Dan-i Dojo implementation and does not include official songs/charts or certification rules.
- Local profile persistence for player name, avatar, title, and user-entered epithet in `taiko-wonder.profile`.
- An online room settings screen with public/private selection, room type, and baton measure configuration.

## Online server status

CMake builds `taiko-wonder-server` on Unix-like systems. It is a room-directory prototype (see [server/README.md](server/README.md)): it does not yet provide client game networking, song/chart synchronization, room lifecycle, secure identity, or online scoring. Do not expose it as a production public service without implementing those protections and features. A game client/server protocol and deployment host still need to be added for actual online multiplayer.
