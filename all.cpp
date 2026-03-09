./std/alg/shuffle.h
#pragma once

#include "xchg.h"

#include <std/sys/types.h>

namespace stl {
    template <typename RNG, typename I>
    void shuffle(RNG& r, I b, I e) noexcept {
        for (auto n = e - b; n > 1; --n) {
            xchg(b[n - 1], b[r.uniformUnbiased((u32)n)]);
        }
    }
}
./std/alg/advance.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    template <typename T>
    inline T* advancePtr(T* ptr, size_t len) noexcept {
        return (T*)(len + (const u8*)ptr);
    }
}
./std/alg/bits.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    inline u32 clp2(u32 v) noexcept {
        return v <= 1 ? 1u : 1u << (32 - __builtin_clz(v - 1));
    }

    inline u64 clp2(u64 v) noexcept {
        return v <= 1 ? 1ull : 1ull << (64 - __builtin_clzll(v - 1));
    }
}
./std/alg/destruct.h
#pragma once

namespace stl {
    template <typename T>
    auto destruct(T* t) {
        return (t->~T(), t);
    }
}
./std/alg/range.h
#pragma once

namespace stl {
    template <typename B, typename E>
    struct Range {
        const B b;
        const E e;

        auto begin() const noexcept {
            return b;
        }

        auto end() const noexcept {
            return e;
        }

        unsigned long length() const noexcept {
            return e - b;
        }
    };

    template <typename B, typename E>
    auto range(B b, E e) noexcept {
        return Range<B, E>{b, e};
    }

    template <typename C>
    auto range(const C& c) noexcept {
        return range(c.begin(), c.end());
    }

    template <typename C>
    auto mutRange(C& c) noexcept {
        return range(c.mutBegin(), c.mutEnd());
    }
}
./std/alg/reverse.h
#pragma once

#include "xchg.h"

namespace stl {
    template <typename B, typename E>
    void reverse(B b, E e) {
        for (; b < e; xchg(*b++, *--e)) {
        }
    }

    template <typename R>
    void reverse(R&& r) {
        reverse(r.begin(), r.end());
    }
}
./std/alg/minmax.h
#pragma once

namespace stl {
    template <typename T>
    T min(T l, T r) {
        return l < r ? l : r;
    }

    template <typename T>
    T max(T l, T r) {
        return l < r ? r : l;
    }
}
./std/alg/qsort.h
#pragma once

#include "xchg.h"

#include <std/rng/pcg.h>
#include <std/lib/vector.h>
#include <std/typ/support.h>

namespace stl::QSP {
    template <typename I, typename C>
    struct Context {
        struct WorkItem {
            I b;
            I e;
        };

        C& f;
        Vector<WorkItem> w;
        PCG32 r;

        Context(C& _f, void* seed) noexcept
            : f(_f)
            , r(seed)
        {
        }

        void insertionSort(I b, I e) {
            for (auto i = b + 1; i != e; ++i) {
                for (auto j = i; j != b && f(*j, *(j - 1)); --j) {
                    xchg(*j, *(j - 1));
                }
            }
        }

        auto chooseRandom(I b, I e) noexcept {
            return b + r.uniformBiased(e - b);
        }

        // called as partitionHoare(b, l) where l = e-1 (pivot is at *l).
        // p captures l; the second while pre-decrements e, so e never reaches p.
        // *p is never swapped — pivot stays in place throughout.
        auto partitionHoare(I b, I e) {
            for (auto p = e;; ++b) {
                while (b != e && f(*b, *p)) {
                    ++b;
                }

                while (b != e && f(*p, *--e)) {
                }

                if (b == e) {
                    return b;
                }

                xchg(*b, *e);
            }
        }

        // assume b < e
        bool alreadySorted(I b, I e) {
            for (++b; b != e; ++b) {
                if (f(*b, *(b - 1))) {
                    return false;
                }
            }

            return true;
        }

        void qSortStep(I b, I e) {
            if (e - b < 16) {
                return;
            }

            if (alreadySorted(b, e)) {
                return;
            }

            auto l = e - 1;

            // pivot to last
            xchg(*chooseRandom(b, e), *l);

            // place for pivot
            auto p = partitionHoare(b, l);

            // move pivot form last to proper place
            xchg(*p, *l);

            // recurse
            w.pushBack(WorkItem{p + 1, e});
            w.pushBack(WorkItem{b, p});
        }

        void qSortLoop() {
            while (!w.empty()) {
                auto item = w.popBack();

                qSortStep(item.b, item.e);
            }
        }

        void qSort(I b, I e) {
            if (b != e) {
                qSortStep(b, e);
                qSortLoop();
                insertionSort(b, e);
            }
        }
    };
}

namespace stl {
    template <typename I, typename C>
    void quickSort(I b, I e, C&& f) {
        if (b != e) {
            QSP::Context<I, C>(f, (void*)&*b).qSort(b, e);
        }
    }

    template <typename I>
    void quickSort(I b, I e) {
        return quickSort(b, e, [](const auto& x, const auto& y) {
            return x < y;
        });
    }

    template <typename R, typename C>
    void quickSort(R&& r, C&& f) {
        return quickSort(r.begin(), r.end(), forward<C>(f));
    }

    template <typename R>
    void quickSort(R&& r) {
        return quickSort(r.begin(), r.end());
    }
}
./std/alg/defer.h
#pragma once

#define STD_DEFER ::stl::ScopedGuard _ = [&] mutable -> void

namespace stl {
    template <typename F>
    struct ScopedGuard {
        ScopedGuard(F&& ff) noexcept
            : f(ff)
        {
        }

        ~ScopedGuard() {
            f();
        }

        F f;
    };
}
./std/alg/xchg.h
#pragma once

#include <std/typ/support.h>

namespace stl {
    template <typename T>
    void xchg(T& l, T& r) noexcept {
        if constexpr (requires { l.xchg(r); }) {
            l.xchg(r);
        } else if constexpr (requires { l.swap(r); }) {
            l.swap(r);
        } else if constexpr (requires { T(move(l)) = move(l); }) {
            T t = move(l);

            l = move(r);
            r = move(t);
        } else {
            T t = l;

            l = r;
            r = t;
        }
    }
}
./std/alg/exchange.h
#pragma once

namespace stl {
    template <typename T, typename N>
    T exchange(T& o, N n) {
        auto r = o;

        o = n;

        return r;
    }
}
./std/sym/s_map.h
#pragma once

#include "h_map.h"

#include <std/str/view.h>

namespace stl {
    struct SymbolHasher {
        static u64 hash(StringView k) noexcept {
            return k.hash64();
        }
    };

    template <typename T>
    using SymbolMap = HashMap<T, StringView, SymbolHasher>;
}
./std/sym/i_map.h
#pragma once

#include "h_map.h"

namespace stl {
    struct IntHasher {
        static u64 hash(u64 k) noexcept {
            return k;
        }
    };

    template <typename T>
    using IntMap = HashMap<T, u64, IntHasher>;
}
./std/sym/h_map.h
#pragma once

#include "h_table.h"

#include <std/mem/embed.h>
#include <std/typ/intrin.h>
#include <std/alg/destruct.h>
#include <std/mem/obj_list.h>

namespace stl {
    template <typename T, typename K, typename H>
    class HashMap {
        struct Node: public HashTable::Node, public Embed<T> {
            using Embed<T>::Embed;
        };

        ObjList<Node> ol;
        HashTable st;

        T* insertNode(Node* value) {
            if (auto prev = (Node*)st.insert(value); prev) {
                ol.release(prev);
            }

            return &value->t;
        }

    public:
        ~HashMap() {
            if constexpr (!stdHasTrivialDestructor(Node)) {
                st.visit([](auto node) {
                    destruct((Node*)node);
                });
            }
        }

        T* find(K key) const noexcept {
            if (auto n = (Node*)st.find(H::hash(key)); n) {
                return &n->t;
            }

            return nullptr;
        }

        template <typename... A>
        T* insert(K key, A&&... a) {
            auto value = ol.make(forward<A>(a)...);

            return (value->key = H::hash(key), insertNode(value));
        }

        template <typename... A>
        T* insertKeyed(A&&... a) {
            auto value = ol.make(forward<A>(a)...);

            return (value->key = value->t.key(), insertNode(value));
        }

        T& operator[](K key) {
            if (auto res = find(key); res) {
                return *res;
            }

            return *insert(key);
        }

        void erase(K key) noexcept {
            if (auto prev = (Node*)st.erase(H::hash(key)); prev) {
                ol.release(prev);
            }
        }

        template <typename F>
        void visit(F f) {
            st.visit([f](auto el) {
                f(((Node*)el)->t);
            });
        }
    };
}
./std/sym/h_table.h
#pragma once

#include <std/sys/types.h>
#include <std/lib/buffer.h>
#include <std/lib/visitor.h>

namespace stl {
    class HashTable {
    public:
        struct Node {
            u64 key;
            Node* next = nullptr;
        };

    private:
        Buffer buf;

        void rehash(size_t len);
        void addNoRehash(Node* node);
        void visitImpl(VisitorFace&& v);
        Node** bucketFor(u64 key) const noexcept;

    public:
        HashTable(size_t initial);

        HashTable()
            : HashTable(8)
        {
        }

        ~HashTable() noexcept;

        Node* find(u64 key) const noexcept;
        void xchg(HashTable& t) noexcept;
        size_t capacity() const noexcept;
        Node* erase(u64 key) noexcept;
        size_t size() const noexcept;
        Node* insert(Node* node);

        template <typename V>
        void visit(V v) {
            visitImpl(makeVisitor([v](void* ptr) {
                v((Node*)ptr);
            }));
        }
    };
}
./std/rng/split_mix_64.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    u64 splitMix64(u64 x) noexcept;
    u64 nextSplitMix64(u64* x) noexcept;
}
./std/rng/pcg.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class PCG32 {
        u64 state_;
        u64 seq_;

    public:
        PCG32(u64 seq) noexcept;
        PCG32(const void* seq) noexcept;
        PCG32(u64 state, u64 seq) noexcept;

        u32 nextU32() noexcept;

        u32 uniformBiased(u32 n) noexcept {
            // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
            return ((u64)nextU32() * (u64)n) >> 32;
        }

        u32 uniformUnbiased(u32 n) noexcept;
    };
};
./std/sys/fd.h
#pragma once

#include <std/sys/types.h>

struct iovec;

namespace stl {
    class FD {
        // does not own fd
        int fd;

    public:
        FD() noexcept
            : fd(-1)
        {
        }

        FD(int _fd) noexcept
            : fd(_fd)
        {
        }

        size_t read(void* data, size_t len);
        size_t writeV(iovec* parts, size_t count);
        size_t write(const void* data, size_t len);

        void close();
        void fsync();

        void xchg(FD& fd) noexcept;
    };

    struct ScopedFD: public FD {
        using FD::FD;

        ~ScopedFD() noexcept(false);
    };

    void createPipeFD(ScopedFD& in, ScopedFD& out);
}
./std/sys/crt.h
#pragma once

#include "types.h"

namespace stl {
    // allocator
    void* allocateMemory(size_t len);
    void freeMemory(void* ptr) noexcept;

    // string ops
    void memZero(void* from, void* to) noexcept;
    int memCmp(const void* l, const void* r, size_t len) noexcept;
    void* memCpy(void* to, const void* from, size_t len) noexcept;
    size_t strLen(const u8* s) noexcept;
}
./std/sys/fs.h
#pragma once

#include <std/str/view.h>
#include <std/lib/visitor.h>

namespace stl {
    struct TPathInfo {
        StringView item;
        bool isDir;
    };

    void listDirImpl(StringView path, VisitorFace&& vis);

    template <typename F>
    void listDir(StringView path, F f) {
        listDirImpl(path, makeVisitor([f](void* ptr) {
                        f(*(TPathInfo*)ptr);
                    }));
    }
}
./std/sys/types.h
#pragma once

#include <stddef.h>
#include <inttypes.h>

using i8 = int8_t;
// yep
using u8 = char8_t;

using i16 = int16_t;
using u16 = uint16_t;

using i32 = int32_t;
using u32 = uint32_t;

using i64 = int64_t;
using u64 = uint64_t;
./std/sys/atomic.h
#pragma once

#define stdAtomicAddAndFetch __atomic_add_fetch
#define stdAtomicSubAndFetch __atomic_sub_fetch
#define stdAtomicFetch __atomic_load_n
#define stdAtomicStore __atomic_store_n
#define stdAtomicCAS(ptr, expected, desired, success_order, fail_order) \
    __atomic_compare_exchange_n(ptr, expected, desired, false, success_order, fail_order)

namespace stl {
    namespace MemoryOrder {
        constexpr int Acquire = __ATOMIC_ACQUIRE;
        constexpr int Release = __ATOMIC_RELEASE;
        constexpr int Consume = __ATOMIC_CONSUME;
        constexpr int Relaxed = __ATOMIC_RELAXED;
    };
}
./std/sys/throw.h
#pragma once

namespace stl {
    class Buffer;
    class StringView;

    enum class ExceptionKind {
        Errno,
    };

    struct Exception {
        virtual ~Exception() noexcept;

        virtual ExceptionKind kind() const noexcept = 0;
        virtual StringView description() = 0;

        static StringView current();
    };

    struct Errno {
        int error;

        Errno() noexcept;

        [[noreturn]]
        void raise(Buffer&& text);
    };
}
./std/ios/in_buf.h
#pragma once

#include "in_zc.h"

#include <std/sys/types.h>
#include <std/lib/buffer.h>

namespace stl {
    class InBuf: public ZeroCopyInput {
        Input* in_;
        Buffer buf;
        size_t pos;

        InBuf() noexcept;

        size_t hintImpl() const noexcept override;
        size_t nextImpl(const void** chunk) override;
        void commitImpl(size_t len) noexcept override;

    public:
        ~InBuf() override;

        InBuf(Input& in) noexcept;
        InBuf(Input& in, size_t chunkSize) noexcept;

        InBuf(InBuf&& buf) noexcept
            : InBuf()
        {
            buf.xchg(*this);
        }

        InBuf(const InBuf&) = delete;

        void xchg(InBuf& buf) noexcept;

        Input& stream() noexcept {
            return *in_;
        }
    };
}
./std/ios/out_buf.h
#pragma once

#include "out_zc.h"

#include <std/sys/types.h>
#include <std/lib/buffer.h>

namespace stl {
    class OutBuf: public ZeroCopyOutput {
        Output* out_;
        Buffer buf_;
        size_t chunk;

        OutBuf() noexcept;

        size_t writeDirect(const void* ptr, size_t len);
        size_t writeMultipart(const void* ptr, size_t len);

        // state
        void flushImpl() override;
        void finishImpl() override;

        // classic
        size_t writeImpl(const void* ptr, size_t len) override;
        size_t hintImpl() const noexcept override;

        // zero-copy
        void* imbueImpl(size_t* len) override;
        void commitImpl(size_t len) override;

    public:
        ~OutBuf() override;

        OutBuf(Output& out) noexcept;
        OutBuf(Output& out, size_t chunkSize) noexcept;

        OutBuf(OutBuf&& buf) noexcept
            : OutBuf()
        {
            buf.xchg(*this);
        }

        OutBuf(const OutBuf&) = delete;

        void xchg(OutBuf& buf) noexcept;

        Output& stream() noexcept {
            return *out_;
        }
    };
}
./std/ios/sys.h
#pragma once

#include "manip.h"
#include "output.h"
#include "out_buf.h"

#define sysO ::stl::OutBuf(::stl::stdoutStream())
#define sysE ::stl::OutBuf(::stl::stderrStream())

namespace stl {
    Output& stdoutStream() noexcept;
    Output& stderrStream() noexcept;
}
./std/ios/in_zc.h
#pragma once

#include "input.h"

#include <std/sys/types.h>

namespace stl {
    class Buffer;

    class ZeroCopyInput: public Input {
        void sendTo(Output& out) override;
        size_t readImpl(void* data, size_t len) override;

        virtual size_t nextImpl(const void** chunk) = 0;
        virtual void commitImpl(size_t len) noexcept = 0;

    public:
        ~ZeroCopyInput() noexcept override;

        bool readLine(Buffer& buf);
        bool readTo(Buffer& buf, u8 delim);

        size_t next(const void** chunk) {
            return nextImpl(chunk);
        }

        void commit(size_t len) noexcept {
            commitImpl(len);
        }
    };
}
./std/ios/input.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class Output;
    class Buffer;
    class ZeroCopyInput;

    class Input {
        virtual size_t hintImpl() const noexcept;
        virtual size_t readImpl(void* data, size_t len) = 0;

    public:
        virtual ~Input() noexcept;

        size_t read(void* data, size_t len) {
            return readImpl(data, len);
        }

        size_t hint() const noexcept {
            return hintImpl();
        }

        size_t hint(size_t def) const noexcept {
            if (auto h = hint(); h) {
                return h;
            }

            return def;
        }

        void readAll(Buffer& res);

        virtual void sendTo(Output& out);
    };
}
./std/ios/in_mem.h
#pragma once

#include "in_zc.h"

#include <std/sys/types.h>

namespace stl {
    class MemoryInput: public ZeroCopyInput {
        const u8* b;
        const u8* e;

        void sendTo(Output& out) override;
        size_t nextImpl(const void** chunk) override;
        void commitImpl(size_t len) noexcept override;

    public:
        MemoryInput(const void* data, size_t len) noexcept
            : b((const u8*)data)
            , e(b + len)
        {
        }
    };
}
./std/ios/out_fd.h
#pragma once

#include "output.h"

namespace stl {
    class FD;

    class FDOutput: public Output {
        size_t writeImpl(const void* data, size_t len) override;
        size_t writeVImpl(iovec* parts, size_t count) override;

        void finishImpl() override;

    public:
        // does not own fd
        FD* fd;

        FDOutput(FD& _fd) noexcept
            : fd(&_fd)
        {
        }

        ~FDOutput() noexcept override;
    };

