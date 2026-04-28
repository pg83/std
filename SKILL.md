---
name: std-design
description: Manual trigger only — invoke when the user explicitly asks (e.g. `/std-design`, "design this in std style", "apply the std philosophy"). Encodes the arena-first design philosophy from README.md: a single ObjPool owns every object in a logical unit of work, headers traffic only in interface pointers, implementations live in anonymous namespaces inside `.cpp`, and lifetime is bulk — not per-object. Use this when shaping a new subsystem, choosing data structures, deciding header vs. implementation split, or reviewing existing code for fit with the philosophy. Do NOT auto-invoke from file content alone.
---

# std-design — the arena-first philosophy

One sentence: **a single ObjPool owns every object in a logical unit of work, and every sub-object lives in the same contiguous arena.**

That sentence is the whole library. Everything below is a consequence of it.

## The five consequences

1. **Allocation is a pointer bump.** `pool->make<T>(args...)` placement-news `T` into the arena. O(1), a few instructions. No `malloc` per object.
2. **Deallocation is dropping the pool.** Non-trivial destructors are chained LIFO and fired in reverse construction order when the pool dies. Trivially-destructible objects cost zero bookkeeping.
3. **Headers are interface pointers.** Public APIs traffic in `T*` where `T` is a pure-virtual interface forward-declared in the header. The concrete implementation lives in an anonymous namespace in the `.cpp`, allocated through a `static T* create(ObjPool* pool, ...)` factory.
4. **Data locality survives indirection.** A virtual call still costs a vtable load, but the concrete object sits bytes away from the interface that owns it — closer than an inlined `std::vector` whose heap buffer is always elsewhere.
5. **No per-object lifetime.** Objects live exactly as long as the pool. A request handler creates a pool, builds everything it needs, processes the request, drops the pool. One deallocation tears down the entire object graph.

## How to design something new in this style

When the user asks you to design or extend a subsystem, walk through these decisions in order. Don't skip — the answers compound.

### 1. What is the unit of work?

Every pool answers one question: "what dies together?" A request handler. A connection. A test. A long-running daemon. Whatever it is, that is the lifetime boundary, and the pool is created and dropped at that boundary.

If you cannot name the unit, you do not know where the pool goes yet. Ask the user.

### 2. What's an interface, what's a struct?

- **Interface** (`*Iface` or just a struct with virtual methods, in the header): for anything whose implementation might vary, anything platform-specific, anything heavy enough that its includes would punish build times. Header is forward declarations + pure virtual methods + a `create` factory. Caller never sees the concrete type.
- **Struct** (concrete in the header): for small value types — POD configs, intrusive nodes, plain data carriers. `StringView`, `PollFD`, `SpawnParams`. No virtual methods, no allocation, often trivially destructible.

When in doubt, prefer an interface. The build-time tax of a heavy include is real and lasting; the cost of one indirection through a vtable is one cache line that's already in L1.

### 3. Where does it allocate from?

Always a pool passed in. Never `new`, never `std::make_unique`, never a static singleton with global lifetime. The factory looks like:

```cpp
Foo* Foo::create(ObjPool* pool, /* deps */) {
    return pool->make<FooImpl>(/* deps */);
}
```

The caller decides the lifetime by deciding which pool to pass.

### 4. How do you store collections?

`Vector<T>` requires `T` to be trivially destructible. This is not a wart — it is the model.

- **Trivial T (pointers, ints, POD)**: use `Vector<T>` directly. Growth is `memcpy`. No move/copy traffic. No destructor bookkeeping. `Vector<T*>` is exactly `sizeof(void*)` per element.
- **Non-trivial T**: store `Vector<T*>`, allocate the `T`s from the pool. The objects live arena-contiguous (because `pool->make` bumps sequentially), the pointers live vector-contiguous, and growth never moves the objects — just the pointers. The pool destroys the `T`s automatically when it dies; the vector itself is trivial.
- **Keyed lookup**: `IntMap<T>`, `SymbolMap<T>`, `Map<K, V>`. All take a pool in the constructor and allocate nodes from it.
- **Reusable slots** (connection pools, free lists): `ObjList<T>` — typed allocation with O(1) reuse.

