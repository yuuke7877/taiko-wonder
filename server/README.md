# Room server protocol (prototype)

Build from the repository root with CMake, then run `./build/taiko-wonder-server [port]` (default port 24816). The server binds to all network interfaces. Use firewall rules and a trusted network; this prototype is not suitable for deployment as a public Internet service.

Clients send newline-delimited UTF-8 commands over TCP. Names are single words in this prototype.

- `HELP` — print supported commands.
- `LIST` — list public rooms, ending in `END` (`EMPTY` if none exist).
- `CREATE public <room-name> <host-name>` — create a public room.
- `CREATE private <room-name> <host-name>` — create a private room and return its generated four-digit code.
- `JOIN <4-digit-code>` — increment the room player count (up to 8).
- `LEAVE` — acknowledgment only.
- `QUIT` — close the connection.

The SDL client currently displays room configuration options but does not connect to this server. Online gameplay is not implemented: this directory server does not track sessions/membership, route gameplay messages, synchronize charts or options, validate scores, or expire rooms. Public deployment requires those features, transport security, abuse controls, and robust lifecycle management.