    class FDRegular: public FDOutput {
        void flushImpl() override;
        size_t hintImpl() const noexcept override;

    public:
        FDRegular(FD& fd) noexcept;
    };

    class FDCharacter: public FDOutput {
        size_t hintImpl() const noexcept override;

    public:
        FDCharacter(FD& fd) noexcept;
    };

    class FDPipe: public FDOutput {
        size_t hintImpl() const noexcept override;

    public:
        FDPipe(FD& fd) noexcept;
    };
}
./std/ios/out_zc.h
#pragma once

#include "output.h"
#include "unbound.h"
#include "outable.h"

#include <std/sys/types.h>

namespace stl {
    class StringView;

    class ZeroCopyOutput: public Output {
        size_t writeImpl(const void* data, size_t len) override;

        virtual void* imbueImpl(size_t* len) = 0;
        virtual void commitImpl(size_t len) = 0;

    public:
        ~ZeroCopyOutput() noexcept override;

        // zero-copy interface
        UnboundBuffer imbue(size_t len) {
            return {imbueImpl(&len)};
        }

        void* imbue(size_t* avail) {
            return imbueImpl(avail);
        }

        size_t next(void** chunk);

        void next(void** chunk, size_t* len) {
            *len = next(chunk);
        }

        void commit(size_t len) {
            commitImpl(len);
        }

        void recvFromI(Input& in) override;
        void recvFromZ(ZeroCopyInput& in) override;
    };

    template <typename O, typename T>
        requires requires(O o) {
            static_cast<ZeroCopyOutput*>(&o);
        }
    O&& operator<<(O&& out, const T& t) {
        output<ZeroCopyOutput, T>(out, t);

        return static_cast<O&&>(out);
    }
}
./std/ios/in_fd.h
#pragma once

#include "input.h"

namespace stl {
    class FD;

    class FDInput: public Input {
        size_t readImpl(void* data, size_t len) override;
        size_t hintImpl() const noexcept override;

    public:
        FD* fd;

        FDInput(FD& _fd) noexcept
            : fd(&_fd)
        {
        }

        ~FDInput() noexcept override;
    };
}
./std/ios/out_mem.h
#pragma once

#include "out_zc.h"

namespace stl {
    class MemoryOutput: public ZeroCopyOutput {
        void* imbueImpl(size_t* avail) override;
        void commitImpl(size_t len) noexcept override;

    public:
        void* ptr;

        MemoryOutput(void* _ptr) noexcept
            : ptr(_ptr)
        {
        }
    };
}
./std/ios/fs_utils.h
#pragma once

namespace stl {
    class Buffer;

    void readFileContent(Buffer& path, Buffer& out);
}
./std/ios/manip.h
#pragma once

#define flsH (::stl::Manip::Flush())
#define finI (::stl::Manip::Finish())
#define endL (::stl::Manip::EndLine())

namespace stl::Manip {
    struct EndLine {
    };

    struct Flush {
    };

    struct Finish {
    };
}
./std/ios/output.h
#pragma once

#include <std/sys/types.h>

struct iovec;

namespace stl {
    class Input;
    class StringView;
    class ZeroCopyInput;
    class ZeroCopyOutput;

    class Output {
        virtual size_t writeImpl(const void* data, size_t len) = 0;
        virtual size_t hintImpl() const noexcept;

        // have sensible defaults
        virtual void flushImpl();
        virtual void finishImpl();
        virtual size_t writeVImpl(iovec* parts, size_t count);

    public:
        virtual ~Output() noexcept;

        void writeC(const void* data, size_t len);

        size_t writeP(const void* data, size_t len);

        size_t writeV(iovec* parts, size_t count);
        size_t writeV(const StringView* parts, size_t count);

        // zero == no hint
        size_t hint() const noexcept {
            return hintImpl();
        }

        bool hint(size_t* res) const noexcept;
        size_t hint(size_t def) const noexcept;

        size_t write(const void* data, size_t len) {
            return (writeC(data, len), len);
        }

        void flush() {
            flushImpl();
        }

        void finish() {
            finishImpl();
        }

        virtual void recvFromI(Input& in);
        virtual void recvFromZ(ZeroCopyInput& in);
    };
}
./std/ios/unbound.h
#pragma once

#include "outable.h"

#include <std/sys/types.h>

namespace stl {
    struct UnboundBuffer {
        void* ptr;

        size_t distance(UnboundBuffer e) const noexcept {
            return (const u8*)e.ptr - (const u8*)ptr;
        }
    };

    template <typename T>
    UnboundBuffer operator<<(UnboundBuffer out, const T& t) {
        output<UnboundBuffer, T>(out, t);

        return out;
    }
}
./std/ios/outable.h
#pragma once

#include <std/typ/intrin.h>

namespace stl {
    template <typename T>
    concept PassByVal = sizeof(T) <= 2 * sizeof(long double) && stdIsTriviallyCopyable(T);

    template <typename T>
    concept PassByRef = !PassByVal<T>;

    template <typename O, PassByVal T>
    void output(O& out, T t);

    template <typename O, PassByRef T>
    void output(O& out, const T& t);
}
./std/thr/cond_var_iface.h
#pragma once

namespace stl {
    struct MutexIface;

    struct CondVarIface {
        virtual ~CondVarIface() noexcept;

        virtual void wait(MutexIface* mutex) noexcept = 0;
        virtual void signal() noexcept = 0;
        virtual void broadcast() noexcept = 0;
    };
}
./std/thr/coro.h
#pragma once

#include "runable.h"

#include <std/ptr/arc.h>
#include <std/ptr/intrusive.h>

namespace stl {
    struct ThreadPool;
    struct MutexIface;
    struct CondVarIface;
    struct CoroExecutor;

    struct Cont {
        virtual CoroExecutor* executor() noexcept = 0;
    };

    struct SpawnParams {
        size_t stackSize;
        void* stackPtr;
        Runable* runable;

        SpawnParams() noexcept;

        SpawnParams& setStackPtr(void* v) noexcept;
        SpawnParams& setStackSize(size_t v) noexcept;
        SpawnParams& setRunablePtr(Runable* v) noexcept;

        template <typename F>
        SpawnParams& setRunable(F f) {
            return setRunablePtr(makeRunablePtr(f));
        }
    };

    struct CoroExecutor: public ARC {
        using Ref = IntrusivePtr<CoroExecutor>;

        virtual ~CoroExecutor() noexcept;

        virtual void yield() noexcept = 0;
        virtual Cont* me() const noexcept = 0;
        virtual MutexIface* createMutex() = 0;
        virtual CondVarIface* createCondVar() = 0;
        virtual void spawnRun(SpawnParams params) = 0;
        virtual ThreadPool* pool() const noexcept = 0;

        template <typename F>
        void spawn(F f) {
            spawnRun(SpawnParams().setRunable([this, f]() {
                f(this->me());
            }));
        }

        static Ref create(size_t threads);
        static Ref create(ThreadPool* pool);
    };
}
./std/thr/cond_var.h
#pragma once

namespace stl {
    class Mutex;
    struct CondVarIface;
    struct CoroExecutor;

    class CondVar {
        CondVarIface* impl;

    public:
        CondVar();
        CondVar(CoroExecutor* exec);
        CondVar(CondVarIface* iface);

        ~CondVar() noexcept;

        void wait(Mutex& mutex) noexcept;
        void signal() noexcept;
        void broadcast() noexcept;
    };
}
./std/thr/barrier.h
#pragma once

namespace stl {
    struct CoroExecutor;

    class Barrier {
        struct Impl;
        Impl* impl;

    public:
        explicit Barrier(int n);
        Barrier(int n, CoroExecutor* exec);

        ~Barrier() noexcept;

        void wait() noexcept;
    };
}
./std/thr/wait_queue.h
#pragma once

#include <std/ptr/arc.h>
#include <std/sys/types.h>
#include <std/ptr/intrusive.h>

namespace stl {
    class WaitQueue: public ARC {
    public:
        using Ref = IntrusivePtr<WaitQueue>;

        struct Item {
            Item* next = nullptr;
            u8 index = 0;
        };

        virtual ~WaitQueue() noexcept;

        virtual void enqueue(Item* item) noexcept = 0;
        virtual Item* dequeue() noexcept = 0;

        static Ref construct(size_t maxWaiters);
    };
}
./std/thr/wait_group.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    struct CoroExecutor;

    class WaitGroup {
        struct Impl;
        Impl* impl;

    public:
        WaitGroup();
        explicit WaitGroup(CoroExecutor* exec);

        ~WaitGroup() noexcept;

        void add(size_t n) noexcept;
        void done() noexcept;
        void wait() noexcept;
    };
}
./std/thr/pool.h
#pragma once

#include "task.h"
#include "runable.h"

#include <std/ptr/arc.h>
#include <std/sys/types.h>
#include <std/ptr/intrusive.h>

namespace stl {
    u64 registerTlsKey() noexcept;

    struct ThreadPool: public ARC {
        virtual ~ThreadPool() noexcept;

        virtual void submitTask(Task* task) noexcept = 0;
        virtual void join() noexcept = 0;
        virtual void** tls(u64 key) noexcept = 0;

        template <typename F>
        void submit(F f) {
            submitTask(makeTask(f));
        }

        using Ref = IntrusivePtr<ThreadPool>;

        static Ref sync();
        static Ref simple(size_t threads);
        static Ref workStealing(size_t threads);
    };
}
./std/thr/thread.h
#pragma once

#include "runable.h"

#include <std/sys/types.h>

namespace stl {
    // should not be used directly, use ScopedThread or detach(Runable&)
    class Thread {
        struct Impl;
        Impl* impl;

    public:
        explicit Thread(Runable& runable);

        ~Thread() noexcept;

        void join() noexcept;
        void detach() noexcept;

        u64 threadId() const noexcept;

        static u64 currentThreadId() noexcept;
    };

    class ScopedThread {
        Thread thr;

    public:
        template <typename T>
        ScopedThread(T functor)
            : thr(*makeRunablePtr(functor))
        {
        }

        ~ScopedThread() noexcept {
            thr.join();
        }
    };

    void detach(Runable& runable);
}
./std/thr/mutex_iface.h
#pragma once

namespace stl {
    struct MutexIface {
        virtual ~MutexIface() noexcept;

        virtual void lock() noexcept = 0;
        virtual void unlock() noexcept = 0;
        virtual bool tryLock() noexcept = 0;

        virtual void* nativeHandle() noexcept;
    };
}
./std/thr/runable.h
#pragma once

namespace stl {
    struct Runable {
        virtual void run() = 0;
    };

    template <typename V>
    struct RunableImpl: public Runable {
        V v;

        RunableImpl(V vv)
            : v(vv)
        {
        }

        void run() override {
            v();
            delete this;
        }
    };

    template <typename T>
    auto makeRunablePtr(T t) {
        return new RunableImpl<T>(t);
    }
}
./std/thr/latch.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    struct CoroExecutor;

    class Latch {
        struct Impl;
        Impl* impl;

    public:
        explicit Latch(size_t n);
        Latch(size_t n, CoroExecutor* exec);

        ~Latch() noexcept;

        void arrive() noexcept;
        void wait() noexcept;
    };
}
./std/thr/task.h
#pragma once

#include "runable.h"

#include <std/lib/node.h>

namespace stl {
    struct Task: public Runable, public IntrusiveNode {
    };

    template <typename V>
    struct TaskImpl: public Task {
        V v;

        TaskImpl(V vv)
            : v(vv)
        {
        }

        void run() override {
            v();
            delete this;
        }
    };

    template <typename T>
    auto makeTask(T t) {
        return new TaskImpl<T>(t);
    }
}
./std/thr/mutex.h
#pragma once

namespace stl {
    class CondVar;

    struct MutexIface;
    struct CoroExecutor;

    class Mutex {
        friend class CondVar;

        MutexIface* impl;

    public:
        Mutex();
        Mutex(bool lock);
        Mutex(MutexIface* iface);
        Mutex(CoroExecutor* exec);

        ~Mutex() noexcept;

        void lock() noexcept;
        void unlock() noexcept;
        bool tryLock() noexcept;
    };

    class LockGuard {
        Mutex& mutex_;

    public:
        explicit LockGuard(Mutex& mutex)
            : mutex_(mutex)
        {
            mutex_.lock();
        }

        ~LockGuard() noexcept {
            mutex_.unlock();
        }
    };

    class UnlockGuard {
        Mutex& mutex_;

    public:
        explicit UnlockGuard(Mutex& mutex)
            : mutex_(mutex)
        {
            mutex_.unlock();
        }

        ~UnlockGuard() noexcept {
            mutex_.lock();
        }
    };
}
./std/mem/new.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    struct Newable {
        static void* operator new(size_t, void* ptr) noexcept {
            return ptr;
        }
    };
}
./std/mem/free_list.h
#pragma once

#include <std/ptr/arc.h>
#include <std/sys/types.h>
#include <std/ptr/intrusive.h>

namespace stl {
    class FreeList: public ARC {
    public:
        using Ref = IntrusivePtr<FreeList>;

        virtual ~FreeList() noexcept;

        virtual void* allocate() = 0;
        virtual void release(void* ptr) noexcept = 0;

        static Ref fromMemory(size_t objSize);
    };
}
./std/mem/disposer.h
#pragma once

#include "disposable.h"

namespace stl {
    class Disposer {
        Disposable* end = 0;

    public:
        ~Disposer() noexcept {
            dispose();
        }

        void dispose() noexcept;

        void submit(Disposable* d) noexcept {
            d->prev = end;
            end = d;
        }

        unsigned length() const noexcept;
    };
}
./std/mem/obj_list.h
#pragma once

#include "new.h"
#include "embed.h"
#include "free_list.h"

#include <std/typ/support.h>
#include <std/alg/destruct.h>

namespace stl {
    template <typename T>
    class ObjList {
        struct TT: public Embed<T>, public Newable {
            using Embed<T>::Embed;
        };

        static_assert(sizeof(TT) == sizeof(T));

        FreeList::Ref fl = FreeList::fromMemory(sizeof(TT));

    public:
        template <typename... A>
        T* make(A&&... a) {
            return &(new (fl->allocate()) TT(forward<A>(a)...))->t;
        }

        void release(T* t) {
            fl->release(destruct((TT*)(void*)t));
        }
    };
}
./std/mem/mem_pool.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class Disposer;

    class MemoryPool {
        u8* currentChunk;
        u8* currentChunkEnd;
        Disposer* ds;

        void allocateNewChunk(size_t minSize);

    public:
        MemoryPool();
        MemoryPool(void* buf, size_t len) noexcept;

        ~MemoryPool() noexcept;

        void* allocate(size_t len);
    };
}
./std/mem/disposable.h
#pragma once

namespace stl {
    struct Disposable {
        Disposable* prev = 0;

        virtual ~Disposable() noexcept;
    };
}
./std/mem/obj_pool.h
#pragma once

#include "new.h"
#include "embed.h"
#include "disposable.h"

#include <std/ptr/arc.h>
#include <std/sys/types.h>
#include <std/typ/intrin.h>
#include <std/ptr/intrusive.h>

namespace stl {
    class StringView;

    class ObjPool: public ARC {
        virtual void submit(Disposable* d) noexcept = 0;

        template <typename T, typename... A>
        T* makeImpl(A&&... a) {
            return new (allocate(sizeof(T))) T(forward<A>(a)...);
        }

    public:
        using Ref = IntrusivePtr<ObjPool>;

        virtual ~ObjPool() noexcept;

        virtual void* allocate(size_t len) = 0;

        StringView intern(StringView s);

        // king of ownership
        template <typename T, typename... A>
        T* make(A&&... a) {
            struct Wrapper1: public Embed<T>, public Newable {
                using Embed<T>::Embed;
            };

            if constexpr (stdHasTrivialDestructor(T)) {
                static_assert(sizeof(Wrapper1) == sizeof(T));

                return &makeImpl<Wrapper1>(forward<A>(a)...)->t;
            } else {
                struct Wrapper2: public Disposable, public Wrapper1 {
                    using Wrapper1::Wrapper1;
                };

                auto res = makeImpl<Wrapper2>(forward<A>(a)...);

                submit(res);

                return &res->t;
            }
        }

        static Ref fromMemory() {
            return fromMemoryRaw();
        }

        static ObjPool* fromMemoryRaw();
    };
}
./std/mem/embed.h
#pragma once

#include <std/typ/support.h>

namespace stl {
    template <typename T>
    struct Embed {
        T t;

        template <typename... A>
        Embed(A&&... a)
            : t(forward<A>(a)...)
        {
        }
    };
}
./std/lib/visitor.h
#pragma once

namespace stl {
    struct VisitorFace {
        virtual void visit(void*) = 0;
    };

    template <typename V>
    struct Visitor: public VisitorFace {
        V v;

        Visitor(V vv)
            : v(vv)
        {
        }

        void visit(void* el) override {
            v(el);
        }
    };

    template <typename T>
    Visitor<T> makeVisitor(T t) {
        return {t};
    }
}
./std/lib/vector.h
#pragma once

#include "buffer.h"

#include <std/typ/intrin.h>

namespace stl {
    template <typename T>
    class Vector {
        Buffer buf_;

    public:
        static_assert(stdHasTrivialDestructor(T));

        Vector(size_t reserve) {
            grow(reserve);
        }

        Vector() = default;
        Vector(Vector&&) = default;
        Vector(const Vector&) = default;

        auto data() const noexcept {
            return (const T*)buf_.data();
        }

        auto begin() const noexcept {
            return data();
        }

        auto end() const noexcept {
            return (const T*)buf_.current();
        }

        auto storageEnd() const noexcept {
            return begin() + capacity();
        }

        auto mutData() noexcept {
            return const_cast<T*>(data());
        }

        auto mutBegin() noexcept {
            return const_cast<T*>(begin());
        }

        auto mutEnd() noexcept {
            return const_cast<T*>(end());
        }

        auto mutStorageEnd() noexcept {
            return const_cast<T*>(storageEnd());
        }

