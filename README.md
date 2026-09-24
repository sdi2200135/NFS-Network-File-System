# System Programming-HW2
# NFS (Network File System)

> This document describes **exactly what the submitted code does**, including
> the parts of the assignment specification that are **not** implemented and
> suggestions for completing them.

---

## 1. Overview

This project is the second assignment of the Operating Systems course and is
a network-oriented extension of the first assignment (FSS). The goal is to
build a **Network File System (NFS)** that synchronizes files between remote
source and target directories over TCP sockets.

The system is composed of three executables:

| Component | Purpose |
|-----------|---------|
| `nfs_manager` | Central daemon: accepts commands from the console, coordinates clients |
| `nfs_console` | User interface: sends commands to the manager over a socket |
| `nfs_client`  | File server: listens on a port and serves file requests from the manager |

The implementation uses:
- **TCP sockets** (`socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`)
- **Timestamps** in `%Y-%m-%d %H:%M:%S` format
- **Separate manager and console log files**

---

## 2. High-Level Architecture (as implemented)

```
        +-------------+      TCP socket       +---------------+
        | nfs_console | --------------------> |  nfs_manager  |
        |             | <-------------------- |               |
        +-------------+                       +-------+-------+
                                                      |
                                         TCP socket   |  (one-shot connections)
                                                      v
                                             +-----------------+
                                             |   nfs_client    |
                                             +-----------------+
```

- The **console** connects to the manager's socket, sends user commands, and
  logs both commands and responses.
- The **manager** accepts one console connection at a time, parses the
  command, and replies over the same socket.
- For the `add` and `shutdown` commands, the manager opens an **outgoing**
  TCP connection to the corresponding `nfs_client` and sends it a message.
- The **client** listens on its port, accepts a single incoming connection,
  reads one message, prints it, and closes the connection (or exits on
  `shutdown`).

---

## 3. Project Structure

```
.
├── Makefile
├── README.md
├── nfs_manager.c
├── nfs_console.c
├── nfs_client.c
├── commands.c        / commands.h
├── log_funcs.c       / log_funcs.h
├── config_file.txt   (example)
├── manager_logfile   (generated)
└── console_logfile   (generated)
```

---

## 4. Component Details

### 4.1 `nfs_manager`

**Command line:**
```
./nfs_manager -l <manager_logfile> -c <config_file> -n <worker_limit> -p <port_number> -b <bufferSize>
```

All five options are parsed and validated for presence. However:

- `-n` (worker_limit) is read and then **always overwritten** by the constant
  `WORKER_LIMIT = 5`.
- `-b` (bufferSize) is read but **not used** anywhere.
- `-c` (config_file) is **not read at startup** — it is only read later,
  inside the `shutdown` handler.

**Startup:**
1. Parses and validates the CLI arguments.
2. Creates a listening TCP socket on `port_number` (Q = 5).

**Main loop (single console connection at a time):**
1. `accept()` a connection from the console (`s3`).
2. Loop over `recv()` to read commands until the console closes.
3. Dispatch each command:

| Command | What the manager does |
|---------|-----------------------|
| `shutdown` | Reads `config_file` line by line, parses each `source@host:port`, opens a TCP connection to each source host/port, sends `shutdown` to it, then calls `shutdown_mode(s3)` (prints 4 shutdown lines) and exits the outer loop. |
| `add <source> <target>` | Stores `source` and `target` into parallel arrays (`source_array`, `target_array`). Parses `source` as `path@host:port` and calls `connect_to_port()` to send a message to that host/port. Calls `add_mode(s3, source, target)`, prints "Added file ...", logs it, and sends it back to the console. |
| anything else | Ignored (no error message is returned to the console). |

4. After the `shutdown` command the outer loop breaks and the listening
   socket is closed.

**Helper:** `connect_to_port(host, port, buff)` — creates a socket, connects,
sends `buff`, and closes. It is **one-shot**: one connection per message, no
response is read.

**Logging:** `add_mode()` writes one line to `manager_logfile`:
```
[<timestamp>] Added file <source> -> <target>
```
No `[SOURCE] [TARGET] [THREAD_PID] [OP] [RESULT] [DETAILS]` format is used.

### 4.2 `nfs_console`

**Command line:**
```
./nfs_console -l <console-logfile> -h <host_IP> -p <host_port>
```

All arguments are parsed and validated.

**Behaviour:**
1. Creates a TCP socket and connects to `host_IP:host_port`.
2. Reads a line from `stdin`, strips the newline, and sends it to the
   manager.
3. Logs the command via `log_entry_file()` (prefixed with `Command ` if it is
   one of the known commands).
4. Waits for a response with `recv()`, prints it, and logs it.
5. Repeats until the user enters `shutdown`.

