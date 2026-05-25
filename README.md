# Chat Application (TCP Sockets)

A simple terminal chat app built with **TCP sockets** in C. Good for demos on how client–server networking works.

## Features

- **Multiple clients** — server uses `select()` to handle many connections
- **Usernames** — each client picks a name when joining
- **Private messages** — `/pm user message`
- **Logging** — all joins, messages, and PMs go to `chat.log`
- **Colored output** — ANSI colors in the terminal

## Project layout

```
chat-app/
├── server.c    # accepts clients, routes messages
├── client.c    # connect, send, receive in a thread
├── common.h    # shared settings, colors, logging
├── Makefile
└── README.md
```

## Build

```bash
cd chat-app
make
```

## Run (two or more terminals)

**Terminal 1 — server:**

```bash
./server
# optional: ./server 5555
```

**Terminal 2 — client:**

```bash
./client
# optional: ./client 127.0.0.1 5555
```

**Terminal 3 — another client:**

```bash
./client
```

## Client commands

| Input | What it does |
|-------|----------------|
| `hello everyone` | broadcast to all users |
| `/pm alice secret` | private message to `alice` |
| `/list` | show online usernames |
| `/quit` | disconnect |

## How messages travel

```
Client A  --TCP-->  Server  --TCP-->  Client B
              \___________/
                 broadcast
```

Wire format (one line per message):

```
JOIN|bob
SAY|hello
PM|alice|meet me at 5
LIST|
QUIT|
```

Server replies with tags like `BROADCAST|`, `PRIVATE|`, `SYSTEM|`, `USERLIST|`, `ERROR|`.

## Logs

`chat.log` is created in the folder where you **start the server**. Example:

```
[2026-05-24 14:30:01] SERVER started
[2026-05-24 14:30:10] JOIN alice
[2026-05-24 14:30:15] BROADCAST bob: hi all
[2026-05-24 14:30:20] PM alice -> bob
```

## Presentation demo (3 min)

1. Start `./server` — point out port and log file.
2. Open two `./client` windows with different names.
3. Send a public message — both clients see it; server console shows `[broadcast]`.
4. Send `/pm` — only the target gets it; check `chat.log`.
5. `/list` and `/quit` — show user list and clean disconnect.

## Requirements

- Linux (or WSL)
- GCC

## See also

`DOCUMENTATION.md` — architecture, socket concepts, and viva Q&A.