        size_t left() const noexcept {
            return capacity() - length();
        }

        size_t capacity() const noexcept {
            return buf_.capacity() / sizeof(T);
        }

        size_t length() const noexcept {
            return end() - begin();
        }

        bool empty() const noexcept {
            return buf_.empty();
        }

        auto& mut(size_t i) noexcept {
            return *(mutBegin() + i);
        }

        const auto& operator[](size_t i) const noexcept {
            return *(begin() + i);
        }

        const auto& back() const noexcept {
            return *(end() - 1);
        }

        auto& mutBack() noexcept {
            return *(mutEnd() - 1);
        }

        void clear() noexcept {
            buf_.reset();
        }

        void grow(size_t len) {
            buf_.grow(len * sizeof(T));
        }

        void growDelta(size_t delta) {
            grow(length() + delta);
        }

        void pushBack(const T& t) {
            buf_.append(&t, sizeof(t));
        }

        auto popBack() noexcept {
            auto res = back();

            buf_.seekAbsolute(end() - 1);

            return res;
        }

        void append(const T* b, const T* e) {
            buf_.append((const u8*)b, (const u8*)e - (const u8*)b);
        }

        void append(const T* b, size_t len) {
            append(b, b + len);
        }

        void xchg(Vector& v) noexcept {
            buf_.xchg(v.buf_);
        }

        void zero(size_t cnt) {
            buf_.zero(cnt * sizeof(T));
        }
    };
}
./std/lib/buffer.h
#pragma once

#include <std/mem/new.h>
#include <std/sys/types.h>
#include <std/alg/advance.h>

namespace stl {
    class StringView;

    class Buffer {
        void* data_;

    public:
        struct Header: public Newable {
            size_t used;
            size_t size;
        };

    private:
        Header* header() const noexcept {
            return (Header*)data_ - 1;
        }

        void appendUnsafe(const void* data, size_t len);

    public:
        ~Buffer() noexcept;

        Buffer(size_t len);
        Buffer(StringView v);
        Buffer(const void* data, size_t len);

        Buffer() noexcept;
        Buffer(const Buffer& buf);
        Buffer(Buffer&& buf) noexcept;

        Buffer& operator=(const Buffer& buf) {
            Buffer(buf).xchg(*this);

            return *this;
        }

        Buffer& operator=(Buffer&& buf) noexcept {
            Buffer tmp;

            tmp.xchg(buf);
            tmp.xchg(*this);

            return *this;
        }

        auto data() const noexcept {
            return data_;
        }

        auto mutData() noexcept {
            return data_;
        }

        auto current() const noexcept {
            return advancePtr(data(), used());
        }

        auto mutCurrent() noexcept {
            return const_cast<void*>(current());
        }

        auto storageEnd() const noexcept {
            return advancePtr(data(), capacity());
        }

        auto mutStorageEnd() noexcept {
            return const_cast<void*>(storageEnd());
        }

        size_t capacity() const noexcept {
            return header()->size;
        }

        size_t used() const noexcept {
            return header()->used;
        }

        size_t length() const noexcept {
            return used();
        }

        bool empty() const noexcept {
            return used() == 0;
        }

        size_t left() const noexcept {
            return capacity() - used();
        }

        void xchg(Buffer& buf) noexcept;

        void reset() noexcept {
            seekAbsolute((size_t)0);
        }

        void seekRelative(size_t len) noexcept {
            seekAbsolute(used() + len);
        }

        void seekNegative(size_t len) noexcept {
            seekAbsolute(used() - len);
        }

        void seekAbsolute(size_t pos) noexcept;

        void seekAbsolute(const void* ptr) noexcept {
            seekAbsolute(offsetOf(ptr));
        }

        size_t offsetOf(const void* ptr) noexcept {
            return (const u8*)ptr - (const u8*)data();
        }

        void shrinkToFit();
        void grow(size_t size);
        void setCapacity(size_t cap) noexcept;
        void append(const void* data, size_t len);

        void growDelta(size_t len) {
            grow(used() + len);
        }

        void* imbueMe(size_t* len);

        char* cStr();
        void zero(size_t len);
    };
}
./std/lib/list.h
#pragma once

#include "node.h"

namespace stl {
    class IntrusiveList {
        IntrusiveNode head;

    public:
        ~IntrusiveList() noexcept {
            clear();
        }

        IntrusiveList() noexcept {
        }

        IntrusiveList(IntrusiveList&& r) noexcept {
            r.xchgWithEmptyList(*this);
        }

        bool empty() const noexcept {
            return head.next == &head;
        }

        void pushBack(IntrusiveNode* node) noexcept {
            insertAfter(head.prev, node);
        }

        void pushBack(IntrusiveList& lst) noexcept;

        void pushFront(IntrusiveNode* node) noexcept {
            insertAfter(&head, node);
        }

        IntrusiveNode* popFront() noexcept {
            return head.next->remove();
        }

        IntrusiveNode* popFrontOrNull() noexcept;

        IntrusiveNode* popBack() noexcept {
            return head.prev->remove();
        }

        IntrusiveNode* popBackOrNull() noexcept;

        void clear() noexcept {
            head.remove();
        }

        IntrusiveNode* mutFront() noexcept {
            return head.next;
        }

        const IntrusiveNode* front() const noexcept {
            return head.next;
        }

        IntrusiveNode* mutBack() noexcept {
            return head.prev;
        }

        const IntrusiveNode* back() const noexcept {
            return head.prev;
        }

        IntrusiveNode* mutEnd() noexcept {
            return &head;
        }

        const IntrusiveNode* end() const noexcept {
            return &head;
        }

        bool almostEmpty() const noexcept {
            return head.almostEmpty();
        }

        unsigned length() const noexcept;

        static void insertAfter(IntrusiveNode* pos, IntrusiveNode* node) noexcept;

        static void insertBefore(IntrusiveNode* pos, IntrusiveNode* node) noexcept {
            insertAfter(pos->prev, node);
        }

        void xchg(IntrusiveList& r) noexcept;
        void xchgWithEmptyList(IntrusiveList& r) noexcept;

        using Compare1 = bool (*)(const IntrusiveNode*, const IntrusiveNode*);
        using Compare2 = bool (*)(void*, const IntrusiveNode*, const IntrusiveNode*);

        void sort(Compare1 cmp) noexcept;
        void sort(Compare2 cmp, void* ctx) noexcept;

        template <typename F>
        void sort(const F& f) noexcept {
            sort([](void* ctx, const IntrusiveNode* l, const IntrusiveNode* r) -> bool {
                return (*(F*)ctx)(l, r);
            }, (void*)&f);
        }

        void splitHalf(IntrusiveList& l, IntrusiveList& r) noexcept;
    };
}
./std/lib/node.h
#pragma once

namespace stl {
    struct IntrusiveNode {
        IntrusiveNode* prev = this;
        IntrusiveNode* next = this;

        auto remove() noexcept {
            return (unlink(), this);
        }

        void reset() noexcept;
        void unlink() noexcept;
        void xchg(IntrusiveNode& r);

        bool singular() const noexcept;
        bool almostEmpty() const noexcept;
    };
}
./std/map/map.h
#pragma once

#include "treap.h"
#include "treap_node.h"

#include <std/typ/intrin.h>
#include <std/typ/support.h>
#include <std/alg/destruct.h>
#include <std/mem/obj_list.h>

namespace stl {
    template <typename K, typename V>
    class Map {
        static void* tov(const K& k) noexcept {
            return (void*)&k;
        }

        struct Impl: public Treap {
            bool cmp(void* l, void* r) const noexcept override {
                return *(K*)l < *(K*)r;
            }
        };

        struct Node: public TreapNode {
            K k;
            V v;

            template <typename... A>
            Node(K key, A&&... a)
                : k(key)
                , v(forward<A>(a)...)
            {
            }

            void* key() const noexcept override {
                return tov(k);
            }
        };

        ObjList<Node> ol;
        Impl map;

        template <typename... A>
        V* insertNew(K key, A&&... a) {
            auto node = ol.make(key, forward<A>(a)...);
            map.insert(node);
            return &node->v;
        }

    public:
        ~Map() noexcept {
            if constexpr (!stdHasTrivialDestructor(Node)) {
                map.visit([](auto ptr) {
                    destruct((Node*)ptr);
                });
            }
        }

        V* find(K k) const noexcept {
            if (auto res = (Node*)map.find(tov(k)); res) {
                return &res->v;
            }

            return nullptr;
        }

        template <typename... A>
        V* insert(K key, A&&... a) {
            return (erase(key), insertNew(key, forward<A>(a)...));
        }

        V& operator[](K key) {
            if (auto res = find(key); res) {
                return *res;
            }

            return *insertNew(key);
        }

        void erase(K key) noexcept {
            if (auto res = (Node*)map.erase(tov(key)); res) {
                ol.release(res);
            }
        }

        template <typename F>
        void visit(F v) {
            map.visit([v](TreapNode* ptr) {
                v(((const Node*)ptr)->k, ((Node*)ptr)->v);
            });
        }
    };
}
./std/map/treap_node.h
#pragma once

namespace stl {
    struct VisitorFace;

    struct TreapNode {
        TreapNode* left = nullptr;
        TreapNode* right = nullptr;

        virtual void* key() const noexcept;

        void visit(VisitorFace& vis);
        unsigned height() const noexcept;
    };
}
./std/map/treap.h
#pragma once

#include <std/sys/types.h>
#include <std/lib/visitor.h>

namespace stl {
    struct TreapNode;

    class Treap {
        TreapNode* root = nullptr;

        void visitImpl(VisitorFace&& vis);
        void split(TreapNode* t, void* k, TreapNode** l, TreapNode** r) noexcept;

        virtual bool cmp(void* a, void* b) const noexcept = 0;

    public:
        template <typename V>
        void visit(V v) {
            visitImpl(makeVisitor([v](void* ptr) {
                v((TreapNode*)ptr);
            }));
        }

        TreapNode* find(void* key) const noexcept;
        TreapNode* erase(void* key) noexcept;
        TreapNode* remove(TreapNode* node) noexcept;

        void insert(TreapNode* node) noexcept;

        size_t height() const noexcept;
        size_t length() const noexcept;
    };
}
./std/str/hash.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    u32 shash32(const void* data, size_t len) noexcept;
    u64 shash64(const void* data, size_t len) noexcept;
}
./std/str/builder.h
#pragma once

#include <std/ios/out_zc.h>
#include <std/lib/buffer.h>

namespace stl {
    class StringBuilder: public ZeroCopyOutput, public Buffer {
        size_t writeImpl(const void* ptr, size_t len) override;
        void* imbueImpl(size_t* len) override;
        void commitImpl(size_t len) noexcept override;

    public:
        StringBuilder() noexcept;
        StringBuilder(size_t reserve);
        StringBuilder(Buffer&& buf) noexcept;

        ~StringBuilder() noexcept override;
    };
}
./std/str/view.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class Buffer;

    class StringView {
        const u8* ptr_;
        size_t len_;

    public:
        StringView() noexcept
            : StringView(nullptr, (size_t)0)
        {
        }

        template <size_t N>
        StringView(const u8 (&str)[N]) noexcept
            : StringView(str, N - 1)
        {
        }

        StringView(const u8* ptr, size_t len) noexcept
            : ptr_(ptr)
            , len_(len)
        {
        }

        StringView(const u8* b, const u8* e) noexcept
            : StringView(b, e - b)
        {
        }

        StringView(const char* s) noexcept;
        StringView(const Buffer& b) noexcept;

        auto data() const noexcept {
            return ptr_;
        }

        auto length() const noexcept {
            return len_;
        }

        // iterator ops
        auto begin() const noexcept {
            return data();
        }

        auto end() const noexcept {
            return begin() + length();
        }

        const auto& operator[](size_t i) const noexcept {
            return *(begin() + i);
        }

        bool empty() const noexcept {
            return length() == 0;
        }

        const auto& back() const noexcept {
            return *(end() - 1);
        }

        // string ops
        StringView prefix(size_t len) const noexcept;
        StringView suffix(size_t len) const noexcept;

        bool endsWith(StringView suffix) const noexcept;
        bool startsWith(StringView prefix) const noexcept;

        const u8* memChr(u8 ch) const noexcept;
        const u8* search(StringView substr) const noexcept;

        // hash ops
        u32 hash32() const noexcept;
        u64 hash64() const noexcept;

        // parse
        u64 stou() const noexcept;
    };

    bool operator==(StringView l, StringView r) noexcept;
    bool operator!=(StringView l, StringView r) noexcept;
    bool operator<(StringView l, StringView r) noexcept;
}
./std/str/fmt.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    void* formatU64Base10(u64 v, void* buf) noexcept;
    void* formatI64Base10(i64 v, void* buf) noexcept;
    void* formatLongDouble(long double v, void* buf) noexcept;
}
./std/tst/ctx.h
#pragma once

namespace stl {
    struct Ctx {
        int argc;
        char** argv;

        virtual void printTB() const = 0;

        void run();
    };
}
./std/tst/ut.h
#pragma once

#include <std/ios/sys.h>
#include <std/str/view.h>
#include <std/sys/types.h>
#include <std/dbg/insist.h>
#include <std/map/treap_node.h>

namespace stl {
    class ZeroCopyOutput;

    struct ExecContext {
        virtual ZeroCopyOutput& output() const = 0;
    };

    struct TestFunc: public TreapNode {
        virtual void execute(ExecContext& ctx) const = 0;
        virtual StringView suite() const = 0;
        virtual StringView name() const = 0;

        void registerMe();
    };
}

#define STD_TEST_SUITE(_name)                     \
    namespace Suite_##_name {                     \
        using S_V = ::stl::StringView;            \
        static const auto S_N = S_V(u8## #_name); \
    }                                             \
    namespace Suite_##_name

#define STD_TEST(_name)                           \
    static struct Test_##_name: ::stl::TestFunc { \
        Test_##_name() {                          \
            registerMe();                         \
        }                                         \
        ::stl::StringView suite() const {         \
            return S_N;                           \
        }                                         \
        ::stl::StringView name() const {          \
            return u8## #_name;                   \
        }                                         \
        void execute(ExecContext& ctx) const;     \
    } REG_##_name;                                \
    void Test_##_name::execute([[maybe_unused]] ExecContext& _ctx) const
./std/dbg/panic.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    using PanicHandler = void (*)();

    PanicHandler setPanicHandler1(PanicHandler hndl) noexcept;
    PanicHandler setPanicHandler2(PanicHandler hndl) noexcept;

    void panic(const u8* what, u32 line, const u8* file);
}
./std/dbg/color.h
#pragma once

namespace stl {
    enum class AnsiColor {
        Reset = 0,

        Black,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White,
    };

    struct Color {
        AnsiColor color;
        bool brightKind;

        static auto dark(AnsiColor c) noexcept {
            return Color{c, false};
        }

        static auto bright(AnsiColor c) noexcept {
            return Color{c, true};
        }

        static auto reset() noexcept {
            return Color{AnsiColor::Reset, false};
        }
    };
}
./std/dbg/assert.h
#pragma once

#if defined(NDEBUG) && !defined(ENABLE_ASSERT)
    #define STD_ASSERT(X)
#else
    #include "insist.h"

    #define STD_ASSERT(X) STD_INSIST(X)
#endif
./std/dbg/insist.h
#pragma once

#include "panic.h"

#include <std/sys/types.h>

#define STD_CAT(X, Y) STD_CA_(X, Y)
#define STD_CA_(X, Y) STD_C__(X, Y)
#define STD_C__(X, Y) X##Y

#define STD_STR(X) STD_ST_(X)
#define STD_ST_(X) #X

#define STD_INSIST(X)                   \
    do {                                \
        if (!(X)) {                     \
            ::stl::panic(               \
                STD_CAT(u8,             \
                        STD_STR(X)),    \
                __LINE__,               \
                STD_CAT(u8, __FILE__)); \
        }                               \
    } while (false)
./std/ptr/arc.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class ARC {
        i32 counter_;

    public:
        ARC() noexcept;

        i32 ref() noexcept;
        i32 refCount() const noexcept;
        i32 unref() noexcept;
    };
}
./std/ptr/refcount.h
#pragma once

#include <std/typ/support.h>

namespace stl {
    void xchgPtr(void** l, void** r);

    template <typename T, typename O>
    class RefCountPtr {
        T* t_;

    public:
        RefCountPtr(T* t) noexcept
            : t_(t)
        {
            O::ref(t_);
        }

        RefCountPtr(const RefCountPtr& ptr) noexcept
            : RefCountPtr(ptr.t_)
        {
        }

        ~RefCountPtr() noexcept {
            O::unref(t_);
        }

        RefCountPtr& operator=(const RefCountPtr& r) noexcept {
            RefCountPtr(r).xchg(*this);

            return *this;
        }

        template <typename... A>
        static auto make(A&&... a) {
            return RefCountPtr(new T(forward<A>(a)...));
        }

        auto ptr() const noexcept {
            return O::ptr(t_);
        }

        auto mutPtr() noexcept {
            return O::mutPtr(t_);
        }

        auto refCount() const noexcept {
            return t_->refCount();
        }

        // sugar
        auto operator->() noexcept {
            return mutPtr();
        }

        auto operator->() const noexcept {
            return ptr();
        }

        auto& operator*() noexcept {
            return *mutPtr();
        }

        auto& operator*() const noexcept {
            return *ptr();
        }

        void xchg(RefCountPtr& r) noexcept {
            xchgPtr((void**)&t_, (void**)&r.t_);
        }
    };

    template <typename T>
    struct RefCountOps {
        static auto ref(T* t) noexcept {
            t->ref();
        }

        static auto unref(T* t) noexcept {
            if (t->refCount() == 1 || t->unref() == 0) {
                delete t;
            }
        }
    };
}
./std/ptr/intrusive.h
#pragma once

#include "refcount.h"

namespace stl::IPP {
    template <typename T>
    struct Ops: public RefCountOps<T> {
        static auto ptr(const T* t) noexcept {
            return t;
        }

