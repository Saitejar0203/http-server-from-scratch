[![progress-banner](https://backend.codecrafters.io/progress/http-server/90e18e80-b78f-40de-873e-cc6f7ba667f6)](https://app.codecrafters.io/users/Saitejar0203?r=2qF)

# HTTP Server from Scratch

I’m building an HTTP/1.1 server in Python from scratch to deepen my understanding
of networking and low-level CS fundamentals and become better at building systems.
I’m following the [CodeCrafters challenge](https://app.codecrafters.io/courses/http-server/overview)
to explore sockets, TCP byte streams, file I/O, threads, compression, and connection lifetimes.

The server supports echo and user-agent responses, file downloads and uploads,
gzip compression, and concurrent persistent connections with explicit closure.

## Local setup

Requires Python 3.14 or later and `uv`.

```sh
uv sync --locked
./your_program.sh
```

The launcher starts the server on `localhost:4221`. To serve files, pass
`--directory /path/to/files`.

## How we work

Discuss the mechanism, review a design, implement the requested exercise, test,
commit, submit with `codecrafters submit`, and push to GitHub. Each completed
exercise gets its own commit. See [AGENTS.md](AGENTS.md) for the learning workflow.

The `origin` remote connects to CodeCrafters; `github` publishes this project.

## Original challenge instructions

This is a starting point for Python solutions to the
["Build Your Own HTTP server" Challenge](https://app.codecrafters.io/courses/http-server/overview).

[HTTP](https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol) is the
protocol that powers the web. In this challenge, you'll build a HTTP/1.1 server
that is capable of serving multiple clients.

Along the way you'll learn about TCP servers,
[HTTP request syntax](https://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html),
and more.

**Note**: If you're viewing this repo on GitHub, head over to
[codecrafters.io](https://codecrafters.io) to try the challenge.

# Passing the first stage

The entry point for your HTTP server implementation is in `app/main.py`. Study
and uncomment the relevant code, and then run the command below to execute the
tests on our servers:

```sh
codecrafters submit
```

Time to move on to the next stage!

# Stage 2 & beyond

Note: This section is for stages 2 and beyond.

1. Ensure you have `uv` installed locally
1. Run `./your_program.sh` to run your program, which is implemented in
   `app/main.py`.
1. Run `codecrafters submit` to submit your solution to CodeCrafters. Test
   output will be streamed to your terminal.
