# Chat App — Documentation for Presentation and Viva

## 1. Overview

This program shows **TCP socket programming** in C: one **server** waits for connections, many **clients** connect and exchange text messages through the server.

---

## 2. Architecture

```
+----------+       +----------+       +----------+
| Client 1 |       |  Server  |       | Client 2 |
| (thread) |<----->| select() |<----->| (thread) |
+----------+       +----------+       +----------+
                        |
                   chat.log
```

- **Server** never chats on behalf of users; it only **relays** and **logs**.
- **Client** uses a **thread** to read from the socket while the main thread reads your keyboard.

---

## 3. Socket flow (step by step)

### Server

1. `socket()` — create TCP socket  
2. `bind()` — attach to port (default 5555)  
3. `listen()` — wait for connections  
4. `accept()` — new client arrives  
5. `select()` — watch server socket + all client sockets  
6. `recv()` — read a line from a client  
7. `send()` — push replies to the right client(s)

### Client

1. `socket()`  
2. `connect()` to server IP/port  
3. `send()` — `JOIN|username`  
4. `pthread` + `recv()` — print incoming messages  
5. `send()` — user types `SAY|...` or `PM|...`

---

## 4. Protocol (simple text lines)

| Direction | Example | Meaning |
|-----------|---------|---------|
| Client → Server | `JOIN\|ali` | Register name |
| Client → Server | `SAY\|hi` | Broadcast |
| Client → Server | `PM\|bob\|secret` | Private to bob |
| Client → Server | `LIST\|` | Request user list |
| Client → Server | `QUIT\|` | Leave |
| Server → Client | `BROADCAST\|ali\|hi` | Public chat |
| Server → Client | `PRIVATE\|bob\|secret` | Private chat |
| Server → Client | `SYSTEM\|...` | Info text |
| Server → Client | `USERLIST\|ali,bob` | Online users |
| Server → Client | `ERROR\|...` | Problem |

Each message ends with `\n`.

---

## 5. Key features explained

### Multiple clients

The server keeps an array of up to `MAX_CLIENTS` (20). `select()` tells which socket has data ready, so one process handles everyone.

### Usernames

Stored per client after `JOIN`. Duplicate names are rejected.

### Private messaging

`PM|target|text` is parsed on the server and sent only to the sender and target socket.

### Logging

`log_event()` in `common.h` appends timestamped lines to `chat.log`.

### Colored terminal

ANSI escape codes (e.g. `\033[32m` for green). Works in most Linux terminals.

---

## 6. Files

| File | Role |
|------|------|
| `common.h` | Port, limits, colors, `log_event()`, `send_line()` |
| `server.c` | Main loop, routing, broadcast, PM |
| `client.c` | UI, receive thread, command parsing |
| `Makefile` | Builds `server` and `client` |

---

## 7. Demo script (~5 minutes)

1. Explain **TCP** = reliable, connected stream.  
2. Run server, show green “running” line.  
3. Connect two clients; show join messages and `chat.log`.  
4. Public message on both clients + server `[broadcast]`.  
5. `/pm` — only one client sees it; show log line `PM ...`.  
6. `/list` — names on screen.  
7. `/quit` — disconnect; optional: show `select()` handling drop.

---

## 8. Viva questions and answers

**What is a socket?**  
An endpoint for network communication — like a phone line between programs.

**TCP vs UDP?**  
TCP is connection-based and reliable (order preserved). UDP is faster but no guarantee. Chat uses **TCP**.

**What does `bind()` do?**  
Links the server socket to an IP/port so clients know where to connect.

**What does `listen()` do?**  
Puts the socket in passive mode — ready to accept clients.

**What does `accept()` do?**  
Creates a new socket for one connected client.

**Why `select()`?**  
One thread can wait on many sockets; when any has data, the server reads it.

**Why a thread on the client?**  
`recv()` blocks. Without a thread, you could not type and read messages at the same time.

**What is broadcast?**  
Server sends the same message to every connected client except optional skip.

**How is private chat different?**  
Server sends only to sender and target file descriptors.

**What is `chat.log` for?**  
Audit trail: joins, public messages, PMs, server events.

**Port number?**  
Default `5555` in `common.h`. Can pass another port: `./server 6000`.

**Security limits?**  
Demo only — no encryption, no password. Real apps use TLS and authentication.

**How to extend?**  
Rooms/channels, file transfer, GUI, database for history.

---

## 9. Compile commands (without Make)

```bash
gcc -Wall -Wextra -std=c99 -pthread -o server server.c
gcc -Wall -Wextra -std=c99 -pthread -o client client.c
```

---

## 10. Troubleshooting

| Problem | Fix |
|---------|-----|
| `bind: Address in use` | Kill old server or use another port |
| `Connection refused` | Start server first |
| No colors | Use a modern terminal (most Linux consoles work) |
| Empty `chat.log` | Log is written next to where you run `./server` |

---

## 11. Summary

You built a **multi-client TCP chat** with usernames, public and private messages, file logging, and colored terminal UI — all core ideas for explaining **socket programming** in a presentation or viva.