        static auto mutPtr(T* t) noexcept {
            return t;
        }
    };
}

namespace stl {
    template <typename T>
    using IntrusivePtr = RefCountPtr<T, IPP::Ops<T>>;
}
./std/ptr/scoped.h
#pragma once

namespace stl {
    template <typename T>
    struct ScopedPtr {
        T* ptr;

        void drop() noexcept {
            ptr = 0;
        }

        ~ScopedPtr() {
            delete ptr;
        }
    };
}
./std/ptr/shared.h
#pragma once

#include "refcount.h"

#include <std/mem/embed.h>
#include <std/typ/support.h>

namespace stl::SPP {
    template <typename T>
    struct Ops: public RefCountOps<T> {
        static auto ptr(const T* t) noexcept {
            return &t->t;
        }

        static auto mutPtr(T* t) noexcept {
            return &t->t;
        }
    };

    template <typename T, typename R>
    struct Base: public Embed<T>, public R {
        using Embed<T>::Embed;
    };
}

namespace stl {
    class ARC;

    template <typename T, typename R>
    using SharedPtr = RefCountPtr<SPP::Base<T, R>, SPP::Ops<SPP::Base<T, R>>>;

    template <typename T>
    using AtomicSharedPtr = SharedPtr<T, ARC>;
}
./std/typ/support.h
#pragma once

namespace stl {
    template <typename T>
    using rem_ref = __remove_reference_t(T);

    // move semantics
    template <typename T>
    constexpr rem_ref<T>&& move(T&& t) noexcept {
        return static_cast<rem_ref<T>&&>(t);
    }

    // perfect forwarding
    template <typename T>
    constexpr T&& forward(rem_ref<T>& t) noexcept {
        return static_cast<T&&>(t);
    }

    template <typename T>
    constexpr T&& forward(rem_ref<T>&& t) noexcept {
        return static_cast<T&&>(t);
    }
}
./std/typ/intrin.h
#pragma once

// portable defines for compiler intrinsics

#define stdIsTriviallyCopyable __is_trivially_copyable
#define stdHasTrivialDestructor __has_trivial_destructor
./all.cpp
./std/alg/qsort.cpp
#include "qsort.h"
./std/alg/reverse.cpp
#include "reverse.h"
./std/alg/defer.cpp
#include "defer.h"
./std/alg/advance.cpp
#include "advance.h"
./std/alg/exchange.cpp
#include "exchange.h"
./std/alg/minmax.cpp
#include "minmax.h"
./std/alg/shuffle.cpp
#include "shuffle.h"
./std/alg/range.cpp
#include "range.h"
./std/alg/bits.cpp
#include "bits.h"
./std/alg/destruct.cpp
#include "destruct.h"
./std/alg/xchg.cpp
#include "xchg.h"
./std/sym/i_map.cpp
#include "i_map.h"
./std/sym/h_map.cpp
#include "h_map.h"
./std/sym/s_map.cpp
#include "s_map.h"
./std/sym/h_table.cpp
#include "h_table.h"

#include <std/sys/crt.h>
#include <std/alg/range.h>
#include <std/alg/minmax.h>
#include <std/alg/exchange.h>

using namespace stl;

namespace {
    static auto buckets(const Buffer& buf) noexcept {
        return range((HashTable::Node**)buf.data(), (HashTable::Node**)buf.storageEnd());
    }
}

HashTable::HashTable(size_t initialCapacity)
    : buf(max(initialCapacity, (size_t)1) * sizeof(Node*))
{
    // here we assume (2^N - 2 * sizeof(void*)) % sizeof(void*) == 0 for N > 3
    memZero(buf.mutData(), buf.mutStorageEnd());
}

HashTable::~HashTable() noexcept {
}

void HashTable::xchg(HashTable& t) noexcept {
    buf.xchg(t.buf);
}

size_t HashTable::size() const noexcept {
    return buf.used();
}

HashTable::Node* HashTable::find(u64 key) const noexcept {
    for (auto node = *bucketFor(key); node; node = node->next) {
        if (node->key == key) {
            return node;
        }
    }

    return nullptr;
}

size_t HashTable::capacity() const noexcept {
    return buckets(buf).length();
}

void HashTable::rehash(size_t len) {
    HashTable next(len);

    visit([&](auto node) {
        next.addNoRehash(node);
    });

    next.xchg(*this);
}

HashTable::Node* HashTable::insert(Node* nn) {
    if (auto cap = capacity(); size() >= cap * 0.7) {
        rehash(cap * 1.5);
    }

    auto res = erase(nn->key);

    return (addNoRehash(nn), res);
}

HashTable::Node* HashTable::erase(u64 key) noexcept {
    for (auto ptr = bucketFor(key); *ptr; ptr = &(*ptr)->next) {
        if (auto node = *ptr; node->key == key) {
            *ptr = node->next;
            buf.seekNegative(1);

            return node;
        }
    }

    return nullptr;
}

HashTable::Node** HashTable::bucketFor(u64 key) const noexcept {
    auto bb = buckets(buf);

    return &bb.b[key % bb.length()];
}

void HashTable::addNoRehash(Node* nn) {
    auto b = bucketFor(nn->key);

    nn->next = *b;
    *b = nn;

    buf.seekRelative(1);
}

void HashTable::visitImpl(VisitorFace&& v) {
    for (auto node : buckets(buf)) {
        while (node) {
            v.visit(exchange(node, node->next));
        }
    }
}
./std/rng/pcg.cpp
#include "pcg.h"
#include "split_mix_64.h"

#include <std/alg/exchange.h>

using namespace stl;

namespace {
    static u32 xorShift(u64 v) noexcept {
        const u32 xorshifted = ((v >> 18u) ^ v) >> 27u;
        const u32 rot = v >> 59u;

        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }
}

PCG32::PCG32(u64 seq) noexcept
    : PCG32(splitMix64((size_t)this), seq)
{
}

PCG32::PCG32(const void* seq) noexcept
    : PCG32(splitMix64((size_t)seq))
{
}

PCG32::PCG32(u64 state, u64 seq) noexcept
    : state_(0)
    , seq_(2ULL * seq + 1ULL)
{
    nextU32();
    state_ += state;
    nextU32();
}

u32 PCG32::nextU32() noexcept {
    return xorShift(exchange(state_, state_ * 6364136223846793005ULL + seq_));
}

u32 PCG32::uniformUnbiased(u32 n) noexcept {
    const u32 limit = -n % n; // 2^32 % n — число «лишних» значений сверху
    u32 x;

    do {
        x = nextU32();
    } while (x < limit);

    return x % n;
}
./std/rng/split_mix_64.cpp
#include "split_mix_64.h"

using namespace stl;

u64 stl::splitMix64(u64 x) noexcept {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9U;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebU;
    x ^= x >> 31;

    return x;
}

u64 stl::nextSplitMix64(u64* x) noexcept {
    return splitMix64(*x += 0x9e3779b97f4a7c15);
}
./std/sys/types.cpp
#include "types.h"

static_assert(sizeof(i8) == 1);
static_assert(sizeof(u8) == 1);

static_assert(sizeof(i16) == 2);
static_assert(sizeof(u16) == 2);

static_assert(sizeof(i32) == 4);
static_assert(sizeof(u32) == 4);

static_assert(sizeof(i64) == 8);
static_assert(sizeof(u64) == 8);
./std/sys/crt.cpp
#include "crt.h"

#include <std/dbg/insist.h>
#include <std/ios/output.h>

#include <stdlib.h>
#include <string.h>

void* stl::allocateMemory(size_t len) {
    if (auto ret = malloc(len); ret) {
        return ret;
    }

    STD_INSIST(false);

    return 0;
}

void stl::freeMemory(void* ptr) noexcept {
    free(ptr);
}

int stl::memCmp(const void* l, const void* r, size_t len) noexcept {
    if (len == 0) {
        return 0;
    }

    return memcmp(l, r, len);
}

void* stl::memCpy(void* to, const void* from, size_t len) noexcept {
    if (len) {
        memcpy(to, from, len);
    }

    return len + (u8*)to;
}

size_t stl::strLen(const u8* s) noexcept {
    return s ? strlen((const char*)s) : 0;
}

void stl::memZero(void* from, void* to) noexcept {
    const size_t len = (u8*)to - (u8*)from;

    if (len) {
        memset(from, 0, len);
    }
}
./std/sys/fd.cpp
#include "fd.h"

#include <std/alg/xchg.h>
#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/alg/minmax.h>
#include <std/str/builder.h>
#include <std/alg/exchange.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>

using namespace stl;

size_t FD::read(void* data, size_t len) {
    if (auto res = ::read(fd, data, min(len, (size_t)(0x7ffff000 - 1))); res >= 0) {
        return res;
    }

    if (errno == EFAULT && len > 1024) {
        return read(data, len / 2);
    }

    Errno().raise(StringBuilder() << StringView(u8"read() failed"));
}

size_t FD::write(const void* data, size_t len) {
    auto res = ::write(fd, data, len);

    if (res < 0) {
        Errno().raise(StringBuilder() << StringView(u8"write() failed"));
    }

    return res;
}

size_t FD::writeV(iovec* parts, size_t count) {
    auto res = writev(fd, parts, count);

    if (res < 0) {
        Errno().raise(StringBuilder() << StringView(u8"writev() failed"));
    }

    return res;
}

void FD::fsync() {
    if (::fsync(fd) < 0) {
        Errno().raise(StringBuilder() << StringView(u8"fsync() failed"));
    }
}

void FD::close() {
    if (fd < 0) {
        return;
    }

    if (::close(exchange(fd, -1)) < 0) {
        Errno().raise(StringBuilder() << StringView(u8"close() failed"));
    }
}

void FD::xchg(FD& other) noexcept {
    ::stl::xchg(fd, other.fd);
}

ScopedFD::~ScopedFD() noexcept(false) {
    close();
}

void stl::createPipeFD(ScopedFD& in, ScopedFD& out) {
    int fd[2];

    if (auto res = pipe(fd); res < 0) {
        Errno().raise(StringBuilder() << StringView(u8"pipe() failed"));
    }

    ScopedFD(fd[0]).xchg(in);
    ScopedFD(fd[1]).xchg(out);
}
./std/sys/atomic.cpp
#include "atomic.h"
./std/sys/fs.cpp
#include "fs.h"

#include <std/alg/defer.h>
#include <std/sys/throw.h>
#include <std/lib/buffer.h>
#include <std/str/builder.h>

#include <dirent.h>
#include <sys/types.h>

using namespace stl;

void stl::listDirImpl(StringView path, VisitorFace&& vis) {
    DIR* dir = opendir(Buffer(path).cStr());

    if (!dir) {
        Errno().raise(StringBuilder() << StringView(u8"opendir() failed for ") << path);
    }

    STD_DEFER {
        if (closedir(dir) != 0) {
            Errno().raise(StringBuilder() << StringView(u8"closedir() failed for ") << path);
        }
    };

    while (struct dirent* entry = readdir(dir)) {
        StringView name((const char*)entry->d_name);

        if (name == StringView(u8".") || name == StringView(u8"..")) {
            continue;
        }

        TPathInfo pi = {
            .item = name,
            .isDir = entry->d_type == DT_DIR,
        };

        // name points into entry->d_name, which is valid until the next readdir call —
        // vis.visit() completes before that, so no dangling reference here.
        vis.visit(&pi);
    }
}
./std/sys/throw.cpp
#include "throw.h"

#include <std/str/view.h>
#include <std/lib/buffer.h>
#include <std/typ/support.h>
#include <std/str/builder.h>

#include <errno.h>
#include <string.h>

using namespace stl;

namespace {
    struct ErrnoError: public Exception {
        Buffer full;
        Buffer text;
        int error;

        ErrnoError(int e, Buffer&& t) noexcept
            : text(move(t))
            , error(e)
        {
        }

        ExceptionKind kind() const noexcept override {
            return ExceptionKind::Errno;
        }

        StringView description() override {
            if (!full.empty()) {
                return full;
            }

            (StringBuilder()
             << StringView(u8"(code ")
             << error
             << StringView(u8", descr ")
             << (const char*)strerror(error)
             << StringView(u8") ")
             << text)
                .xchg(full);

            return full;
        }
    };
}

Exception::~Exception() noexcept {
}

StringView Exception::current() {
    try {
        throw;
    } catch (Exception& e) {
        return e.description();
    } catch (...) {
        return StringView(u8"unknown exception");
    }
}

Errno::Errno() noexcept
    : error(errno)
{
    errno = 0;
}

void Errno::raise(Buffer&& text) {
    throw ErrnoError(error, move(text));
}
./std/ios/output.cpp
#include "output.h"
#include "in_zc.h"
#include "out_buf.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/alg/range.h>
#include <std/alg/exchange.h>

#include <alloca.h>
#include <sys/uio.h>

using namespace stl;

Output::~Output() noexcept {
}

void Output::flushImpl() {
}

void Output::finishImpl() {
}

size_t Output::writeV(iovec* iov, size_t iovcnt) {
    size_t res = 0;

    while (iovcnt > 0) {
        auto written = writeVImpl(iov, iovcnt);

        res += written;

        while (written >= iov->iov_len) {
            written -= iov->iov_len;
            iov++;
            iovcnt--;
        }

        if (written > 0) {
            iov->iov_base = (char*)iov->iov_base + written;
            iov->iov_len -= written;
        }
    }

    return res;
}

size_t Output::writeVImpl(iovec* parts, size_t count) {
    size_t res = 0;

    for (const auto& it : range(parts, parts + count)) {
        res += write(it.iov_base, it.iov_len);
    }

    return res;
}

size_t Output::writeV(const StringView* parts, size_t count) {
    auto io = (iovec*)alloca(count * sizeof(iovec));

    memZero(io, io + count);

    for (size_t i = 0; i < count; ++i) {
        io[i].iov_len = parts[i].length();
        io[i].iov_base = (void*)parts[i].data();
    }

    return writeV(io, count);
}

void Output::writeC(const void* data, size_t len) {
    auto b = (u8*)data;
    auto e = b + len;

    while (b < e) {
        b += writeImpl(b, e - b);
    }
}

size_t Output::hintImpl() const noexcept {
    return 0;
}

bool Output::hint(size_t* res) const noexcept {
    if (const auto h = hintImpl(); h) {
        *res = h;

        return true;
    }

    return false;
}

size_t Output::writeP(const void* data, size_t len) {
    if (len) {
        return writeImpl(data, len);
    }

    return 0;
}

void Output::recvFromI(Input& in) {
    OutBuf(*this).recvFromI(in);
}

void Output::recvFromZ(ZeroCopyInput& in) {
    const void* chunk;

    while (auto len = in.next(&chunk)) {
        in.commit(writeP(chunk, len));
    }
}

size_t Output::hint(size_t def) const noexcept {
    if (auto h = hint(); h) {
        return h;
    }

    return def;
}
./std/ios/input.cpp
#include "input.h"

#include <std/alg/defer.h>
#include <std/lib/buffer.h>
#include <std/str/builder.h>

using namespace stl;

Input::~Input() noexcept {
}

void Input::readAll(Buffer& res) {
    StringBuilder sb;

    sb.xchg(res);

    STD_DEFER {
        sb.xchg(res);
    };

    sendTo(sb);
}

void Input::sendTo(Output& out) {
    out.recvFromI(*this);
}

size_t Input::hintImpl() const noexcept {
    return 0;
}
./std/ios/in_buf.cpp
#include "in_buf.h"

#include "input.h"

#include <std/alg/xchg.h>

using namespace stl;

InBuf::~InBuf() {
}

InBuf::InBuf(Input& in) noexcept
    : InBuf(in, in.hint(1 << 14))
{
}

InBuf::InBuf(Input& in, size_t chunkSize) noexcept
    : in_(&in)
    , buf(chunkSize)
    , pos(0)
{
    // buf.setCapacity(chunkSize);
}

InBuf::InBuf() noexcept
    : in_(nullptr)
    , pos(0)
{
}

size_t InBuf::hintImpl() const noexcept {
    return buf.capacity();
}

size_t InBuf::nextImpl(const void** ptr) {
    if (pos >= buf.used()) {
        buf.seekAbsolute(in_->read(buf.mutData(), buf.capacity()));
        pos = 0;
    }

    return (*ptr = (const u8*)buf.data() + pos, buf.used() - pos);
}

void InBuf::commitImpl(size_t len) noexcept {
    pos += len;
}

void InBuf::xchg(InBuf& r) noexcept {
    ::stl::xchg(buf, r.buf);
    ::stl::xchg(in_, r.in_);
    ::stl::xchg(pos, r.pos);
}
./std/ios/out_buf.cpp
#include "out_buf.h"

#include "manip.h"
#include "output.h"
#include "unbound.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/alg/xchg.h>
#include <std/alg/defer.h>
#include <std/alg/minmax.h>
#include <std/alg/advance.h>

#include <sys/uio.h>

using namespace stl;

OutBuf::~OutBuf() {
    if (out_) {
        finish();
    }
}

OutBuf::OutBuf(Output& out) noexcept
    : OutBuf(out, out.hint(1 << 14))
{
}

OutBuf::OutBuf(Output& out, size_t chunkSize) noexcept
    : out_(&out)
    , chunk(chunkSize)
{
}

OutBuf::OutBuf() noexcept
    : out_(nullptr)
    , chunk(0)
{
}

void* OutBuf::imbueImpl(size_t* len) {
    return buf_.imbueMe(len);
}

void OutBuf::commitImpl(size_t len) {
    buf_.seekRelative(len);

    if (buf_.used() >= chunk) {
        Buffer buf;
        buf.xchg(buf_);
        write(buf.data(), buf.length());
    }
}

size_t OutBuf::writeDirect(const void* ptr, size_t len) {
    return out_->writeP(ptr, len - len % chunk);
}

