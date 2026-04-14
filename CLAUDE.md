# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and test

```
sh ./dev/run.sh
```

This sets up deps via `ix`, builds with `make -j`, and runs `./tst/test --top=20`.

To build only: `make -j`. To run tests only: `./tst/test`.

Source/test discovery is automatic via `wildcard` — adding a new `.cpp` under `std/*/` is enough, no Makefile edits needed. Files ending in `_ut.cpp` are compiled as unit tests; the rest go into `std/libstd.a`.

## Coding conventions

- Git author: `claude <claude@users.noreply.github.com>`. Commit messages in English.
- Types: `CamelCase`. Variables/functions: `camelCase`. Private members: trailing `_` (e.g. `len_`, `ptr_`). Macros: `STD_*` prefix.
- CamelCase for test names in `STD_TEST`.
- Namespace `stl` for public API. Free classes and functions in `.cpp` go into anonymous namespace.
- Methods longer than 1 line must be defined out of line and outside any namespace.
- Each `xxx.h` must have a corresponding `xxx.cpp`.
- No single-line braced blocks: `{ ... }` must span multiple lines.
- Avoid includes in headers; prefer forward declarations. Only include in `.cpp` files.
- Type aliases: `i8`/`u8`/`i16`/`u16`/`i32`/`u32`/`i64`/`u64` from `std/sys/types.h`.

## Architecture

C++ standard library replacement built around ObjPool — a bump-allocating arena that owns all objects. See README.md for design rationale.

Key modules under `std/`:

- **mem** — `ObjPool`, `MemoryPool`, `Disposer` (arena allocation + LIFO destruction)
- **ptr** — `ARC`, `IntrusivePtr`, reference counting
- **lib** — `Vector`, `Buffer`, `Deque`, `List` (containers; `Vector<T>` requires trivially destructible `T`)
- **str** — `StringView` (non-owning), `StringBuilder`, `fmt`
- **sym** — hash maps: `HashMap`, `IntMap`, `StringMap`
- **map** — ordered maps via treap
- **thr** — coroutines (`CoroExecutor`), thread pool, `Mutex`, `CondVar`, `Semaphore`, `Channel`, poller (epoll/kqueue/poll)
- **ios** — I/O: `ZeroCopyInput`/`ZeroCopyOutput`, buffered/fd/memory streams
- **net** — HTTP client/server, TCP sockets, TLS (multi-backend)
- **dns** — async DNS (c-ares) with thread-pool fallback
- **sys** — atomics, types, event_fd, platform primitives
- **dbg** — `STD_INSIST`, `STD_ASSERT`, panic
- **tst** — test framework: `STD_TEST_SUITE`, `STD_TEST`

Core pattern: headers expose pure virtual interfaces (`*Iface`); `.cpp` files contain concrete implementations in anonymous namespaces. Objects are created via `pool->make<T>(...)` or `T::create(&pool)`. This gives interface decoupling with arena-local data locality.

## Testing

```cpp
STD_TEST_SUITE(MySuite) {
    STD_TEST(TestName) {
        // _ctx available (ExecContext)
        STD_INSIST(condition);
    }
}
```

Test files: `std/*/*_ut.cpp`, auto-discovered by the build.
