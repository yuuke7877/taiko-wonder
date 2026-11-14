# taiko wonder room directory

This C++ server provides a small TCP room-directory MVP for the Taiko Wonder client. It supports public listing and private four-digit join codes. It does **not** relay timed note inputs, synchronize a live chart, authenticate identity, verify scores, or provide TLS. It is not safe to expose directly as an Internet service; use a trusted network or put a properly authenticated TLS gateway in front of it.

## Build and run

```sh
cmake -S . -B build
cmake --build build --parallel
./build/taiko-wonder-server [port] [--public]
```

Default port is `24816`; by default the server binds only to `127.0.0.1`. `--public` binds all interfaces and should only be used with network-level protections. Rooms are held in memory and disappear when the server exits. The simple MVP has no expiry, persistent membership, or production abuse controls.

The SDL client uses `TAIKO_WONDER_SERVER=host:port` (default `127.0.0.1:24816`). The current protocol sends one command per TCP connection.

## Commands

- `PING` → `PONG`
- `LIST` → public `ROOM <code> <name> <players> <host> <mode> <baton-measures>` lines, ending with `END`
- `CREATE <public|private> <room> <host> <mode> <baton-measures>` → `CREATED <code> <visibility>`
- `JOIN <code>` → `JOINED <code> <player-count>`
- `LEAVE` → `OK`

Names and mode tokens are single-word ASCII in this protocol. The room server only advertises rooms; online gameplay and authoritative scoring remain prototype features, not a fully synchronized competitive multiplayer service.