size_t OutBuf::writeMultipart(const void* ptr, size_t len) {
    Buffer buf;

    buf.xchg(buf_);

    STD_DEFER {
        buf.reset();
        buf.xchg(buf_);
    };

    iovec parts[] = {
        {
            .iov_base = (void*)buf.data(),
            .iov_len = buf.used(),
        },
        {
            .iov_base = (void*)ptr,
            .iov_len = len - (buf.used() + len) % chunk,
        },
    };

    return out_->writeV(parts, 2);
}

size_t OutBuf::writeImpl(const void* ptr, size_t len) {
    if (buf_.used() + len < chunk) {
        return (buf_.append(ptr, len), len);
    } else if (const auto used = buf_.used(); used) {
        return writeMultipart(ptr, len) - used;
    } else {
        return writeDirect(ptr, len);
    }
}

size_t OutBuf::hintImpl() const noexcept {
    return chunk;
}

void OutBuf::flushImpl() {
    Buffer buf;

    buf.xchg(buf_);

    STD_DEFER {
        buf.reset();
        buf.xchg(buf_);
    };

    out_->write(buf.data(), buf.used());
}

void OutBuf::finishImpl() {
    STD_DEFER {
        out_ = nullptr;
    };

    flush();
}

void OutBuf::xchg(OutBuf& buf) noexcept {
    ::stl::xchg(buf_, buf.buf_);
    ::stl::xchg(out_, buf.out_);
    ::stl::xchg(chunk, buf.chunk);
}
./std/ios/in_mem.cpp
#include "in_mem.h"
#include "output.h"

using namespace stl;

size_t MemoryInput::nextImpl(const void** chunk) {
    return (*chunk = b, e - b);
}

void MemoryInput::commitImpl(size_t len) noexcept {
    b += len;
}

void MemoryInput::sendTo(Output& out) {
    b += out.write(b, e - b);
}
./std/ios/manip.cpp
#include "manip.h"
#include "out_zc.h"

using namespace stl;
using namespace stl::Manip;

// modifiers
template <>
void stl::output<ZeroCopyOutput, Flush>(ZeroCopyOutput& out, Flush) {
    out.flush();
}

template <>
void stl::output<ZeroCopyOutput, Finish>(ZeroCopyOutput& out, Finish) {
    out.finish();
}

template <>
void stl::output<ZeroCopyOutput, EndLine>(ZeroCopyOutput& out, EndLine) {
    out.write(u8"\n", 1);
}
./std/ios/out_zc.cpp
#include "out_zc.h"
#include "manip.h"
#include "input.h"
#include "in_zc.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/alg/minmax.h>
#include <std/alg/advance.h>

using namespace stl;
using namespace stl::Manip;

namespace {
    struct U64 {
        u64 val;
    };

    struct I64 {
        i64 val;
    };
}

ZeroCopyOutput::~ZeroCopyOutput() noexcept {
}

void ZeroCopyOutput::recvFromI(Input& in) {
    void* chunk;
    size_t clen;

    while (auto len = (next(&chunk, &clen), in.read(chunk, clen))) {
        commit(len);
    }
}

void ZeroCopyOutput::recvFromZ(ZeroCopyInput& in) {
    if (in.hint() > hint()) {
        const void* chunk;

        while (auto len = in.next(&chunk)) {
            in.commit(writeP(chunk, len));
        }
    } else {
        recvFromI(in);
    }
}

size_t ZeroCopyOutput::writeImpl(const void* data, size_t len) {
    void* chunk;

    len = min(len, next(&chunk));

    memCpy(chunk, data, len);
    commit(len);

    return len;
}

size_t ZeroCopyOutput::next(void** chunk) {
    size_t res = 1;

    *chunk = imbue(&res);

    return res;
}

// std types
#define DEF_OUT(typ)                                                                       \
    template <>                                                                            \
    void stl::output<ZeroCopyOutput, typ>(ZeroCopyOutput & out, typ v) {                   \
        out << I64{v};                                                                     \
    }                                                                                      \
    template <>                                                                            \
    void stl::output<ZeroCopyOutput, unsigned typ>(ZeroCopyOutput & out, unsigned typ v) { \
        out << U64{v};                                                                     \
    }

DEF_OUT(int)
DEF_OUT(long)
DEF_OUT(short)
DEF_OUT(long long)

#define DEF_OUT_FLOAT(typ)                                               \
    template <>                                                          \
    void stl::output<ZeroCopyOutput, typ>(ZeroCopyOutput & out, typ v) { \
        auto buf = out.imbue(128);                                       \
        out.commit(buf.distance(buf << (long double)v));                 \
    }

DEF_OUT_FLOAT(float)
DEF_OUT_FLOAT(double)
DEF_OUT_FLOAT(long double)

template <>
void stl::output<ZeroCopyOutput, U64>(ZeroCopyOutput& out, U64 v) {
    auto buf = out.imbue(24);

    out.commit(buf.distance(buf << v.val));
}

template <>
void stl::output<ZeroCopyOutput, I64>(ZeroCopyOutput& out, I64 v) {
    auto buf = out.imbue(24);

    out.commit(buf.distance(buf << v.val));
}

template <>
void stl::output<ZeroCopyOutput, const char*>(ZeroCopyOutput& out, const char* v) {
    out << StringView(v);
}
./std/ios/outable.cpp
#include "outable.h"
./std/ios/out_fd.cpp
#include "out_fd.h"

#include <std/sys/fd.h>

using namespace stl;

FDOutput::~FDOutput() noexcept {
}

size_t FDOutput::writeImpl(const void* data, size_t len) {
    return fd->write(data, len);
}

void FDOutput::finishImpl() {
    fd = nullptr;
}

size_t FDOutput::writeVImpl(iovec* parts, size_t count) {
    return fd->writeV(parts, count);
}

void FDRegular::flushImpl() {
    fd->fsync();
}

size_t FDRegular::hintImpl() const noexcept {
    return 1 << 14;
}

FDRegular::FDRegular(FD& fd) noexcept
    : FDOutput(fd)
{
}

size_t FDCharacter::hintImpl() const noexcept {
    return 1 << 10;
}

FDCharacter::FDCharacter(FD& fd) noexcept
    : FDOutput(fd)
{
}

size_t FDPipe::hintImpl() const noexcept {
    return 1 << 12;
}

FDPipe::FDPipe(FD& fd) noexcept
    : FDOutput(fd)
{
}
./std/ios/in_zc.cpp
#include "in_zc.h"
#include "output.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/sys/types.h>
#include <std/lib/buffer.h>
#include <std/alg/minmax.h>

using namespace stl;

ZeroCopyInput::~ZeroCopyInput() noexcept {
}

size_t ZeroCopyInput::readImpl(void* data, size_t len) {
    const void* chunk;

    len = min(next(&chunk), len);

    memCpy(data, chunk, len);
    commit(len);

    return len;
}

void ZeroCopyInput::sendTo(Output& out) {
    out.recvFromZ(*this);
}

bool ZeroCopyInput::readLine(Buffer& buf) {
    return readTo(buf, u8'\n');
}

bool ZeroCopyInput::readTo(Buffer& buf, u8 delim) {
    const void* chunk;

    size_t len = next(&chunk);

    if (!len) {
        return false;
    }

    do {
        StringView part((const u8*)chunk, len);

        if (auto pos = part.memChr(delim); pos) {
            StringView line(part.begin(), pos);

            buf.append(line.begin(), line.length());
            commit(line.length() + 1);

            return true;
        } else {
            buf.append(part.begin(), part.length());
            commit(part.length());
        }
    } while (len = next(&chunk));

    return true;
}
./std/ios/in_fd.cpp
#include "in_fd.h"

#include <std/sys/fd.h>

using namespace stl;

size_t FDInput::readImpl(void* data, size_t len) {
    return fd->read(data, len);
}

FDInput::~FDInput() noexcept {
}

size_t FDInput::hintImpl() const noexcept {
    return 1 << 14;
}
./std/ios/fs_utils.cpp
#include "fs_utils.h"

#include "in_fd.h"

#include <std/sys/fd.h>
#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/lib/buffer.h>
#include <std/str/builder.h>

#include <fcntl.h>
#include <unistd.h>

using namespace stl;

void stl::readFileContent(Buffer& path, Buffer& out) {
    int rawFd = ::open(path.cStr(), O_RDONLY);

    if (rawFd < 0) {
        Errno().raise(StringBuilder() << StringView(u8"can not open ") << path);
    }

    ScopedFD fd(rawFd);
    FDInput(fd).readAll(out);
}
./std/ios/unbound.cpp
#include "unbound.h"

#include <std/sys/crt.h>
#include <std/str/fmt.h>
#include <std/str/view.h>

using namespace stl;

template <>
void stl::output<UnboundBuffer, u64>(UnboundBuffer& buf, u64 v) {
    buf.ptr = formatU64Base10(v, buf.ptr);
}

template <>
void stl::output<UnboundBuffer, i64>(UnboundBuffer& buf, i64 v) {
    buf.ptr = formatI64Base10(v, buf.ptr);
}

template <>
void stl::output<UnboundBuffer, long double>(UnboundBuffer& buf, long double v) {
    buf.ptr = formatLongDouble(v, buf.ptr);
}

template <>
void stl::output<UnboundBuffer, StringView>(UnboundBuffer& buf, StringView v) {
    buf.ptr = memCpy(buf.ptr, v.data(), v.length());
}
./std/ios/sys.cpp
#include "sys.h"
#include "out_fd.h"
#include "output.h"

#include <std/sys/fd.h>

#include <sys/stat.h>

using namespace stl;

namespace {
    template <typename T>
    struct Scoped: public FD, public T {
        Scoped(int fd) noexcept
            : FD(fd)
            , T(*(FD*)this)
        {
        }
    };

    static FDOutput* wrap(int fd) {
        struct stat st;

        if (fstat(fd, &st) == 0) {
            if (S_ISREG(st.st_mode) || S_ISBLK(st.st_mode)) {
                return new Scoped<FDRegular>(fd);
            }

            if (S_ISCHR(st.st_mode)) {
                return new Scoped<FDCharacter>(fd);
            }
        }

        return new Scoped<FDPipe>(fd);
    }
}

Output& stl::stdoutStream() noexcept {
    static auto fd = wrap(1);

    return *fd;
}

Output& stl::stderrStream() noexcept {
    static auto fd = wrap(2);

    return *fd;
}
./std/ios/out_mem.cpp
#include "out_mem.h"

#include <std/alg/advance.h>

using namespace stl;

void* MemoryOutput::imbueImpl(size_t* len) {
    return (*len = (size_t)-1, ptr);
}

void MemoryOutput::commitImpl(size_t len) noexcept {
    ptr = advancePtr(ptr, len);
}
./std/thr/mutex.cpp
#include "mutex.h"
#include "coro.h"
#include "mutex_iface.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

#include <pthread.h>

using namespace stl;

struct PosixMutexImpl: public MutexIface, public pthread_mutex_t {
    PosixMutexImpl() {
        if (pthread_mutex_init(this, nullptr) != 0) {
            Errno().raise(StringBuilder() << StringView(u8"pthread_mutex_init failed"));
        }
    }

    ~PosixMutexImpl() noexcept override {
        STD_INSIST(pthread_mutex_destroy(this) == 0);
    }

    void lock() noexcept override {
        STD_INSIST(pthread_mutex_lock(this) == 0);
    }

    void unlock() noexcept override {
        STD_INSIST(pthread_mutex_unlock(this) == 0);
    }

    bool tryLock() noexcept override {
        return pthread_mutex_trylock(this) == 0;
    }

    void* nativeHandle() noexcept override {
        return static_cast<pthread_mutex_t*>(this);
    }
};

Mutex::Mutex()
    : Mutex(new PosixMutexImpl())
{
}

Mutex::Mutex(CoroExecutor* exec)
    : Mutex(exec->createMutex())
{
}

Mutex::Mutex(MutexIface* iface)
    : impl(iface)
{
}

Mutex::Mutex(bool locked)
    : Mutex()
{
    if (locked) {
        lock();
    }
}

Mutex::~Mutex() noexcept {
    delete impl;
}

void Mutex::lock() noexcept {
    impl->lock();
}

void Mutex::unlock() noexcept {
    impl->unlock();
}

bool Mutex::tryLock() noexcept {
    return impl->tryLock();
}
./std/thr/barrier.cpp
#include "barrier.h"
#include "mutex.h"
#include "cond_var.h"
#include "coro.h"

#include <std/sys/types.h>

using namespace stl;

struct Barrier::Impl {
    Mutex mutex;
    CondVar cv;
    size_t remaining;

    Impl(int n)
        : remaining(n)
    {
    }

    Impl(int n, CoroExecutor* exec)
        : mutex(exec)
        , cv(exec)
        , remaining(n)
    {
    }

    void wait() noexcept {
        LockGuard lock(mutex);

        if (--remaining == 0) {
            cv.broadcast();
        } else {
            while (remaining > 0) {
                cv.wait(mutex);
            }
        }
    }
};

Barrier::Barrier(int n)
    : impl(new Impl(n))
{
}

Barrier::Barrier(int n, CoroExecutor* exec)
    : impl(new Impl(n, exec))
{
}

Barrier::~Barrier() noexcept {
    delete impl;
}

void Barrier::wait() noexcept {
    impl->wait();
}
./std/thr/cond_var.cpp
#include "mutex.h"
#include "cond_var.h"
#include "coro.h"
#include "mutex_iface.h"
#include "cond_var_iface.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

#include <pthread.h>

using namespace stl;

struct PosixCondVarImpl: public CondVarIface, public pthread_cond_t {
    PosixCondVarImpl() {
        if (pthread_cond_init(this, nullptr) != 0) {
            Errno().raise(StringBuilder() << StringView(u8"pthread_cond_init failed"));
        }
    }

    ~PosixCondVarImpl() noexcept override {
        STD_INSIST(pthread_cond_destroy(this) == 0);
    }

    void wait(MutexIface* mutex) noexcept override {
        STD_INSIST(pthread_cond_wait(this, (pthread_mutex_t*)mutex->nativeHandle()) == 0);
    }

    void signal() noexcept override {
        STD_INSIST(pthread_cond_signal(this) == 0);
    }

    void broadcast() noexcept override {
        STD_INSIST(pthread_cond_broadcast(this) == 0);
    }
};

CondVar::CondVar()
    : CondVar(new PosixCondVarImpl())
{
}

CondVar::CondVar(CondVarIface* iface)
    : impl(iface)
{
}

CondVar::CondVar(CoroExecutor* exec)
    : CondVar(exec->createCondVar())
{
}

CondVar::~CondVar() noexcept {
    delete impl;
}

void CondVar::wait(Mutex& mutex) noexcept {
    impl->wait(mutex.impl);
}

void CondVar::signal() noexcept {
    impl->signal();
}

void CondVar::broadcast() noexcept {
    impl->broadcast();
}
./std/thr/wait_queue.cpp
#include "wait_queue.h"
#include "mutex.h"

#include <std/sys/types.h>
#include <std/dbg/assert.h>
#include <std/sys/atomic.h>

using namespace stl;

namespace {
    template <typename T>
    struct BitmaskImpl: public WaitQueue {
        static constexpr u8 N = sizeof(T) * 8;

        T bits_ = 0;
        Item* items_[N] = {};

        void enqueue(Item* item) noexcept override {
            u8 idx = item->index;
            STD_ASSERT(idx < N);

            items_[idx] = item;
            __atomic_fetch_or(&bits_, T(1) << idx, __ATOMIC_RELEASE);
        }

        Item* dequeue() noexcept override {
            T old = stdAtomicFetch(&bits_, MemoryOrder::Acquire);

            for (;;) {
                if (!old) {
                    return nullptr;
                }

                int idx;

                if constexpr (sizeof(T) == 8) {
                    idx = __builtin_ctzll(old);
                } else {
                    idx = __builtin_ctz(old);
                }

                T desired = old & ~(T(1) << idx);

                if (stdAtomicCAS(&bits_, &old, desired, MemoryOrder::Acquire, MemoryOrder::Acquire)) {
                    return items_[idx];
                }
            }
        }
    };

#if __SIZEOF_POINTER__ == 8
    // No ABA here: each Item is a Worker that re-enqueues itself only after being
    // dequeued and completing a full condvar sleep/wake cycle. The tag would need
    // to wrap 65536 times while a competing CAS spins — impossible in practice.
    struct PointerImpl: public WaitQueue {
        // upper 16 bits: tag, lower 48 bits: pointer
        u64 head_ = 0;

        static u64 pack(Item* ptr, u64 tag) noexcept {
            return ((tag & 0xFFFFu) << 48) | ((uintptr_t)(ptr) & 0x0000FFFFFFFFFFFFu);
        }

        static Item* unpackPtr(u64 val) noexcept {
            return (Item*)(val & 0x0000FFFFFFFFFFFFu);
        }

        static u64 unpackTag(u64 val) noexcept {
            return val >> 48;
        }

        void enqueue(Item* item) noexcept override {
            u64 old = stdAtomicFetch(&head_, MemoryOrder::Relaxed);
            u64 desired;

            do {
                item->next = unpackPtr(old);
                desired = pack(item, unpackTag(old) + 1);
            } while (!stdAtomicCAS(&head_, &old, desired, MemoryOrder::Release, MemoryOrder::Relaxed));
        }

        Item* dequeue() noexcept override {
            u64 old = stdAtomicFetch(&head_, MemoryOrder::Acquire);

            for (;;) {
                Item* ptr = unpackPtr(old);

                if (!ptr) {
                    return nullptr;
                }

                u64 desired = pack(ptr->next, unpackTag(old));

                if (stdAtomicCAS(&head_, &old, desired, MemoryOrder::Acquire, MemoryOrder::Acquire)) {
                    return ptr;
                }
            }
        }
    };
#endif

    struct MutexImpl: public WaitQueue {
        Mutex mutex;
        Item* head = nullptr;
        int empty = true;

        void enqueue(Item* item) noexcept override {
            LockGuard lock(mutex);

            item->next = head;
            head = item;

            stdAtomicStore(&empty, false, MemoryOrder::Release);
        }

