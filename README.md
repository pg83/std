# std

A C++ standard library replacement built around a single idea: **ObjPool owns everything, and everything lives close together.**

## The Problem

A typical C++ class accumulates heap-allocated members — `std::vector`, `std::map`, `std::deque`, `std::string` — each managing its own little island of memory. When an object holds five containers, its data is scattered across six or more disjoint heap regions. The CPU prefetcher cannot help you. Cache lines are wasted on allocator metadata. And every header that declares those containers drags in thousands of lines of template machinery, punishing build times across the entire project.

Interfaces (pure virtual classes) solve the header problem — a forward-declared pointer compiles instantly — but they make the data locality problem worse. Now you have indirection to a heap-allocated implementation *and* each implementation still scatters its own members across the heap.

## The Idea

What if a single bump allocator owned every object in a logical unit of work, and every sub-object lived in the same contiguous arena?

```
ObjPool pool = ObjPool::fromMemory();

auto* reactor = ReactorIface::create(exec, threadPool, &pool);
auto* client  = HttpClient::create(&pool, input);
auto* poller  = PollerIface::create(&pool);
```

Each `create` call placement-news the concrete implementation into the pool's arena. The reactor, the poller it uses, the HTTP client, the chunked-transfer decoder inside that client — they all live in the same growing region of memory, allocated in the order they were constructed.

## How ObjPool Works

**Allocation** is a pointer bump — O(1), a few instructions:

```cpp
void* MemoryPool::allocate(size_t len) {
    const size_t aligned = (len + alignment - 1) & ~(alignment - 1);

    if (current + aligned > end) {
        allocateNewChunk(aligned + sizeof(Chunk));
    }

    return exchange(current, current + aligned);
}
```

Chunks grow exponentially, so even large object graphs rarely trigger more than a handful of `malloc` calls.

**Destruction** is automatic. When an object has a non-trivial destructor, `ObjPool::make` wraps it in a `Disposable` node and adds it to a LIFO chain. When the pool dies, every tracked object is destroyed in reverse construction order — no manual cleanup, no `shared_ptr` overhead, no ref-count traffic:

```cpp
template <typename T, typename... A>
T* make(A&&... a) {
    if constexpr (trivially_destructible<T>) {
        // zero overhead — just bump the pointer
        return &makeImpl<Wrapper1>(args...)->t;
    } else {
        // one extra pointer for the destructor chain
        auto res = makeImpl<Wrapper2>(args...);
        submit(res);
        return &res->t;
    }
}
```

Trivially-destructible objects (integers, POD structs, pointers) cost exactly zero bookkeeping.

## Why This Beats the Conventional Layout

Consider a reactor that manages file descriptors, timers, a wake-up event, and a poller:

**Conventional approach** — members are `std::vector`, `std::unordered_map`, etc.:

```
ReactorState object     [heap block A]
  → vector<Fd> data     [heap block B]
  → unordered_map<...>  [heap block C, D, E...]
  → Poller impl         [heap block F]
    → epoll internals   [heap block G]
```

Seven allocations, seven cache misses on a cold access, seven sources of fragmentation. Every header includes `<vector>`, `<unordered_map>`, `<memory>`. Build times suffer.

**ObjPool approach:**

```
Pool arena chunk
  [ ReactorState | PollerIface(Epoll) | IntMap | DeadlineTreap | ... ]
```

One arena. Objects are packed in construction order. A linear scan through the reactor's state is a linear scan through memory. The poller — even though it is accessed through a virtual interface — sits bytes away from the reactor that uses it.

## Interfaces Without the Penalty

The standard advice is: *interfaces cost you data locality.* You pay for a vtable pointer, and the concrete object could be anywhere on the heap.

With ObjPool, the second cost disappears. The concrete object is allocated from the same arena as its owner. The vtable pointer is still there (one pointer per object, unavoidable with virtual dispatch), but the data behind it is *closer* than it would be if you had inlined a `std::vector` — because `std::vector`'s heap buffer is always elsewhere, while the pool-allocated implementation is right next door.

This means you can freely use interfaces to:

- **Decouple headers.** A header declares `PollerIface*` — one line, no platform includes. The `.cpp` picks `EpollPoller`, `KqueuePoller`, or `PollPoller` at construction time.
- **Swap implementations.** Testing with a mock poller? Same interface, same pool.
- **Compose decorators.** `ZeroCopyInput` wraps into `ChunkedInput` wraps into `LimitedInput` — each allocated from the same pool, each sitting next to the other in memory.

All without sacrificing the data locality you would get from a monolithic class with everything inlined.

## Build Time

Because public APIs traffic in interface pointers, headers are minimal. A file that uses `HttpClient*` does not include the HTTP parser, the chunked transfer decoder, the socket layer, or the TLS implementation. It includes a struct with four virtual methods.

The concrete implementations — often hundreds of lines with platform-specific includes — live in `.cpp` files. Changes to an implementation do not trigger recompilation of its users.

## Lifetime

ObjPool uses atomic reference counting (`ARC`) for its own lifetime. You hold an `IntrusivePtr<ObjPool>`, and when the last reference drops, the pool destroys every tracked object and frees every chunk.

Within the pool, there is no per-object lifetime management. Objects live exactly as long as the pool does. This is not a limitation — it is the model. A request handler creates a pool, builds everything it needs, processes the request, and drops the pool. One deallocation tears down the entire object graph.

For the cases where individual object recycling matters (connection pools, free lists), `ObjList<T>` provides typed allocation with O(1) reuse from a free list, backed by the same arena mechanics.

## Summary

| Concern | Conventional | ObjPool |
|---|---|---|
| Data locality | Scattered across heap | Packed in arena order |
| Allocation cost | `malloc` per object | Pointer bump |
| Deallocation | Per-object `free` / ref-count | Drop the pool |
| Header weight | Heavy (containers, templates) | Light (interface pointers) |
| Build time | Slow (transitive includes) | Fast (decoupled headers) |
| Interface cost | Heap-scattered impl | Arena-adjacent impl |
