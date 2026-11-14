# Room server protocol (prototype)

Build from the repository root with CMake, then run `./build/taiko-wonder-server [port]`. The default port is 24816. The server binds to all network interfaces. Use firewall rules and a trusted network until authentication and abuse protection are added.

Each client sends newline-delimited UTF-8 commands. Responses are newline-delimited text. Names are currently single words (no spaces).

- `HELP` — print supported commands.
- `LIST` — list public rooms, ending in `END` (`EMPTY` is sent if none exist).
- `CREATE public <room-name> <host-name>` — create a public room.
- `CREATE private <room-name> <host-name>` — create a room whose generated four-digit code is returned to the host.
- `JOIN <code>` — increment the room's player count (up to 8).
- `LEAVE` — acknowledgment only; membership tracking is not implemented yet.
- `QUIT` — close the connection.

This is an early room-directory demonstration, not a production-ready public service. In particular, rooms do not expire, clients are not associated with rooms after joining, and there is no gameplay relay, authorization, TLS, rate limiting, or persistence. Do not treat a room code as authentication.