**Note:** The console uses a **single blocking `recv()`** per command. It does
not loop to drain multi-line responses (unlike the previous assignment's
console), so if the manager sends more than one message per command, only the
first is displayed.

### 4.3 `nfs_client`

**Command line:**
```
./nfs_client -p <port_number>
```

**Behaviour:**
1. Creates a TCP socket, binds to `INADDR_ANY:port_number`, and listens.
2. Loops forever:
   - `accept()` one connection.
   - `recv()` **one** message.
   - Prints `Received: <message>`.
   - If the message is exactly `shutdown`, breaks out of the loop and closes.
   - Otherwise closes the connection and waits for the next one.

**Notes:**
- The client does **not** implement `LIST`, `PULL`, or `PUSH` — it only
  prints whatever it receives.
- A connection is served with a single `recv`; the client does not read
  until the peer closes.
- The port is the **client's** listening port, not the manager's.

### 4.4 `commands.c` / `commands.h`

Contains two small helpers used by the manager:

- `shutdown_mode(int s3)` — builds a string with 4 shutdown lines
  (`Shutting down manager...`, `Waiting for all active workers to finish.`,
  `Processing remaining queued tasks.`, `Manager shutdown complete.`), prints
  it, and sends it to the console socket.
- `add_mode(int s3, char* source, char* target)` — builds
  `[<timestamp>] Added file <source> -> <target>`, prints it, appends it to
  `manager_logfile`, and sends it back to the console socket.

### 4.5 `log_funcs.c` / `log_funcs.h`

- `get_time()` — returns a static buffer with the current time in
  `%Y-%m-%d %H:%M:%S`.
- `error_func(msg)` — `perror` + `exit(EXIT_FAILURE)`.
- `log_entry(entry)` — prints `[<timestamp>] <entry>` to stdout.
- `log_entry_file(entry, logfile)` — appends to `logfile`. If the filename
  contains `console_logfile`, commands are prefixed with `Command `; manager
  log entries are written verbatim.

---

## 5. IPC Summary

| Channel | Type | Direction | Purpose |
|---------|------|-----------|---------|
| Console ↔ Manager | TCP socket (one per console) | both ways | User commands + responses |
| Manager → Client | TCP socket (one-shot) | manager → client | Send a message to a source client |

Both directions of the manager↔console channel share the same accepted
socket; there is no second port for replies.

---

## 6. Build & Run

### 6.1 Build

```bash
make all
```

Produces `nfs_manager`, `nfs_console`, and `nfs_client`.

> **Note on separate compilation:** although `MANAGER_OBJS` and
> `CONSOLE_OBJS` are defined, the Makefile compiles each target in a single
> `gcc` invocation (e.g. `gcc -o nfs_manager nfs_manager.c log_funcs.c commands.c`).
> This is **not** true object-by-object separate compilation, which the
> specification requires.

### 6.2 Run

Three terminals are needed.

**Terminal A — client:**
```bash
make run_client
# ./nfs_client -p 1234
```

**Terminal B — manager:**
```bash
make run_man
# ./nfs_manager -l manager_logfile -c config_file.txt -n 2 -p 4321 -b 1024
```

**Terminal C — console:**
```bash
make run_cons
# ./nfs_console -l console_logfile -h 127.0.0.1 -p 4321
```

### 6.3 Config file

Expected by the spec:

```
/source1@123.10.10.20:8000 /source2@100.200.10.10:8080
```

The current implementation only reads this file inside the `shutdown`
handler, and only to extract the source hosts/ports so it can notify clients.

### 6.4 Clean

```bash
make clean
```

Removes `.o` files, the three executables, and `console_logfile` /
`manager_logfile`.

---

## 7. Log Formats

### 7.1 `manager_logfile`

```
[<timestamp>] Added file <source> -> <target>
```

Only `add` writes to the manager log; there is no per-worker entry with
`[SOURCE] [TARGET] [THREAD_PID] [OP] [RESULT] [DETAILS]`.

### 7.2 `console_logfile`

```
[<timestamp>] Command <command>
<response lines>
```

All timestamps use `%Y-%m-%d %H:%M:%S`.

---

## 8. Design Choices

1. **TCP sockets for everything.** The console, the manager, and the clients
   all communicate over TCP; there are no FIFOs or shared memory.
2. **One console connection at a time.** The manager accepts a single
   console and serves it in a loop; a second console cannot connect until
   the first disconnects.
3. **One-shot outgoing connections.** `connect_to_port()` opens a socket,
   sends a single message, and closes it. This is enough for the
   `shutdown` notification but not for a request/response protocol.
4. **Arrays instead of a hash table.** Monitored pairs are kept in parallel
   arrays (`source_array`, `target_array`) rather than a structured
   `sync_info_mem_store`.
5. **Time via `strftime`.** All timestamps are produced by `get_time()`
   using `%Y-%m-%d %H:%M:%S`.
