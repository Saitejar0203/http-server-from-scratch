# HTTP Server in C

I’m building this HTTP server from scratch to understand TCP sockets, HTTP framing, concurrency, and memory ownership, and become better at building systems.

All 14 CodeCrafters HTTP server stages passed: basic responses and routing, echo and user-agent endpoints, concurrent clients, file downloads and uploads, gzip negotiation/compression, persistent connections, and explicit connection closure. The public repository keeps this C implementation alongside my Python implementation.

## Build and run

Requires a C compiler with C23 support, CMake 3.13+, POSIX threads, and zlib development headers/library.

```sh
./your_program.sh --directory /tmp
```

The directory flag is optional; file endpoints require it. The server listens on TCP port 4221. The launcher configures and builds `build/http-server` before running it. On macOS, if Xcode is unavailable but Command Line Tools are installed, prefix the command with `DEVELOPER_DIR=/Library/Developer/CommandLineTools`.

## Verification

With the server running, run the concurrent persistent-connection regression:

```sh
python3 tests/test_concurrency.py
```

It checks 48 clients, 384 responses, pipelined requests, gzip round trips, and an idle peer. Additional local checks covered fragmented binary uploads/downloads, path and symlink rejection, incomplete uploads, and explicit connection closure. AddressSanitizer and UndefinedBehaviorSanitizer checks passed for concurrent fragmented and pipelined traffic.

For a local sanitizer build:

```sh
cmake -S . -B build-sanitize -DCMAKE_C_FLAGS="-fsanitize=address,undefined -g"
cmake --build build-sanitize
./build-sanitize/http-server --directory /tmp
```

## Scope

This is a learning implementation, not a full HTTP server. Headers are limited to 64 KiB and request bodies/files to 64 MiB. File routes accept a single filename under the configured directory and refuse symlinks. One detached thread owns each connection. Chunked request bodies, TLS, timeouts, and full HTTP validation are outside the implemented stages.

Built for the [CodeCrafters HTTP server challenge](https://app.codecrafters.io/courses/http-server/overview).