If you find yourself wanting `std::vector<MyClass>` with a non-trivial `MyClass`, stop. Use `Vector<MyClass*>` and pool-allocate.

### 5. How do you handle strings?

There is no owning string class. Strings have three phases:

- **Build**: `Buffer` for raw bytes, `StringBuilder` (which is also a `ZeroCopyOutput`) for streamed construction. Anything that writes to an `Output` can produce a string.
- **Intern**: `pool->intern(view)` copies the bytes into the arena and returns a `StringView`. The pool now owns them.
- **Use**: `StringView` everywhere — comparison, hashing, splitting, prefix/suffix, parsing. Non-owning `(ptr, len)`. No allocations on the hot path.

A field like `StringView name_;` is normal. A field like `std::string name_;` is wrong — it scatters.

### 6. How do errors propagate?

- **Programmer errors**: `STD_INSIST(cond)` (always-on) or `STD_ASSERT(cond)` (debug-only). Panic, no exception.
- **External / runtime errors**: `Errno(rc).raise(StringBuilder() << ...)` — throws a structured exception. Catch only at meaningful boundaries.
- **Expected failures** (parse, lookup miss): return value — `nullptr`, `bool`, `StringView*`, etc. Do not throw.

### 7. Composition by decoration

Decorators chain through interfaces and the same pool. A `ZeroCopyInput` becomes a `ChunkedInput` becomes a `LimitedInput` — each one is `pool->make<...>(inner, ...)`. Each new layer sits in the arena right next to the previous one. This is how HTTP transfer encoding, TLS, buffering all compose without any of them knowing about each other.

When you reach for a flag (`if (chunked) ...`) or a config bool inside a method, ask whether it should be a decorator instead.

### 8. Concurrency: coroutines, not callbacks

Long-running work runs as coroutines on a `CoroExecutor`. Sync primitives (`Mutex`, `CondVar`, `Semaphore`, `Channel`) come in two flavors via different `create` overloads — one OS-backed for plain threads, one coroutine-aware for use inside a `CoroExecutor`. Pick the one that matches the call site.

`async(exec, fn)` returns a `Future<T>`. Use it for offloading. Do not invent new threading primitives.

## Anti-patterns — recognize and refuse

- A class with `std::vector`, `std::string`, `std::unordered_map` members. → Move it into a pool and replace with `Vector<T*>`, `StringView`, `SymbolMap<T>`.
- `std::shared_ptr<T>` for shared ownership. → The pool is the owner. If you need rebinding (rare), `IntrusivePtr<T>` over `ARC`.
- A `static` global with non-trivial state. → Pass a pool. If it must be process-wide, use `ObjPool::fromMemory()` once at startup.
- A header that includes `<vector>`, `<unordered_map>`, `<memory>`. → Forward-declare an interface in the header, include in the `.cpp`.
- A method that allocates with `new` and returns ownership via `unique_ptr` / raw delete. → Pool factory.
- A "destructor that frees children". → The pool already does that. Drop it.
- A field of type `std::optional<HeavyThing>`. → Pointer that is nullable, allocated via `pool->make` when needed.
- Manual reference counting with `addRef`/`release`. → Pool. If the object truly outlives any single pool, `IntrusivePtr` + `ARC`, and even then only at the boundary.

## Sanity checks before declaring a design done

- Can you state, in one sentence, which pool owns this and at what point that pool dies?
- Does the public header for this subsystem compile in isolation with only forward declarations? (No `<vector>`, no platform headers, no template-heavy includes.)
- If you remove every `delete`, every `shared_ptr`, every explicit destructor call from the design, does it still work? It should.
- For each container, did you choose between `Vector<T*>`, `IntMap<T>`, `SymbolMap<T>`, `ObjList<T>`, `Map<K, V>` deliberately?
- For each string, is its phase clear — building, interned, or borrowed view?
- Could a future implementation be swapped at the `create` boundary without touching callers?

If any answer is "no" or "not sure", revisit the design before writing code.

## When this skill does NOT apply

- Pure algorithms with no state and no allocation (sort, hash, parse-into-out-param). Just write the function.
- Tiny utility headers (`min`, `max`, `exchange`, `defer`). The philosophy is for *systems*, not for one-line helpers.
- Code that lives outside the `std/` library and intentionally uses the standard library. Don't preach.
