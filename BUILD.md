# Build system

## Makefile

Source and test discovery is fully automatic via `wildcard`:

- `std/*/*.cpp` — all sources. Files ending in `_ut.cpp` are separated out as tests, the rest go into `std/libstd.a`.
- `tst/*.cpp` — test runner and additional test code.

Adding a new `.cpp` file to any `std/*/` directory is enough — no need to edit the Makefile.

Build: `make -j`. Run: `./tst/test`. Install: `make install DESTDIR=/prefix`.

The `install` target copies all `.h` files from `std/` into `$(DESTDIR)/include/std/...` preserving directory structure, and `libstd.a` into `$(DESTDIR)/lib/`.

The helper script `run.sh` sets up the build environment via `ix` (package manager), invokes `build.sh`, and runs the tests.

## Optional dependencies

The project uses `__has_include` to detect available libraries at compile time. No library is required — the code always compiles with built-in fallbacks.

### Hash (`std/str/hash.cpp`)

Backend for `shash32`/`shash64`, selected by priority:

| Priority | Header | Library | Notes |
|----------|--------|---------|-------|
| 1 | `<rapidhash.h>` | rapidhash | Header-only, fastest |
| 2 | `<xxhash.h>` | xxhash | `XXH3_64bits` |
| 3 | *(none)* | *(built-in)* | FNV-1a + SplitMix64 |

### TLS (`std/net/ssl_socket.cpp`)

Up to three TLS backends can be available simultaneously. Selection at runtime via `USE_SSL_ENGINE` env var (`openssl`, `s2n`, `mbedtls`), otherwise the first available is used in priority order:

| Priority | Header | Library |
|----------|--------|---------|
| 1 | `<openssl/ssl.h>` | OpenSSL / LibreSSL / AWS-LC |
| 2 | `<s2n.h>` | s2n-tls |
| 3 | `<mbedtls/ssl.h>` | Mbed TLS |

### DNS (`std/dns/ares.cpp`)

| Header | Library | Notes |
|--------|---------|-------|
| `<ares.h>` | c-ares | Async DNS via event loop; without it falls back to `getaddrinfo` offloaded to a thread pool |

### Stack traces

| Header | Library | Used in |
|--------|---------|---------|
| `<execinfo.h>` | libc (glibc/macOS) | `std/tst/ctx.cpp` — `backtrace()` fallback |
| `<cpptrace/cpptrace.hpp>` | cpptrace | `tst/test.cpp` — pretty stack traces in test runner |
