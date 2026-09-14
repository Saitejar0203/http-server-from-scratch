# Learning-first CodeCrafters workflow

Build an HTTP server in Python to understand sockets, TCP byte streams, HTTP
messages, concurrency, compression, and connection lifetimes.

## Working together

1. Explain the current stage and trace the client, server, and relevant state.
2. Review Teja's pseudocode or plain-English design and identify missing cases.
3. Implement only when Teja asks to implement or fix that stage. An explanation
   request does not authorize code changes or moving on to another exercise.
4. Make the smallest clear change, preserve learner comments and unrelated work,
   and run proportionate local checks. Refactor when it improves understanding.
5. Explain the consequential choices and trace a representative request.

Passing tests demonstrates implementation behavior, not mastery. Use plain
language and introduce networking concepts as they become useful.

## Standing commit, submission, and publishing workflow

Teja requested the same process as the Shell project on 2026-09-14, including a
public GitHub repository. After completing and locally verifying each requested
exercise:

1. Commit the exercise with a descriptive message.
2. Run `codecrafters submit` and inspect its result; resolve failures within the
   requested scope before claiming completion.
3. Push the resulting commits to `github` on the default branch, including any
   commit created by the CodeCrafters CLI. Verify the remote head.
4. Report local validation, submission outcome, and GitHub publication separately.

`origin` is the CodeCrafters remote; `github` is the public GitHub remote. Keep
both. Use the configured GitHub-linked author identity; do not rewrite existing
history without an explicit request. Never publish credentials or local auth.
Do not implement further stages without authorization.

## Learning notes

Capture the useful mechanisms, questions, and corrected mental models in the
existing learning repository at `/Users/teja/learning/learning-system`, following
its own instructions and validation. Distinguish material explained from
understanding demonstrated. Preserve unrelated changes there.

## Entry points

- `app/main.py`: server entry point.
- `./your_program.sh`: local launcher, using `uv`.
- `.codecrafters/run.sh`: remote test launcher.
- `uv sync --locked`: install the pinned environment.
- `codecrafters ping`: activate/check the course connection during setup.

The initial setup intentionally leaves the first exercise unimplemented.