        Item* dequeue() noexcept override {
            if (stdAtomicFetch(&empty, MemoryOrder::Acquire)) {
                return nullptr;
            }

            LockGuard lock(mutex);

            Item* item = head;

            if (!item) {
                return nullptr;
            }

            head = item->next;

            stdAtomicStore(&empty, head == nullptr, MemoryOrder::Release);

            return item;
        }
    };
}

WaitQueue::~WaitQueue() noexcept {
}

WaitQueue::Ref WaitQueue::construct(size_t maxWaiters) {
#if __SIZEOF_POINTER__ == 8
    if (maxWaiters <= 64) {
        return new BitmaskImpl<u64>();
    }

    return new PointerImpl();
#else
    if (maxWaiters <= 32) {
        return new BitmaskImpl<u32>();
    }

    return new MutexImpl();
#endif
}
./std/thr/coro.cpp
#include "coro.h"
#include "task.h"
#include "pool.h"
#include "mutex.h"
#include "mutex_iface.h"
#include "cond_var_iface.h"

#include <std/sys/crt.h>
#include <std/ptr/scoped.h>
#include <std/alg/destruct.h>
#include <std/lib/list.h>

#include <ucontext.h>

using namespace stl;

namespace {
    struct CoroExecutorImpl;

    struct ContImpl: public Cont, public Task {
        CoroExecutorImpl* exec_;
        ucontext_t ctx_;
        ucontext_t* workerCtx_;
        Runable* runable_;
        Runable* afterSuspend_;

        ContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept;

        virtual ~ContImpl() = default;

        CoroExecutor* executor() noexcept override;
        void reSchedule() noexcept;
        void run() noexcept override;
        void entryX() noexcept;

        static void entry(u32 lo, u32 hi) noexcept;
    };

    struct HeapContImpl: public ContImpl {
        using ContImpl::ContImpl;

        ~HeapContImpl() override {
            freeMemory(ctx_.uc_stack.ss_sp);
        }
    };

    ContImpl* makeContImpl(CoroExecutorImpl* exec, SpawnParams params) {
        if (params.stackPtr) {
            return new ContImpl(exec, params);
        } else {
            return new HeapContImpl(exec, params.setStackPtr(allocateMemory(params.stackSize)));
        }
    }

    struct CoroMutexImpl;
    struct CoroCondVarImpl;

    struct CoroExecutorImpl: public CoroExecutor {
        ThreadPool::Ref pool_;
        const u64 tlsKey_;

        explicit CoroExecutorImpl(ThreadPool::Ref pool) noexcept
            : pool_(pool)
            , tlsKey_(registerTlsKey())
        {
        }

        auto tls() {
            return pool_->tls(tlsKey_);
        }

        ContImpl* currentCont() {
            return (ContImpl*)*tls();
        }

        void spawnRun(SpawnParams params) override {
            pool_->submitTask(makeContImpl(this, params));
        }

        Cont* me() const noexcept override {
            return ((CoroExecutorImpl*)this)->currentCont();
        }

        void yield() noexcept override {
            auto* c = currentCont();

            swapcontext(&c->ctx_, c->workerCtx_);
        }

        ThreadPool* pool() const noexcept override {
            return (ThreadPool*)pool_.ptr();
        }

        MutexIface* createMutex() override;
        CondVarIface* createCondVar() override;
    };

    struct CoroCondVarImpl: public CondVarIface, public Runable {
        CoroExecutorImpl* exec_;
        Mutex queueMutex_;
        IntrusiveList waiters_;

        CoroCondVarImpl(CoroExecutorImpl* exec) noexcept;

        void run() override;
        void wait(MutexIface* mutex) noexcept override;
        void signal() noexcept override;
        void broadcast() noexcept override;
    };

    struct CoroMutexImpl: public MutexIface, public Runable {
        CoroExecutorImpl* exec_;
        Mutex queueMutex_;
        IntrusiveList waiters_;
        bool locked_;

        CoroMutexImpl(CoroExecutorImpl* exec) noexcept;

        void run() override;
        void lock() noexcept override;
        void unlock() noexcept override;
        bool tryLock() noexcept override;
    };
}

CoroMutexImpl::CoroMutexImpl(CoroExecutorImpl* exec) noexcept
    : exec_(exec)
    , locked_(false)
{
}

void CoroMutexImpl::run() {
    queueMutex_.unlock();
}

void CoroMutexImpl::lock() noexcept {
    auto* cont = exec_->currentCont();

    queueMutex_.lock();

    if (!locked_) {
        locked_ = true;
        queueMutex_.unlock();

        return;
    }

    waiters_.pushBack(cont);
    cont->afterSuspend_ = this;
    swapcontext(&cont->ctx_, cont->workerCtx_);
    // resumed here — worker already ran afterSuspend_
}

void CoroMutexImpl::unlock() noexcept {
    LockGuard guard(queueMutex_);

    if (auto* node = waiters_.popFrontOrNull(); node) {
        auto* cont = (ContImpl*)(Task*)node;

        cont->afterSuspend_ = nullptr;
        cont->reSchedule();
    } else {
        locked_ = false;
    }
}

bool CoroMutexImpl::tryLock() noexcept {
    LockGuard guard(queueMutex_);

    if (!locked_) {
        return locked_ = true;
    }

    return false;
}

ContImpl::ContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept
    : exec_(exec)
    , workerCtx_(nullptr)
    , runable_(params.runable)
    , afterSuspend_(nullptr)
{
    getcontext(&ctx_);

    ctx_.uc_stack.ss_sp = params.stackPtr;
    ctx_.uc_stack.ss_size = params.stackSize;
    ctx_.uc_link = nullptr;

    auto p = (uintptr_t)this;

    makecontext(&ctx_, (void (*)())ContImpl::entry, 2, (u32)p, (u32)(p >> 32));
}

SpawnParams::SpawnParams() noexcept
    : stackSize(16 * 1024)
    , stackPtr(nullptr)
    , runable(nullptr)
{
}

CoroExecutor* ContImpl::executor() noexcept {
    return exec_;
}

void ContImpl::entryX() noexcept {
    runable_->run();
    runable_ = nullptr;
    swapcontext(&ctx_, workerCtx_);
}

void ContImpl::entry(u32 lo, u32 hi) noexcept {
    ((ContImpl*)(((uintptr_t)hi << 32) | lo))->entryX();
}

void ContImpl::reSchedule() noexcept {
    exec_->pool_->submitTask(this);
}

void ContImpl::run() noexcept {
    ucontext_t workerCtx;

    *exec_->tls() = this;
    workerCtx_ = &workerCtx;
    swapcontext(&workerCtx, &ctx_);
    *exec_->tls() = nullptr;

    if (auto* as = afterSuspend_) {
        // after run(), cont may already be rescheduled by another thread
        return as->run();
    }

    if (runable_) {
        reSchedule();
    } else {
        delete this;
    }
}

MutexIface* CoroExecutorImpl::createMutex() {
    return new CoroMutexImpl(this);
}

CoroCondVarImpl::CoroCondVarImpl(CoroExecutorImpl* exec) noexcept
    : exec_(exec)
{
}

void CoroCondVarImpl::run() {
    queueMutex_.unlock();
}

void CoroCondVarImpl::wait(MutexIface* mutex) noexcept {
    auto* cont = exec_->currentCont();

    queueMutex_.lock();
    waiters_.pushBack(cont);
    cont->afterSuspend_ = this;
    mutex->unlock();
    swapcontext(&cont->ctx_, cont->workerCtx_);
    // resumed — queueMutex_ was unlocked by run(), mutex is not held
    mutex->lock();
}

void CoroCondVarImpl::signal() noexcept {
    LockGuard guard(queueMutex_);

    if (auto* node = waiters_.popFrontOrNull(); node) {
        auto* cont = (ContImpl*)(Task*)node;

        cont->afterSuspend_ = nullptr;
        cont->reSchedule();
    }
}

void CoroCondVarImpl::broadcast() noexcept {
    LockGuard guard(queueMutex_);

    while (auto* node = waiters_.popFrontOrNull()) {
        auto* cont = (ContImpl*)(Task*)node;

        cont->afterSuspend_ = nullptr;
        cont->reSchedule();
    }
}

CondVarIface* CoroExecutorImpl::createCondVar() {
    return new CoroCondVarImpl(this);
}

CoroExecutor::~CoroExecutor() noexcept {
}

CoroExecutor::Ref CoroExecutor::create(ThreadPool* pool) {
    return new CoroExecutorImpl(pool);
}

CoroExecutor::Ref CoroExecutor::create(size_t threads) {
    return create(ThreadPool::workStealing(threads).mutPtr());
}

SpawnParams& SpawnParams::setStackSize(size_t v) noexcept {
    stackSize = v;

    return *this;
}

SpawnParams& SpawnParams::setStackPtr(void* v) noexcept {
    stackPtr = v;

    return *this;
}

SpawnParams& SpawnParams::setRunablePtr(Runable* v) noexcept {
    runable = v;

    return *this;
}
./std/thr/latch.cpp
#include "latch.h"
#include "mutex.h"
#include "cond_var.h"
#include "coro.h"

using namespace stl;

struct Latch::Impl {
    Mutex mutex;
    CondVar cv;
    size_t remaining;

    Impl(size_t n)
        : remaining(n)
    {
    }

    Impl(size_t n, CoroExecutor* exec)
        : mutex(exec)
        , cv(exec)
        , remaining(n)
    {
    }

    void arrive() noexcept {
        LockGuard lock(mutex);

        if (--remaining == 0) {
            cv.broadcast();
        }
    }

    void wait() noexcept {
        LockGuard lock(mutex);

        while (remaining > 0) {
            cv.wait(mutex);
        }
    }
};

Latch::Latch(size_t n)
    : impl(new Impl(n))
{
}

Latch::Latch(size_t n, CoroExecutor* exec)
    : impl(new Impl(n, exec))
{
}

Latch::~Latch() noexcept {
    delete impl;
}

void Latch::arrive() noexcept {
    impl->arrive();
}

void Latch::wait() noexcept {
    impl->wait();
}
./std/thr/thread.cpp
#include "thread.h"
#include "runable.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/ptr/scoped.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

#include <pthread.h>

using namespace stl;

struct Thread::Impl {
    pthread_t thread;
    Runable* runable;

    explicit Impl(Runable& r)
        : runable(&r)
    {
        if (pthread_create(&thread, nullptr, threadFunc, this)) {
            Errno().raise(StringBuilder() << StringView(u8"pthread_create failed"));
        }
    }

    static void* threadFunc(void* arg) {
        return (((Impl*)arg)->runable->run(), nullptr);
    }
};

Thread::Thread(Runable& runable)
    : impl(new Impl(runable))
{
}

Thread::~Thread() noexcept {
    delete impl;
}

void Thread::join() noexcept {
    STD_INSIST(pthread_join(impl->thread, nullptr) == 0);
}

void Thread::detach() noexcept {
    STD_INSIST(pthread_detach(impl->thread) == 0);
}

u64 Thread::threadId() const noexcept {
    static_assert(sizeof(pthread_t) <= sizeof(u64));
    return (u64)impl->thread;
}

u64 Thread::currentThreadId() noexcept {
    return (u64)pthread_self();
}

void stl::detach(Runable& runable) {
    struct Helper: public Runable {
        Runable* slave;
        ScopedPtr<Thread> thr;

        Helper(Runable* r) noexcept
            : slave(r)
            , thr(nullptr)
        {
        }

        void start() {
            (thr.ptr = new Thread(*this))->detach();
        }

        void run() override {
            ScopedPtr<Helper> that(this);
            slave->run();
        }
    };

    ScopedPtr<Helper> guard(new Helper(&runable));

    guard.ptr->start();
    guard.drop();
}
./std/thr/mutex_iface.cpp
#include "mutex_iface.h"

using namespace stl;

MutexIface::~MutexIface() noexcept {
}

void* MutexIface::nativeHandle() noexcept {
    return nullptr;
}
./std/thr/runable.cpp
#include "runable.h"
./std/thr/cond_var_iface.cpp
#include "cond_var_iface.h"

using namespace stl;

CondVarIface::~CondVarIface() noexcept {
}
./std/thr/pool.cpp
#include "pool.h"
#include "task.h"
#include "mutex.h"
#include "thread.h"
#include "runable.h"
#include "cond_var.h"
#include "wait_queue.h"

#include <std/rng/pcg.h>
#include <std/lib/list.h>
#include <std/sym/i_map.h>
#include <std/alg/range.h>
#include <std/alg/minmax.h>
#include <std/lib/vector.h>
#include <std/dbg/insist.h>
#include <std/sys/atomic.h>
#include <std/alg/shuffle.h>
#include <std/alg/exchange.h>
#include <std/rng/split_mix_64.h>

#include <stdlib.h>

using namespace stl;

u64 stl::registerTlsKey() noexcept {
    static u64 g_tlsKeyCounter = 0;

    return stdAtomicAddAndFetch(&g_tlsKeyCounter, 1, MemoryOrder::Relaxed);
}

namespace {
    struct ShutDown: public Task {
        ~ShutDown() noexcept {
            unlink();
        }

        void run() override {
            throw this;
        }
    };

    struct SyncThreadPool: public ThreadPool {
        IntMap<void*> tls_;

        void submitTask(Task* task) noexcept override {
            task->run();
        }

        void join() noexcept override {
        }

        void** tls(u64 key) noexcept override {
            return &tls_[key];
        }
    };

    class ThreadPoolImpl: public ThreadPool {
        struct Worker: public Runable {
            ThreadPoolImpl* pool_;
            IntMap<void*> tls_;
            Thread thread_;

            explicit Worker(ThreadPoolImpl* p) noexcept
                : pool_(p)
                , thread_(*this)
            {
            }

            auto key() const noexcept {
                return thread_.threadId();
            }

            void run() noexcept override {
                try {
                    pool_->workerLoop();
                } catch (ShutDown* sh) {
                    pool_->submitTask(sh);
                }
            }
        };

        Mutex mutex_;
        CondVar condVar_;
        IntrusiveList queue_;
        IntMap<Worker> workerIndex_;

        void workerLoop();

    public:
        ThreadPoolImpl(size_t numThreads);

        void submitTask(Task* task) noexcept override;
        void join() noexcept override;
        void** tls(u64 key) noexcept override;
    };
}

ThreadPoolImpl::ThreadPoolImpl(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        workerIndex_.insertKeyed(this);
    }
}

void ThreadPoolImpl::submitTask(Task* task) noexcept {
    LockGuard lock(mutex_);
    queue_.pushBack(task);
    condVar_.signal();
}

void ThreadPoolImpl::join() noexcept {
    ShutDown task;

    submitTask(&task);

    workerIndex_.visit([](Worker& w) {
        w.thread_.join();
    });
}

void** ThreadPoolImpl::tls(u64 key) noexcept {
    if (auto w = workerIndex_.find(Thread::currentThreadId()); w) {
        return &w->tls_[key];
    }

    return nullptr;
}

void ThreadPoolImpl::workerLoop() {
    LockGuard lock(mutex_);

    for (;; condVar_.wait(mutex_)) {
        while (auto t = (Task*)queue_.popFrontOrNull()) {
            UnlockGuard unlock(mutex_);

            t->run();
        }
    }
}

ThreadPool::~ThreadPool() noexcept {
}

ThreadPool::Ref ThreadPool::sync() {
    return new SyncThreadPool();
}

ThreadPool::Ref ThreadPool::simple(size_t threads) {
    if (threads == 0) {
        return sync();
    }

    return new ThreadPoolImpl(threads);
}

namespace {
    struct WorkStealingThreadPool: public ThreadPool {
        struct Worker: public Runable, public WaitQueue::Item {
            WorkStealingThreadPool* pool_;
            u32 myIndex_;
            PCG32 rng_;
            Vector<u32> so_;
            Mutex mutex_;
            CondVar condVar_;
            IntrusiveList tasks_;
            IntrusiveList local_;
            IntMap<void*> tls_;
            Thread thread_;

            Worker(WorkStealingThreadPool* pool, u32 myIndex, u32 numWorkers);

            auto key() const noexcept {
                return thread_.threadId();
            }

            void flush() noexcept;

            Task* popNoLock() noexcept {
                return (Task*)tasks_.popFrontOrNull();
            }

            void push(Task* task) noexcept {
                LockGuard lock(mutex_);
                flush();
                tasks_.pushBack(task);
                condVar_.signal();
            }

            void pushLocal(Task* task) noexcept {
                {
                    LockGuard lock(mutex_);

                    tasks_.pushBack(task);
                }

                pool_->notifyOne();
            }

            void pushThrLocal(Task* task) noexcept {
                // pushLocal(task);
                local_.pushBack(task);
            }

            void notify() noexcept {
                LockGuard lock(mutex_);
                condVar_.signal();
            }

            void join() noexcept {
                thread_.join();
            }

            void sleep() noexcept {
                pool_->wq->enqueue(this);
                condVar_.wait(mutex_);
            }

            void loop();
            void run() noexcept override;
            void split(IntrusiveList* stolen) noexcept;
        };

        Vector<Worker*> workers_;
        IntMap<Worker> workerIndex_;
        WaitQueue::Ref wq;
        size_t running_;

        WorkStealingThreadPool(size_t numThreads);

        bool notifyOne() noexcept;
        void join() noexcept override;
        Worker* localWorker() noexcept;
        Worker* nextSleeping() noexcept;
        void** tls(u64 key) noexcept override;
        void submitTask(Task* task) noexcept override;
        void trySteal(const u32* order, u32 n, u32 offset, IntrusiveList* stolen) noexcept;
    };
}

WorkStealingThreadPool::WorkStealingThreadPool(size_t numThreads)
    : workers_(numThreads)
    , wq(WaitQueue::construct(numThreads))
    , running_(numThreads)
{
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.pushBack(workerIndex_.insertKeyed(this, i, numThreads));
    }

    for (auto w : mutRange(workers_)) {
        w->mutex_.unlock();
    }
}