6. **No worker threads.** The current implementation is single-threaded on
   the manager side; there is no thread pool and no condition variable.

---

## 9. What Is Not Yet Implemented / Could Be Added

The following parts of the assignment specification are **not present in the
current implementation**. They are listed in the order of the spec so that
each one can be added incrementally.

### 9.1 Real file-list discovery (`LIST`)
The manager should, for every `(source, target)` pair in the config file,
connect to the source `nfs_client` and send `LIST <source_dir>`. The client
should reply with one filename per line terminated by a single `!`. The
manager should then enqueue one sync job per file with the source and target
information. **Currently the manager never sends `LIST` and never parses a
file list.**

### 9.2 Real file transfer (`PULL` / `PUSH`)
For each queued file the worker should:
1. Open a connection to the source client and send `PULL /source_dir/file`.
   The client should reply with `<filesize><space><data>` (or `-1 <error>`
   on failure).
2. Open a connection to the target client and send
   `PUSH /target_dir/file <chunk_size> <data>`, using `chunk_size = -1` to
   truncate the target and `chunk_size = 0` to signal EOF.

**Currently the client only prints received bytes; it does not serve or
store files.**

### 9.3 Worker threads and thread pool
The manager should create a pool of `worker_limit` threads at startup. Each
thread should loop taking jobs from a shared queue. **Currently the manager
is single-threaded and there are no threads at all.**

### 9.4 Bounded buffer + condition variables
The sync-job queue should be a bounded buffer of size `bufferSize`, guarded
by a mutex and two condition variables (`not_empty`, `not_full`). Workers
should block on `not_empty` when the queue is empty and the producer should
block on `not_full` when the queue is full. **Currently the manager ignores
`-b` and has no queue.**

### 9.5 `sync_info_mem_store`
A structured in-memory store (`source_host, source_port, source_dir,
target_host, target_port, target_dir, status, last_sync_time, active,
error_count`) should replace the parallel arrays. It should be searched on
every `add` and `cancel` and updated on every completed sync.

### 9.6 Duplicate detection for `add`
The spec requires that re-adding an existing `(source, target)` pair does
**not** trigger a new sync and that the manager prints
`Already in queue: <source>`. **Currently duplicates are silently accepted
and the message is not produced.**

### 9.7 `cancel <source dir>`
Cancels the remaining queued sync jobs of the given source directory.
**Currently the `cancel` command is not implemented at all** — the manager
ignores anything that is not `add` or `shutdown`.

### 9.8 Graceful `shutdown`
The spec requires that shutdown:
1. Stops accepting new commands.
2. Waits for active workers to finish.
3. Processes the remaining queued tasks.
4. Then exits.

**Currently `shutdown` prints the four lines, notifies the clients, and
exits immediately — active work (if there were any) would be lost.**

### 9.9 Proper client protocol on the client side
The `nfs_client` should:
- Serve `LIST`, `PULL`, and `PUSH` requests in a loop.
- Handle multiple connections (threads or `select`/`poll`).
- Open files with low-level syscalls (`open`, `read`, `write`, `close`) and
  handle errors with `strerror(errno)`.

**Currently the client only prints the first message it receives and never
implements the three required commands.**

### 9.10 Full log format
Every completed sync should produce a line:
```
[TIMESTAMP] [SOURCE_DIR] [TARGET_DIR] [THREAD_PID] [OPERATION] [RESULT] [DETAILS]
```
with `OPERATION ∈ {PULL, PUSH}` and `RESULT ∈ {SUCCESS, ERROR}`.
**Currently only an abbreviated `Added file ...` line is written.**

### 9.11 Reading `config_file` at startup
The config file should be parsed **at startup** so that the initial full
sync runs when the manager starts. **Currently it is only read inside the
`shutdown` handler.**

### 9.12 Proper separate compilation
The Makefile should compile each `.c` into its own `.o` and then link the
objects, which is what the specification requires. **Currently a single
`gcc` call compiles all sources at once.**

### 9.13 Console draining of multi-line responses
The console should keep reading from the socket until it has received the
complete response for the current command (e.g. by looping `recv` until the
peer signals completion, or using a small protocol). **Currently only the
first `recv` is displayed.**

### 9.14 Client-to-manager reply channel
Once `LIST` / `PULL` / `PUSH` are implemented, the client must be able to
reply to the manager over the same TCP connection. **Currently
`connect_to_port()` never reads a reply.**

---

## 10. Quick Reference

```bash
# Build
make all

# Terminal A – client
make run_client      # ./nfs_client -p 1234

# Terminal B – manager
make run_man         # ./nfs_manager -l manager_logfile -c config_file.txt -n 2 -p 4321 -b 1024

# Terminal C – console
make run_cons        # ./nfs_console -l console_logfile -h 127.0.0.1 -p 4321

# Clean
make clean
```