bool WorkStealingThreadPool::notifyOne() noexcept {
    if (auto item = (Worker*)wq->dequeue()) {
        item->notify();

        return true;
    }

    return false;
}

WorkStealingThreadPool::Worker* WorkStealingThreadPool::localWorker() noexcept {
    static thread_local Worker* curw = nullptr;

    if (curw) {
        return curw;
    } else if (auto w = workerIndex_.find(Thread::currentThreadId()); w) {
        return curw = w;
    }

    return nullptr;
}

void WorkStealingThreadPool::submitTask(Task* task) noexcept {
    if (auto w = localWorker(); w) {
        return w->pushThrLocal(task);
    } else if (auto w = (Worker*)wq->dequeue()) {
        return w->push(task);
    } else {
        return workers_[PCG32(&task).uniformUnbiased(workers_.length())]->pushLocal(task);
    }
}

void** WorkStealingThreadPool::tls(u64 key) noexcept {
    if (auto w = localWorker(); w) {
        return &w->tls_[key];
    }

    return nullptr;
}

void WorkStealingThreadPool::join() noexcept {
    ShutDown task;

    submitTask(&task);

    for (auto w : mutRange(workers_)) {
        w->join();
    }
}

void WorkStealingThreadPool::trySteal(const u32* order, u32 n, u32 offset, IntrusiveList* stolen) noexcept {
    for (u32 i = 0; stolen->empty() && i < n; ++i) {
        workers_[order[(offset + i) % n]]->split(stolen);
    }
}

WorkStealingThreadPool::Worker* WorkStealingThreadPool::nextSleeping() noexcept {
    while (stdAtomicFetch(&running_, MemoryOrder::Relaxed) > 1) {
        if (auto w = (Worker*)wq->dequeue(); w) {
            return w;
        }
    }

    return nullptr;
}

WorkStealingThreadPool::Worker::Worker(WorkStealingThreadPool* pool, u32 myIndex, u32 numWorkers)
    : WaitQueue::Item{nullptr, (u8)myIndex}
    , pool_(pool)
    , myIndex_(myIndex)
    , rng_(splitMix64(myIndex))
    , so_(numWorkers - 1)
    , mutex_(true)
    , thread_(*this)
{
    for (u32 i = 0; i < numWorkers; ++i) {
        if (i != myIndex) {
            so_.pushBack(i);
        }
    }

    shuffle(rng_, so_.mutBegin(), so_.mutEnd());
}

void WorkStealingThreadPool::Worker::flush() noexcept {
    if (!local_.empty()) {
        tasks_.pushBack(local_);
        pool_->notifyOne();
    }
}

void WorkStealingThreadPool::Worker::run() noexcept {
    try {
        loop();
    } catch (ShutDown* sh) {
        if (auto w = pool_->nextSleeping(); w) {
            w->push(sh);
        }

        LockGuard lock(mutex_);

        flush();

        while (auto task = popNoLock()) {
            task->run();
        }

        stdAtomicSubAndFetch(&pool_->running_, 1, MemoryOrder::Relaxed);
    }
}

void WorkStealingThreadPool::Worker::loop() {
    LockGuard lock(mutex_);

    do {
        while (auto task = popNoLock()) {
            {
                UnlockGuard unlock(mutex_);

                task->run();
            }

            flush();
        }

        IntrusiveList stolen;

        {
            UnlockGuard unlock(mutex_);

            pool_->trySteal(so_.data(), (u32)so_.length(), rng_.uniformUnbiased(so_.length()), &stolen);
        }

        local_.pushBack(stolen);

        flush();
    } while (!tasks_.empty() || (sleep(), true));
}

void WorkStealingThreadPool::Worker::split(IntrusiveList* stolen) noexcept {
    LockGuard lock(mutex_);

    tasks_.splitHalf(tasks_, *stolen);

    if (stolen->empty()) {
        tasks_.xchgWithEmptyList(*stolen);
    }
}

ThreadPool::Ref ThreadPool::workStealing(size_t threads) {
    if (threads <= 1) {
        return simple(threads);
    }

    return new WorkStealingThreadPool(threads);
}
./std/thr/task.cpp
#include "task.h"
./std/thr/wait_group.cpp
#include "wait_group.h"
#include "mutex.h"
#include "cond_var.h"
#include "coro.h"

#include <std/sys/types.h>

using namespace stl;

struct WaitGroup::Impl {
    Mutex mutex;
    CondVar cv;
    size_t counter;

    Impl()
        : counter(0)
    {
    }

    Impl(CoroExecutor* exec)
        : mutex(exec)
        , cv(exec)
        , counter(0)
    {
    }

    void add(size_t n) noexcept {
        LockGuard lock(mutex);
        counter += n;
    }

    void done() noexcept {
        LockGuard lock(mutex);

        if (--counter == 0) {
            cv.signal();
        }
    }

    void wait() noexcept {
        LockGuard lock(mutex);

        while (counter > 0) {
            cv.wait(mutex);
        }
    }
};

WaitGroup::WaitGroup()
    : impl(new Impl())
{
}

WaitGroup::WaitGroup(CoroExecutor* exec)
    : impl(new Impl(exec))
{
}

WaitGroup::~WaitGroup() noexcept {
    delete impl;
}

void WaitGroup::add(size_t n) noexcept {
    impl->add(n);
}

void WaitGroup::done() noexcept {
    impl->done();
}

void WaitGroup::wait() noexcept {
    impl->wait();
}
./std/mem/mem_pool.cpp
#include "new.h"
#include "mem_pool.h"
#include "disposer.h"

#include <std/sys/crt.h>
#include <std/sys/types.h>
#include <std/dbg/assert.h>
#include <std/alg/exchange.h>

using namespace stl;

namespace {
    constexpr size_t initial = 128;
    constexpr size_t alignment = alignof(max_align_t);

    struct alignas(alignment) ChunkDestructor: public Disposable, public Disposer, public Newable {
    };

    struct alignas(alignment) Chunk: public Disposable, public Newable {
        ~Chunk() noexcept override {
            freeMemory(this);
        }
    };

    struct ChunkDisposer: public Chunk, public Disposer {
    };

    static_assert(sizeof(Chunk) % alignment == 0);
    static_assert(sizeof(ChunkDisposer) % alignment == 0);
    static_assert(sizeof(ChunkDestructor) % alignment == 0);

    static void* chk(void* buf) noexcept {
        STD_ASSERT((size_t)buf % alignof(max_align_t) == 0);

        return buf;
    }
}

MemoryPool::MemoryPool()
    : currentChunk((u8*)allocateMemory(initial))
    , currentChunkEnd(currentChunk + initial)
    , ds(new (allocate(sizeof(ChunkDisposer))) ChunkDisposer())
{
    STD_ASSERT((size_t)currentChunk % alignof(max_align_t) == 0);
    ds->submit((ChunkDisposer*)ds);
}

MemoryPool::MemoryPool(void* buf, size_t len) noexcept
    : currentChunk((u8*)chk(buf))
    , currentChunkEnd(currentChunk + len)
    , ds(new (allocate(sizeof(ChunkDestructor))) ChunkDestructor())
{
    STD_ASSERT(len >= initial);
    STD_ASSERT((size_t)currentChunk % alignof(max_align_t) == 0);
    ds->submit((ChunkDestructor*)ds);
}

MemoryPool::~MemoryPool() noexcept {
    ds->dispose();
}

void* MemoryPool::allocate(size_t len) {
    const size_t alignedLen = (len + alignment - 1) & ~(alignment - 1);

    if (currentChunk + alignedLen > currentChunkEnd) {
        allocateNewChunk(alignedLen + sizeof(Chunk));
    }

    return exchange(currentChunk, currentChunk + alignedLen);
}

void MemoryPool::allocateNewChunk(size_t minSize) {
    size_t nextChunkSize = initial * (((size_t)1) << ds->length());

    while (nextChunkSize < minSize) {
        nextChunkSize *= 2;
    }

    auto newChunk = new (allocateMemory(nextChunkSize)) Chunk();

    ds->submit(newChunk);

    currentChunkEnd = (u8*)newChunk + nextChunkSize;
    currentChunk = (u8*)(newChunk + 1);

    STD_ASSERT(currentChunkEnd - currentChunk >= minSize);
}
./std/mem/obj_pool.cpp
#include "obj_pool.h"
#include "mem_pool.h"
#include "disposer.h"

#include <std/sys/crt.h>
#include <std/str/view.h>

using namespace stl;

namespace {
    struct alignas(max_align_t) Base: public ObjPool {
        MemoryPool mp;
        Disposer ds;

        Base(void* buf, size_t len)
            : mp(buf, len)
        {
        }
    };

    struct Pool: public Base {
        alignas(max_align_t) u8 buf[256 - sizeof(Base)];

        Pool() noexcept
            : Base(buf, sizeof(buf))
        {
        }

        void* allocate(size_t len) override {
            return mp.allocate(len);
        }

        void submit(Disposable* d) noexcept override {
            ds.submit(d);
        }
    };

    static_assert(sizeof(Pool) == 256);
}

ObjPool::~ObjPool() noexcept {
}

ObjPool* ObjPool::fromMemoryRaw() {
    return new Pool();
}

StringView ObjPool::intern(StringView s) {
    auto len = s.length();
    auto res = (u8*)allocate(len + 1);

    *(u8*)memCpy(res, s.data(), len) = 0;

    return StringView(res, len);
}
./std/mem/disposable.cpp
#include "disposable.h"

using namespace stl;

Disposable::~Disposable() noexcept {
}
./std/mem/new.cpp
#include "new.h"
./std/mem/disposer.cpp
#include "disposer.h"

#include <std/alg/exchange.h>
#include <std/alg/destruct.h>

using namespace stl;

void Disposer::dispose() noexcept {
    while (end) {
        destruct(exchange(end, end->prev));
    }
}

unsigned Disposer::length() const noexcept {
    unsigned res = 0;

    for (auto cur = end; cur; cur = cur->prev) {
        ++res;
    }

    return res;
}
./std/mem/free_list.cpp
#include "free_list.h"
#include "mem_pool.h"

#include <std/alg/minmax.h>
#include <std/alg/exchange.h>

using namespace stl;

namespace {
    struct Node {
        Node* next;
    };

    struct alignas(max_align_t) Base: public FreeList {
        MemoryPool mp;
        size_t objSize;
        Node* freeList;

        Base(void* buf, size_t len, size_t os)
            : mp(buf, len)
            , objSize(max(os, sizeof(Node)))
            , freeList(nullptr)
        {
        }
    };

    struct Impl: public Base {
        alignas(max_align_t) u8 buf[256 - sizeof(Base)];

        Impl(size_t os) noexcept
            : Base(buf, sizeof(buf), os)
        {
        }

        void* allocate() override {
            if (freeList) {
                return exchange(freeList, freeList->next);
            }

            return mp.allocate(objSize);
        }

        void release(void* ptr) noexcept override {
            auto node = (Node*)ptr;
            node->next = freeList;
            freeList = node;
        }
    };

    static_assert(sizeof(Impl) == 256);
    static_assert(sizeof(Base) % sizeof(max_align_t) == 0);
}

FreeList::~FreeList() noexcept {
}

FreeList::Ref FreeList::fromMemory(size_t objSize) {
    return new Impl(objSize);
}
./std/mem/obj_list.cpp
#include "obj_list.h"
./std/mem/embed.cpp
#include "embed.h"
./std/lib/list.cpp
#include "list.h"

#include <std/alg/xchg.h>
#include <std/alg/exchange.h>

using namespace stl;

namespace {
    static void link(IntrusiveNode* a, IntrusiveNode* b) noexcept {
        a->next = b;
        b->prev = a;
    }

    static void xchgWithEmpty(IntrusiveNode& l, IntrusiveNode& r) noexcept {
        IntrusiveList::insertAfter(&l, &r);
        l.remove();
    }
}

void IntrusiveList::insertAfter(IntrusiveNode* pos, IntrusiveNode* node) noexcept {
    node->remove();
    link(node, pos->next);
    link(pos, node);
}

unsigned IntrusiveList::length() const noexcept {
    unsigned res = 0;

    for (auto c = front(), e = end(); c != e; c = c->next) {
        ++res;
    }

    return res;
}

void IntrusiveList::xchg(IntrusiveList& r) noexcept {
    IntrusiveNode n;

    xchgWithEmpty(r.head, n);
    xchgWithEmpty(head, r.head);
    xchgWithEmpty(n, head);
}

void IntrusiveList::xchgWithEmptyList(IntrusiveList& r) noexcept {
    xchgWithEmpty(head, r.head);
}

namespace {
    static void splitImpl(IntrusiveNode* end, IntrusiveNode* a, IntrusiveNode* b) noexcept {
        IntrusiveNode* p[] = {a, b};

        int x = 0;

        for (auto c = end->next; c != end; c = c->next, x ^= 1) {
            link(exchange(p[x], c), c);
        }

        link(p[0], a);
        link(p[1], b);

        end->reset();
    }

    static void split(IntrusiveList& s, IntrusiveList* l, IntrusiveList* r) noexcept {
        IntrusiveList a;
        IntrusiveList b;

        splitImpl(s.mutEnd(), a.mutEnd(), b.mutEnd());

        l->pushBack(a);
        r->pushBack(b);
    }

    template <typename Compare>
    static void merge(IntrusiveList& d, IntrusiveList& l, IntrusiveList& r, Compare&& cmp) noexcept {
        while (!l.empty() && !r.empty()) {
            if (cmp(r.front(), l.front())) {
                d.pushBack(r.popFront());
            } else {
                d.pushBack(l.popFront());
            }
        }

        while (!l.empty()) {
            d.pushBack(l.popFront());
        }

        while (!r.empty()) {
            d.pushBack(r.popFront());
        }
    }

    template <typename Compare>
    static void sort(IntrusiveList& d, Compare&& cmp) noexcept {
        if (d.almostEmpty()) {
            return;
        }

        IntrusiveList l;
        IntrusiveList r;

        split(d, &l, &r);

        sort(l, cmp);
        sort(r, cmp);

        merge(d, l, r, cmp);
    }
}

void IntrusiveList::splitHalf(IntrusiveList& l, IntrusiveList& r) noexcept {
    IntrusiveList tmp;
    xchgWithEmptyList(tmp);
    ::split(tmp, &l, &r);
}

void IntrusiveList::sort(Compare1 cmp) noexcept {
    ::sort(*this, cmp);
}

void IntrusiveList::sort(Compare2 cmp, void* ctx) noexcept {
    ::sort(*this, [cmp, ctx](const IntrusiveNode* l, const IntrusiveNode* r) -> bool {
        return cmp(ctx, l, r);
    });
}

IntrusiveNode* IntrusiveList::popFrontOrNull() noexcept {
    if (empty()) {
        return nullptr;
    }

    return popFront();
}

IntrusiveNode* IntrusiveList::popBackOrNull() noexcept {
    if (empty()) {
        return nullptr;
    }

    return popBack();
}

void IntrusiveList::pushBack(IntrusiveList& lst) noexcept {
    if (lst.empty()) {
        // nothing to do
    } else if (empty()) {
        lst.xchgWithEmptyList(*this);
    } else {
        link(mutBack(), lst.mutFront());
        link(lst.mutBack(), mutEnd());
        lst.head.reset();
    }
}
./std/lib/vector.cpp
#include "vector.h"

using namespace stl;

static_assert(sizeof(Vector<void*>) == sizeof(void*));
./std/lib/buffer.cpp
#include "buffer.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/alg/bits.h>
#include <std/alg/xchg.h>
#include <std/alg/minmax.h>
#include <std/dbg/assert.h>
#include <std/ios/out_zc.h>

using namespace stl;

static_assert(sizeof(Buffer) == sizeof(void*));

namespace {
    alignas(max_align_t) static const char EMPTY[sizeof(Buffer::Header)] = {};

    static auto nullHeader() noexcept {
        return (Buffer::Header*)EMPTY;
    }

    static auto allocHeader(size_t len) {
        if (len) {
            auto flen = max<size_t>(clp2(len + sizeof(Buffer::Header)), 256);
            auto blen = flen - sizeof(Buffer::Header);

            return new (allocateMemory(flen)) Buffer::Header({
                .used = 0,
                .size = blen,
            });
        }

        return nullHeader();
    }

    static void freeHeader(Buffer::Header* ptr) noexcept {
        if (ptr == nullHeader()) {
        } else {
            freeMemory(ptr);
        }
    }
}

Buffer::~Buffer() noexcept {
    freeHeader(header());
}

Buffer::Buffer() noexcept
    : Buffer(0)
{
}

Buffer::Buffer(size_t len)
    : data_(allocHeader(len) + 1)
{
}

Buffer::Buffer(const Buffer& buf)
    : Buffer(buf.data(), buf.used())
{
}

Buffer::Buffer(const void* ptr, size_t len)
    : Buffer(len)
{
    appendUnsafe(ptr, len);
}

Buffer::Buffer(StringView v)
    : Buffer(v.data(), v.length())
{
}

Buffer::Buffer(Buffer&& buf) noexcept
    : Buffer()
{
    buf.xchg(*this);
}

void Buffer::shrinkToFit() {
    Buffer(*this).xchg(*this);
}

void Buffer::seekAbsolute(size_t pos) noexcept {
    if (header()->used != pos) {
        STD_ASSERT(pos <= capacity());
        header()->used = pos;
    }
}

void Buffer::grow(size_t size) {
    if (size > capacity()) {
        Buffer buf(size);

        buf.appendUnsafe(data(), used());
        buf.xchg(*this);
    }
}

void Buffer::append(const void* ptr, size_t len) {
    growDelta(len);
    appendUnsafe(ptr, len);
}

void Buffer::appendUnsafe(const void* ptr, size_t len) {
    STD_ASSERT(len <= left());

    auto cur = (u8*)mutCurrent();

    if (len == 1) {
        *cur = *(const u8*)ptr;
        header()->used += 1;
    } else if (len) {
        memCpy(cur, ptr, len);
        header()->used += len;
    }
}

void Buffer::xchg(Buffer& buf) noexcept {
    ::stl::xchg(data_, buf.data_);
}

void* Buffer::imbueMe(size_t* len) {
    return (growDelta(*len), *len = left(), (void*)mutCurrent());
}

void Buffer::setCapacity(size_t cap) noexcept {
    if (header()->size) {
        STD_ASSERT(header()->size >= cap);
        header()->size = cap;
    }
}

char* Buffer::cStr() {
    growDelta(1);
    *(u8*)mutCurrent() = 0;
    return (char*)mutData();
}

void Buffer::zero(size_t len) {
    grow(len);
    seekAbsolute(len);
    memZero(mutData(), mutCurrent());
}

template <>
void stl::output<ZeroCopyOutput, Buffer>(ZeroCopyOutput& out, const Buffer& buf) {
    out.write(buf.data(), buf.used());
}
./std/lib/node.cpp
#include "node.h"

#include <std/alg/xchg.h>

using namespace stl;

void IntrusiveNode::xchg(IntrusiveNode& r) {
    ::stl::xchg(next, r.next);
    ::stl::xchg(prev, r.prev);
}

void IntrusiveNode::unlink() noexcept {
    prev->next = next;
    next->prev = prev;
    reset();
}

void IntrusiveNode::reset() noexcept {
    prev = this;
    next = this;
}

bool IntrusiveNode::singular() const noexcept {
    return prev == this && next == this;
}

bool IntrusiveNode::almostEmpty() const noexcept {
    return next->next == this;
}
./std/lib/visitor.cpp
#include "visitor.h"
./std/map/map.cpp
#include "map.h"
./std/map/treap_node.cpp
#include "treap.h"
#include "treap_node.h"

#include <std/alg/minmax.h>

using namespace stl;

void* TreapNode::key() const noexcept {
    return (void*)this;
}

void TreapNode::visit(VisitorFace& vis) {
    if (left) {
        left->visit(vis);
    }

    auto r = right;

    vis.visit(this);

    if (r) {
        r->visit(vis);
    }
}

unsigned TreapNode::height() const noexcept {
    const unsigned lh = left ? left->height() : 0;
    const unsigned rh = right ? right->height() : 0;

    return 1 + max(lh, rh);
}
./std/map/treap.cpp
#include "treap.h"
#include "treap_node.h"

#include <std/rng/split_mix_64.h>

using namespace stl;

namespace {
    static u64 prio(const void* el) noexcept {
        return splitMix64((size_t)el);
    }

    static TreapNode* merge(TreapNode* l, TreapNode* r) noexcept {
        if (!l) {
            return r;
        } else if (!r) {
            return l;
        } else if (prio(l) > prio(r)) {
            return (l->right = merge(l->right, r), l);
        } else {
            return (r->left = merge(l, r->left), r);
        }
    }
}

void Treap::split(TreapNode* t, void* k, TreapNode** l, TreapNode** r) noexcept {
    if (!t) {
        *l = nullptr;
        *r = nullptr;
    } else if (cmp(t->key(), k)) {
        split(t->right, k, &t->right, r);
        *l = t;
    } else {
        split(t->left, k, l, &t->left);
        *r = t;
    }
}

void Treap::insert(TreapNode* node) noexcept {
    TreapNode* l;
    TreapNode* r;

    split(root, node->key(), &l, &r);
    root = merge(merge(l, node), r);
}

TreapNode* Treap::find(void* key) const noexcept {
    auto t = root;

    while (t) {
        if (auto tkey = t->key(); cmp(key, tkey)) {
            t = t->left;
        } else if (cmp(tkey, key)) {
            t = t->right;
        } else {
            return t;
        }
    }

    return nullptr;
}

TreapNode* Treap::erase(void* key) noexcept {
    auto ptr = &root;

    while (auto current = *ptr) {
        if (auto ckey = current->key(); cmp(key, ckey)) {
            ptr = &current->left;
        } else if (cmp(ckey, key)) {
            ptr = &current->right;
        } else {
            return (*ptr = merge(current->left, current->right), current);
        }
    }

    return nullptr;
}

TreapNode* Treap::remove(TreapNode* node) noexcept {
    return erase(node->key());
}

void Treap::visitImpl(VisitorFace&& vis) {
    if (root) {
        root->visit(vis);
    }
}

size_t Treap::length() const noexcept {
    size_t res = 0;

    ((Treap*)this)->visit([&res](void*) {
        res += 1;
    });

    return res;
}

size_t Treap::height() const noexcept {
    if (root) {
        return root->height();
    }

    return 0;
}
./std/str/builder.cpp
#include "builder.h"

using namespace stl;

size_t StringBuilder::writeImpl(const void* ptr, size_t len) {
    return (append(ptr, len), len);
}

void* StringBuilder::imbueImpl(size_t* len) {
    return imbueMe(len);
}

void StringBuilder::commitImpl(size_t len) noexcept {
    seekRelative(len);
}

StringBuilder::StringBuilder() noexcept {
}

StringBuilder::StringBuilder(Buffer&& buf) noexcept {
    buf.xchg(*this);
}

StringBuilder::StringBuilder(size_t reserve)
    : Buffer(reserve)
{
}

StringBuilder::~StringBuilder() noexcept {
}
./std/str/hash.cpp
#include "hash.h"

#include <rapidhash.h>

using namespace stl;

namespace {
    static u32 xorShift(u64 v) noexcept {
        const u32 xorshifted = ((v >> 18u) ^ v) >> 27u;
        const u32 rot = v >> 59u;

        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }
}

u32 stl::shash32(const void* data, size_t len) noexcept {
    return xorShift(shash64(data, len));
}

u64 stl::shash64(const void* data, size_t len) noexcept {
    return rapidhash(data, len);
}
./std/str/view.cpp
#include "view.h"
#include "hash.h"

#include <std/sys/crt.h>
#include <std/lib/buffer.h>
#include <std/alg/minmax.h>
#include <std/ios/out_zc.h>

#define _GNU_SOURCE
#include <string.h>

using namespace stl;

static_assert(stdHasTrivialDestructor(StringView));
static_assert(sizeof(StringView) == 2 * sizeof(void*));

namespace {
    static int spaceship(const u8* l, size_t ll, const u8* r, size_t rl) noexcept {
        const auto rr = memCmp(l, r, ll < rl ? ll : rl);

        return rr ? rr : (ll < rl ? -1 : (ll == rl ? 0 : 1));
    }

    static int spaceship(StringView l, StringView r) noexcept {
        return spaceship(l.data(), l.length(), r.data(), r.length());
    }

    static const u8* fix(const u8* ptr) noexcept {
        return ptr ? ptr : u8"";
    }
}

bool stl::operator==(StringView l, StringView r) noexcept {
    return l.length() == r.length() && spaceship(l, r) == 0;
}

bool stl::operator!=(StringView l, StringView r) noexcept {
    return !(l == r);
}

bool stl::operator<(StringView l, StringView r) noexcept {
    return spaceship(l, r) < 0;
}

StringView::StringView(const char* s) noexcept
    : StringView((const u8*)s, strLen((const u8*)s))
{
}

StringView::StringView(const Buffer& b) noexcept
    : StringView((const u8*)b.data(), b.length())
{
}

u32 StringView::hash32() const noexcept {
    return shash32(data(), length());
}

u64 StringView::hash64() const noexcept {
    return shash64(data(), length());
}

StringView StringView::prefix(size_t len) const noexcept {
    return StringView(ptr_, min(len, len_));
}

StringView StringView::suffix(size_t len) const noexcept {
    return StringView(end() - min(len, len_), end());
}

bool StringView::startsWith(StringView prefix) const noexcept {
    return this->prefix(prefix.length()) == prefix;
}

bool StringView::endsWith(StringView suffix) const noexcept {
    return this->suffix(suffix.length()) == suffix;
}

const u8* StringView::search(StringView substr) const noexcept {
    return (const u8*)memmem(fix(data()), length(), fix(substr.data()), substr.length());
}

const u8* StringView::memChr(u8 ch) const noexcept {
    return (const u8*)memchr(fix(data()), ch, length());
}

u64 StringView::stou() const noexcept {
    u64 result = 0;

    for (size_t i = 0; i < length(); ++i) {
        u8 ch = (*this)[i];

        if (ch >= '0' && ch <= '9') {
            result = result * 10 + (ch - '0');
        }
    }

    return result;
}

template <>
void stl::output<ZeroCopyOutput, StringView>(ZeroCopyOutput& out, StringView str) {
    out.write(str.data(), str.length());
}
./std/str/fmt.cpp
#include "fmt.h"

#include <std/alg/advance.h>
#include <std/alg/reverse.h>

#include <stdio.h>

void* stl::formatU64Base10(u64 v, void* ptr) noexcept {
    u8* b = (u8*)ptr;
    u8* e = b;

    do {
        *e++ = u8'0' + v % 10;
        v /= 10;
    } while (v);

    reverse(b, e);

    return e;
}

void* stl::formatI64Base10(i64 v, void* ptr) noexcept {
    u8* b = (u8*)ptr;

    if (v < 0) {
        *b++ = u8'-';

        return formatU64Base10(-(u64)v, b);
    }

    return formatU64Base10(v, b);
}

void* stl::formatLongDouble(long double v, void* buf) noexcept {
    return advancePtr(buf, sprintf((char*)buf, "%Lf", v));
}
./std/tst/ctx.cpp
#include "ctx.h"
./std/tst/ut.cpp
#include "ut.h"
#include "ctx.h"

#include <std/ios/sys.h>
#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/color.h>
#include <std/dbg/panic.h>
#include <std/alg/range.h>
#include <std/map/treap.h>
#include <std/lib/vector.h>
#include <std/str/builder.h>

#include <stdio.h>
#include <stdlib.h>

using namespace stl;

namespace {
    struct Exc {
    };

    static bool execute(TestFunc* func, ExecContext& ctx) {
        auto& outb = ctx.output();

        try {
            func->execute(ctx);

            outb << Color::bright(AnsiColor::Green)
                 << StringView(u8"+ ")
                 << *func
                 << Color::reset()
                 << endL;
        } catch (const Exc&) {
            outb << Color::bright(AnsiColor::Red)
                 << StringView(u8"- ")
                 << *func
                 << Color::reset()
                 << endL;

            return false;
        } catch (Exception& exc) {
            outb << Color::bright(AnsiColor::Red)
                 << exc.description()
                 << endL
                 << StringView(u8"- ")
                 << *func
                 << Color::reset()
                 << endL;

            return false;
        }

        return true;
    }

    struct GetOpt {
        Vector<StringView> includes;
        Vector<StringView> excludes;

        GetOpt(Ctx& ctx) noexcept {
            for (int i = 1; i < ctx.argc; ++i) {
                StringView arg(ctx.argv[i]);

                if (arg.startsWith(u8"-") && arg.length() > 1) {
                    excludes.pushBack(StringView(arg.data() + 1, arg.length() - 1));
                } else {
                    includes.pushBack(arg);
                }
            }
        }

        bool matchesFilter(StringView testName) const noexcept {
            if (matchesExclude(testName)) {
                return false;
            }

            if (includes.empty()) {
                return true;
            }

            return matchesFilterStrong(testName);
        }

        bool matchesFilterStrong(StringView testName) const noexcept {
            if (matchesExclude(testName)) {
                return false;
            }

            for (auto prefix : range(includes)) {
                if (testName.startsWith(prefix)) {
                    return true;
                }
            }

            return false;
        }

        bool matchesExclude(StringView testName) const noexcept {
            for (auto prefix : range(excludes)) {
                if (testName.startsWith(prefix)) {
                    return true;
                }
            }

            return false;
        }
    };

    struct Tests: public ExecContext, public Treap {
        Ctx* ctx = 0;
        OutBuf* outbuf = 0;
        GetOpt* opt = 0;
        size_t ok = 0;
        size_t err = 0;
        size_t skip = 0;
        size_t mute = 0;

        ZeroCopyOutput& output() const override {
            return *outbuf;
        }

        bool cmp(void* l, void* r) const noexcept override {
            return compare(*(const TestFunc*)(l), *(const TestFunc*)(r));
        }

        static bool compare(const TestFunc& l, const TestFunc& r) noexcept {
            return l.suite() < r.suite() || (l.suite() == r.suite() && l.name() < r.name());
        }

        void run(Ctx& ctx_) {
            ctx = &ctx_;
            opt = new GetOpt(ctx_);
            execute(sysO);
        }

        void execute(OutBuf&& outb) {
            outbuf = &outb;

            setPanicHandler1(panicHandler1);
            setPanicHandler2(panicHandler2);

            StringBuilder sb;

            visit([&](void* el) {
                auto test = (TestFunc*)el;

                sb.reset();
                sb << *test;

                if (test->name().startsWith(u8"_") && !opt->matchesFilterStrong(sb)) {
                    ++mute;
                } else if (!opt->matchesFilter(sb)) {
                    ++skip;
                } else if (::execute(test, *this)) {
                    ++ok;
                } else {
                    ++err;
                }

                // outb.flush();
            });

            outb << Color::bright(AnsiColor::Green)
                 << StringView(u8"OK: ")
                 << ok
                 << Color::reset();

            if (err) {
                outb << StringView(u8", ")
                     << Color::bright(AnsiColor::Red)
                     << StringView(u8"ERR: ")
                     << err
                     << Color::reset();
            }

            if (skip) {
                outb << StringView(u8", ")
                     << Color::bright(AnsiColor::Yellow)
                     << StringView(u8"SKIP: ")
                     << skip
                     << Color::reset();
            }

            if (mute) {
                outb << StringView(u8", ")
                     << Color::bright(AnsiColor::Blue)
                     << StringView(u8"MUTE: ")
                     << mute
                     << Color::reset();
            }

            outb << endL << flsH << finI;

            exit(err);
        }

        void handlePanic1() {
            outbuf->flush();
        }

        void handlePanic2() {
            ctx->printTB();
            fflush(stdout);
            fflush(stderr);
            throw Exc();
        }

        static auto& instance() noexcept {
            static auto res = new Tests();

            return *res;
        }

        static void panicHandler1() {
            instance().handlePanic1();
        }

        static void panicHandler2() {
            instance().handlePanic2();
        }
    };
}

template <>
void stl::output<ZeroCopyOutput, TestFunc>(ZeroCopyOutput& buf, const TestFunc& test) {
    buf << test.suite()
        << StringView(u8"::")
        << test.name();
}

void Ctx::run() {
    Tests::instance().run(*this);
}

void TestFunc::registerMe() {
    Tests::instance().insert(this);
}
./std/dbg/panic.cpp
#include "panic.h"
#include "color.h"

#include <std/ios/sys.h>
#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/ios/output.h>
#include <std/alg/exchange.h>

#include <stdlib.h>

using namespace stl;

namespace {
    static void emptyFunc() {
    }

    static PanicHandler panicHandler1 = (PanicHandler)emptyFunc;
    static PanicHandler panicHandler2 = (PanicHandler)abort;
}

PanicHandler stl::setPanicHandler1(PanicHandler hndl) noexcept {
    return exchange(panicHandler1, hndl);
}

PanicHandler stl::setPanicHandler2(PanicHandler hndl) noexcept {
    return exchange(panicHandler2, hndl);
}

void stl::panic(const u8* what, u32 line, const u8* file) {
    panicHandler1();

    sysE << Color::bright(AnsiColor::Red)
         << StringView(what, strLen(what))
         << StringView(u8" failed, at ")
         << StringView(file, strLen(file))
         << StringView(u8":")
         << line
         << Color::reset()
         << endL
         << finI;

    panicHandler2();
}
./std/dbg/color.cpp
#include "color.h"

#include <std/str/view.h>
#include <std/ios/out_buf.h>

using namespace stl;

template <>
void stl::output<ZeroCopyOutput, Color>(ZeroCopyOutput& buf, Color color) {
    buf << StringView(u8"\033[");

    if (color.color == AnsiColor::Reset) {
        buf << StringView(u8"0");
    } else {
        auto add = color.brightKind ? 60 : 0;

        buf << (u64)(29 + add + (int)color.color);
    }

    buf << StringView(u8"m");
}
./std/dbg/insist.cpp
#include "insist.h"
./std/dbg/assert.cpp
#include "assert.h"
./std/ptr/scoped.cpp
#include "scoped.h"
./std/ptr/intrusive.cpp
#include "intrusive.h"
./std/ptr/refcount.cpp
#include "refcount.h"

#include <std/alg/xchg.h>

void stl::xchgPtr(void** l, void** r) {
    xchg(*l, *r);
}
./std/ptr/shared.cpp
#include "shared.h"
./std/ptr/arc.cpp
#include "arc.h"

#include <std/sys/atomic.h>

using namespace stl;

ARC::ARC() noexcept {
    stdAtomicStore(&counter_, 0, MemoryOrder::Relaxed);
}

i32 ARC::ref() noexcept {
    return stdAtomicAddAndFetch(&counter_, 1, MemoryOrder::Acquire);
}

i32 ARC::refCount() const noexcept {
    return stdAtomicFetch(&counter_, MemoryOrder::Acquire);
}

i32 ARC::unref() noexcept {
    return stdAtomicSubAndFetch(&counter_, 1, MemoryOrder::Release);
}
./std/typ/support.cpp
#include "support.h"
./std/typ/intrin.cpp
#include "intrin.h"
./tst/test.cpp
#include <std/tst/ut.h>
#include <std/tst/ctx.h>

#include <cpptrace/cpptrace.hpp>

using namespace stl;

namespace {
    struct MyCtx: public Ctx {
        MyCtx(int c, char** v) {
            argc = c;
            argv = v;
        }

        void printTB() const override {
            cpptrace::generate_trace().print();
        }
    };
}

int main(int argc, char** argv) {
    MyCtx(argc, argv).run();
}
