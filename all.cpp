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
    template <class T>
    inline T clp2(T v) noexcept {
        if constexpr (sizeof(T) == 4) {
            return v <= 1 ? T(1) : T(1) << (32 - __builtin_clz(v - 1));
        } else if constexpr (sizeof(T) == 8) {
            return v <= 1 ? T(1) : T(1) << (64 - __builtin_clzll(v - 1));
        } else {
            static_assert(false, "shit happen");
        }
    }
}
./std/alg/defer.h
#pragma once

#define STD_DEFER ::stl::ScopedGuard _ = [&] mutable -> void

namespace stl {
    template <typename F>
    struct ScopedGuard {
        ScopedGuard(F ff) noexcept
            : f(ff)
        {
        }

        ~ScopedGuard() {
            f();
        }

        F f;
    };

    template <typename P, typename F>
    static void atExit(P* pool, F f) {
        pool->template make<ScopedGuard<F>>(f);
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
./std/dbg/assert.h
#pragma once

#if defined(NDEBUG) && !defined(ENABLE_ASSERT)
    #define STD_ASSERT(X)
#else
    #include "insist.h"

    #define STD_ASSERT(X) STD_INSIST(X)
#endif
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
./std/dbg/panic.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    using PanicHandler = void (*)();

    PanicHandler setPanicHandler1(PanicHandler hndl) noexcept;
    PanicHandler setPanicHandler2(PanicHandler hndl) noexcept;

    void panic(const u8* what, u32 line, const u8* file);
}
./std/dbg/verify.h
#pragma once

#include "insist.h"

#include <std/sys/throw.h>

#define STD_VERIFY(X)                   \
    do {                                \
        if (!(X)) {                     \
            ::stl::raiseVerify(         \
                STD_CAT(u8,             \
                        STD_STR(X)),    \
                __LINE__,               \
                STD_CAT(u8, __FILE__)); \
        }                               \
    } while (false)
./std/dns/ares.h
#pragma once

namespace stl {
    class ObjPool;

    struct DnsConfig;
    struct DnsResolver;
    struct CoroExecutor;

    DnsResolver* createAresResolver(ObjPool* pool, CoroExecutor* exec, const DnsConfig& cfg);
}
./std/dns/config.h
#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>

namespace stl {
    struct DnsConfig {
        size_t family;
        size_t timeout;
        size_t tries;
        size_t udpMaxQueries;
        bool tcp;
        StringView server;

        DnsConfig() noexcept;
    };
}
./std/dns/iface.h
#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct DnsConfig;
    struct DnsResult;
    struct ThreadPool;
    struct CoroExecutor;

    struct DnsResolver {
        virtual DnsResult* resolve(ObjPool* pool, const StringView& name) = 0;

        static DnsResolver* create(ObjPool* pool, CoroExecutor* exec, ThreadPool* tp, const DnsConfig& cfg);
    };
}
./std/dns/record.h
#pragma once

#include <std/sys/types.h>

struct sockaddr;

namespace stl {
    struct DnsRecord {
        DnsRecord* next;
        int family;
        sockaddr* addr;
    };
}
./std/dns/result.h
#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>

namespace stl {
    struct DnsRecord;

    struct DnsResult {
        int error;
        DnsRecord* record;

        bool ok() const noexcept {
            return error == 0;
        }

        virtual StringView errorDescr() const noexcept = 0;
    };
}
./std/dns/system.h
#pragma once

namespace stl {
    class ObjPool;

    struct DnsConfig;
    struct ThreadPool;
    struct DnsResolver;
    struct CoroExecutor;

    DnsResolver* createSystemDnsResolver(ObjPool* pool, CoroExecutor* exec, ThreadPool* tp, const DnsConfig& cfg);
}
./std/ios/fs_utils.h
#pragma once

namespace stl {
    class Buffer;

    void readFileContent(Buffer& path, Buffer& out);
}
./std/ios/in.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class FD;
    class Input;
    class ObjPool;
    class ZeroCopyInput;

    struct CoroExecutor;

    Input* createFDInput(ObjPool* pool, FD& fd);
    Input* createCoroFDInput(ObjPool* pool, FD& fd, CoroExecutor* exec);

    ZeroCopyInput* createInBuf(ObjPool* pool, Input& in);
    ZeroCopyInput* createInBuf(ObjPool* pool, Input& in, size_t chunkSize);
    ZeroCopyInput* createMemoryInput(ObjPool* pool, const void* data, size_t len);
    ZeroCopyInput* createZeroInput(ObjPool* pool);
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
./std/ios/in_fd_coro.h
#pragma once

#include "input.h"

#include <std/sys/types.h>

namespace stl {
    class FD;

    struct CoroExecutor;

    class CoroFDInput: public Input {
        size_t readImpl(void* data, size_t len) override;
        size_t hintImpl() const noexcept override;

    public:
        FD* fd;
        CoroExecutor* exec;
        off_t offset;

        CoroFDInput(FD& fd, CoroExecutor* exec) noexcept;
        ~CoroFDInput() noexcept override;
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
./std/ios/in_zc.h
#pragma once

#include "input.h"

#include <std/sys/types.h>

namespace stl {
    class Buffer;
    class StringView;

    class ZeroCopyInput: public Input {
        void sendTo(Output& out) override;
        size_t readImpl(void* data, size_t len) override;

        virtual size_t nextImpl(const void** chunk) = 0;
        virtual void commitImpl(size_t len) noexcept = 0;

    public:
        ~ZeroCopyInput() noexcept override;

        void drain();

        bool readLine(Buffer& buf);
        bool readLineZc(StringView& out, Buffer& fallback);
        bool readTo(Buffer& buf, u8 delim);

        size_t next(const void** chunk) {
            return nextImpl(chunk);
        }

        void commit(size_t len) noexcept {
            commitImpl(len);
        }
    };
}
./std/ios/in_zero.h
#pragma once

#include "in_zc.h"

namespace stl {
    class ZeroInput: public ZeroCopyInput {
        size_t readImpl(void* data, size_t len) override;
        size_t nextImpl(const void** chunk) override;
        void commitImpl(size_t len) noexcept override;
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
            if (!len) {
                return 0;
            }

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
./std/ios/out.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class FD;
    class Input;
    class Output;
    class ObjPool;
    class ZeroCopyOutput;

    struct CoroExecutor;

    Output* createFDPipe(ObjPool* pool, FD& fd);
    Output* createFDOutput(ObjPool* pool, FD& fd);
    Output* createFDRegular(ObjPool* pool, FD& fd);
    Output* createFDCharacter(ObjPool* pool, FD& fd);
    Output* createCoroFDOutput(ObjPool* pool, FD& fd, CoroExecutor* exec);

    ZeroCopyOutput* createOutBuf(ObjPool* pool, Output& out);
    ZeroCopyOutput* createMemoryOutput(ObjPool* pool, void* ptr);
    ZeroCopyOutput* createOutBuf(ObjPool* pool, Output& out, size_t chunkSize);

    Output* createNullOutput(ObjPool* pool);
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
./std/ios/out_fd_coro.h
#pragma once

#include "output.h"

#include <std/sys/types.h>

namespace stl {
    class FD;

    struct CoroExecutor;

    class CoroFDOutput: public Output {
        size_t writeImpl(const void* data, size_t len) override;
        size_t hintImpl() const noexcept override;

    public:
        FD* fd;
        CoroExecutor* exec;
        off_t offset;

        CoroFDOutput(FD& fd, CoroExecutor* exec) noexcept;
        ~CoroFDOutput() noexcept override;

        void sync();
        void dataSync();
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
./std/ios/out_null.h
#pragma once

#include "output.h"

namespace stl {
    class NullOutput: public Output {
        size_t writeImpl(const void* data, size_t len) override;

    public:
        ~NullOutput() noexcept override;
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

        ZeroCopyOutput* upgrade() noexcept override;

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

        virtual ZeroCopyOutput* upgrade() noexcept;

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
./std/ios/stream_tcp.h
#pragma once

#include "input.h"
#include "output.h"

namespace stl {
    struct TcpSocket;

    class TcpStream: public Input, public Output {
        size_t readImpl(void* data, size_t len) override;
        size_t writeImpl(const void* data, size_t len) override;
        size_t writeVImpl(iovec* parts, size_t count) override;

    public:
        TcpSocket* sock;

        TcpStream(TcpSocket& sock) noexcept;
        ~TcpStream() noexcept override;
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

        void pushFront(IntrusiveList& lst) noexcept;

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

        void cutHalf(IntrusiveList& other) noexcept;
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
./std/lib/producer.h
#pragma once

namespace stl {
    struct ProducerIface {
        virtual ~ProducerIface() noexcept;

        virtual void* run() = 0;
        virtual void del(void*) = 0;
    };

    template <typename F>
    struct Producer: public ProducerIface {
        F fn;

        Producer(F f)
            : fn(f)
        {
        }

        void* run() override {
            return new decltype(fn())(fn());
        }

        void del(void* p) override {
            delete (decltype(fn())*)p;
        }
    };

    template <typename F>
    ProducerIface* makeProducer(F f) {
        return new Producer<F>(f);
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
./std/map/map.h
#pragma once

#include "treap.h"
#include "treap_node.h"

#include <std/typ/intrin.h>
#include <std/typ/support.h>
#include <std/alg/destruct.h>
#include <std/mem/obj_list.h>

namespace stl {
    class ObjPool;

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

        struct Node final: public TreapNode {
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
        Map(ObjPool* pool)
            : ol(pool)
        {
        }

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
        TreapNode* min() const noexcept;

        void insert(TreapNode* node) noexcept;
        void xchg(Treap& other) noexcept;

        size_t height() const noexcept;
        size_t length() const noexcept;
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
./std/mem/disposable.h
#pragma once

namespace stl {
    struct Disposable {
        Disposable* prev = 0;

        virtual ~Disposable() noexcept;
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
./std/mem/free_list.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    class FreeList {
    public:
        virtual ~FreeList() noexcept;

        virtual void* allocate() = 0;
        virtual void release(void* ptr) noexcept = 0;

        static FreeList* create(ObjPool* pool, size_t objSize);
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
./std/mem/obj_list.h
#pragma once

#include "new.h"
#include "embed.h"
#include "free_list.h"

#include <std/typ/support.h>
#include <std/alg/destruct.h>

namespace stl {
    class ObjPool;

    template <typename T>
    class ObjList {
        struct TT: public Embed<T>, public Newable {
            using Embed<T>::Embed;
        };

        static_assert(sizeof(TT) == sizeof(T));
        static_assert(alignof(TT) <= alignof(max_align_t));

        FreeList* fl;

    public:
        ObjList(ObjPool* pool)
            : fl(FreeList::create(pool, sizeof(TT)))
        {
        }

        template <typename... A>
        T* make(A&&... a) {
            return &(new (fl->allocate()) TT(forward<A>(a)...))->t;
        }

        void release(T* t) {
            fl->release(destruct((TT*)(void*)t));
        }
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
    class MemoryPool;
    class StringView;

    class ObjPool: public ARC {
        virtual void submit(Disposable* d) noexcept = 0;

        template <size_t Size, size_t Align>
        void* allocFor() {
            if constexpr (Align > alignof(max_align_t)) {
                return allocateOverAligned(Size, Align);
            } else {
                return allocate(Size);
            }
        }

        template <typename T, typename... A>
        T* makeImpl(A&&... a) {
            return new (allocFor<sizeof(T), alignof(T)>()) T(forward<A>(a)...);
        }

    public:
        using Ref = IntrusivePtr<ObjPool>;

        virtual ~ObjPool() noexcept;

        virtual void* allocate(size_t len) = 0;
        virtual MemoryPool* memoryPool() noexcept = 0;

        StringView intern(StringView s);
        void* allocateOverAligned(size_t len, size_t align);

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

        static ObjPool* create(ObjPool* pool);
        static ObjPool* fromMemoryRaw();
    };
}
./std/net/http_client.h
#pragma once

#include <std/str/view.h>

namespace stl {
    class Output;
    class ObjPool;
    class ZeroCopyInput;

    struct HttpClientResponse {
        virtual u32 status() = 0;
        virtual StringView reason() = 0;
        virtual ZeroCopyInput* body() = 0;
        virtual StringView* header(StringView name) = 0;
    };

    struct HttpClientRequest {
        virtual Output* out() = 0;
        virtual HttpClientResponse* response() = 0;

        virtual void endHeaders() = 0;
        virtual void setPath(StringView path) = 0;
        virtual void setMethod(StringView method) = 0;
        virtual void addHeader(StringView name, StringView value) = 0;

        static HttpClientRequest* create(ObjPool* pool, ZeroCopyInput* in, Output* out);
    };
}
./std/net/http_io.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class Output;
    class ObjPool;
    class ZeroCopyInput;

    ZeroCopyInput* createLimitedInput(ObjPool* pool, ZeroCopyInput* inner, size_t limit);
    ZeroCopyInput* createChunkedInput(ObjPool* pool, ZeroCopyInput* inner);

    Output* createLimitedOutput(ObjPool* pool, Output* inner, size_t limit);
    Output* createChunkedOutput(ObjPool* pool, Output* inner);
}
./std/net/http_reason.h
#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>

namespace stl {
    StringView reasonPhrase(u32 code);
}
./std/net/http_srv.h
#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>

struct sockaddr;

namespace stl {
    class Input;
    class Output;
    class ObjPool;
    struct WaitGroup;
    class ZeroCopyInput;

    struct SslCtx;
    struct CoroExecutor;

    struct HttpServerRequest {
        virtual StringView path() = 0;
        virtual StringView query() = 0;
        virtual StringView method() = 0;
        virtual ZeroCopyInput* in() = 0;
        virtual StringView* header(StringView name) = 0;
    };

    struct HttpServerResponse {
        virtual Output* out() = 0;
        virtual void endHeaders() = 0;
        virtual void setStatus(u32 code) = 0;
        virtual HttpServerRequest* request() = 0;
        virtual void addHeader(StringView name, StringView value) = 0;
    };

    struct HttpServe {
        virtual SslCtx* ssl();
        virtual void serve(HttpServerResponse& resp) = 0;
    };

    struct HttpServerCtl {
        virtual void stop() = 0;
    };

    struct HttpServeOpts {
        HttpServe* handler = nullptr;
        CoroExecutor* exec = nullptr;
        WaitGroup* wg = nullptr;
        const sockaddr* addr = nullptr;
        u32 addrLen = 0;
        u32 backlog = 128;
    };

    HttpServerCtl* serve(ObjPool* pool, HttpServeOpts opts);
}
./std/net/ssl_socket.h
#pragma once

#include <std/ios/input.h>
#include <std/ios/output.h>

namespace stl {
    class ObjPool;
    class StringView;

    struct SslSocket: public Input, public Output {
    };

    struct SslCtx {
        virtual SslSocket* create(ObjPool* pool, Input* in, Output* out) = 0;

        static SslCtx* create(ObjPool* pool, StringView cert, StringView key);
    };
}
./std/net/tcp_socket.h
#pragma once

#include <std/sys/types.h>

struct iovec;
struct sockaddr;

namespace stl {
    class ObjPool;

    struct ScopedFD;
    struct IoReactor;
    struct CoroExecutor;

    struct TcpSocket {
        int fd;
        IoReactor* io;

        TcpSocket(int fd, CoroExecutor* exec) noexcept;

        void close();
        void shutdown(int how);

        int listen(int backlog);
        int bind(const sockaddr* addr, u32 addrLen);

        static int socket(int* out, int domain, int type, int protocol);

        int setReuseAddr(bool on);
        int setNoDelay(bool on);

        static int connectInf(int* out, CoroExecutor* exec, const sockaddr* addr, u32 addrLen);
        static int connect(int* out, CoroExecutor* exec, const sockaddr* addr, u32 addrLen, u64 deadlineUs);
        static int connectTout(int* out, CoroExecutor* exec, const sockaddr* addr, u32 addrLen, u64 timeoutUs);

        int acceptInf(ScopedFD& out, sockaddr* addr, u32* addrLen);
        int accept(ScopedFD& out, sockaddr* addr, u32* addrLen, u64 deadlineUs);
        int acceptTout(ScopedFD& out, sockaddr* addr, u32* addrLen, u64 timeoutUs);

        int readInf(size_t* nRead, void* buf, size_t len);
        int read(size_t* nRead, void* buf, size_t len, u64 deadlineUs);
        int readTout(size_t* nRead, void* buf, size_t len, u64 timeoutUs);

        int writeInf(size_t* nWritten, const void* buf, size_t len);
        int write(size_t* nWritten, const void* buf, size_t len, u64 deadlineUs);
        int writeTout(size_t* nWritten, const void* buf, size_t len, u64 timeoutUs);

        int writevInf(size_t* nWritten, iovec* iov, size_t iovcnt);
        int writev(size_t* nWritten, iovec* iov, size_t iovcnt, u64 deadlineUs);

        bool peek(u8& out);

        static TcpSocket* create(ObjPool* pool, int fd, CoroExecutor* exec);
    };
}
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

    template <typename T>
    IntrusivePtr<T> makeIntrusivePtr(T* t) noexcept {
        return t;
    }
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

        // sugar
        auto operator->() noexcept {
            return ptr;
        }

        auto operator->() const noexcept {
            return ptr;
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
./std/rng/mix.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    u64 mix(const void* a) noexcept;
    u64 mix(const void* a, const void* b) noexcept;
    u64 mix(const void* a, const void* b, const void* c) noexcept;
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
        u64 nextU64() noexcept;

        u32 uniformBiased(u32 n) noexcept {
            // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
            return ((u64)nextU32() * (u64)n) >> 32;
        }

        u32 uniformUnbiased(u32 n) noexcept;
    };
};
./std/rng/split_mix_64.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    u64 splitMix64(u64 x) noexcept;
    u64 nextSplitMix64(u64* x) noexcept;
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
./std/str/fmt.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    void* formatU64Base10(u64 v, void* buf) noexcept;
    void* formatI64Base10(i64 v, void* buf) noexcept;
    void* formatU64Base16(u64 v, void* buf) noexcept;
    void* formatLongDouble(long double v, void* buf) noexcept;
}
./std/str/hash.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    u32 shash32(const void* data, size_t len) noexcept;
    u64 shash64(const void* data, size_t len) noexcept;
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

        StringView stripSpace() const noexcept;
        StringView stripCr() const noexcept;

        // split by delimiter; returns false if not found
        bool split(u8 delim, StringView& before, StringView& after) const noexcept;

        // write ASCII-lowercased copy into buffer (must be >= length() bytes)
        // returns view into buffer
        StringView lower(u8* buffer) const noexcept;
        StringView lower(Buffer& buffer) const noexcept;

        // parse
        u64 stou() const noexcept;
        u64 stoh() const noexcept;
    };

    bool operator==(StringView l, StringView r) noexcept;
    bool operator!=(StringView l, StringView r) noexcept;
    bool operator<(StringView l, StringView r) noexcept;
}
./std/sym/h_map.h
#pragma once

#include "h_table.h"

#include <std/mem/embed.h>
#include <std/typ/intrin.h>
#include <std/alg/destruct.h>
#include <std/mem/obj_list.h>

namespace stl {
    class ObjPool;

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
        HashMap(ObjPool* pool)
            : ol(pool)
        {
        }

        ~HashMap() {
            if constexpr (!stdHasTrivialDestructor(Node)) {
                st.visit([](auto node) {
                    destruct((Node*)node);
                });
            }
        }

        T* any() const noexcept {
            if (auto n = (Node*)st.any(); n) {
                return &n->t;
            }

            return nullptr;
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

            return (value->key = H::hash(value->t.key()), insertNode(value));
        }

        T& operator[](K key) {
            if (auto res = find(key); res) {
                return *res;
            }

            return *insert(key);
        }

        bool erase(K key) noexcept {
            if (auto prev = (Node*)st.erase(H::hash(key)); prev) {
                ol.release(prev);

                return true;
            }

            return false;
        }

        size_t size() const noexcept {
            return st.size();
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

        Node* any() const noexcept;
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
./std/sym/i_map.h
#pragma once

#include "h_map.h"

namespace stl {
    struct IntHasher {
        static u64 hash(u64 k) noexcept;
    };

    template <typename T>
    using IntMap = HashMap<T, u64, IntHasher>;
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

    u64 monotonicNowUs() noexcept;
}
./std/sys/event_fd.h
#pragma once

namespace stl {
    class EventFD {
        struct Impl;
        Impl* impl;

    public:
        EventFD();
        ~EventFD() noexcept;

        int fd() const noexcept;

        void signal();
        void drain();
    };
}
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

        int get() const noexcept {
            return fd;
        }

        size_t read(void* data, size_t len);
        size_t writeV(iovec* parts, size_t count);
        size_t write(const void* data, size_t len);

        void close();
        void fsync();
        void setNonBlocking();

        void xchg(FD& fd) noexcept;
    };

    struct ScopedFD: public FD {
        using FD::FD;

        ~ScopedFD();

        FD release() noexcept;
    };

    void createPipeFD(ScopedFD& in, ScopedFD& out);
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
./std/sys/mem_fd.h
#pragma once

namespace stl {
    int memFD(const char* name);
}
./std/sys/num_cpu.h
#pragma once

#include "types.h"

namespace stl {
    u32 numCpu() noexcept;
}
./std/sys/throw.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class Buffer;
    class StringView;

    enum class ExceptionKind {
        Errno,
        Verify,
    };

    struct Exception {
        virtual ~Exception() noexcept;

        virtual ExceptionKind kind() const noexcept = 0;
        virtual StringView description() = 0;

        static StringView current();
    };

    [[noreturn]]
    void raiseVerify(const u8* what, u32 line, const u8* file);

    struct Errno {
        int error;

        Errno() noexcept;
        explicit Errno(int error) noexcept;

        [[noreturn]]
        void raise(Buffer&& text);
    };
}
./std/sys/types.h
#pragma once

#include <stddef.h>
#include <inttypes.h>
#include <sys/types.h>

using i8 = int8_t;
// yep
using u8 = char8_t;

using i16 = int16_t;
using u16 = uint16_t;

using i32 = int32_t;
using u32 = uint32_t;

using i64 = int64_t;
using u64 = uint64_t;
./std/thr/async.h
#pragma once

#include "future.h"
#include "future_iface.h"

#include <std/lib/producer.h>

namespace stl {
    struct ThreadPool;
    struct CoroExecutor;

    FutureIfaceRef asyncImpl(ProducerIface* prod);
    FutureIfaceRef asyncImpl(ProducerIface* prod, ThreadPool* pool);
    FutureIfaceRef asyncImpl(ProducerIface* prod, CoroExecutor* exec);

    template <typename F>
    auto async(F fn) {
        return Future<decltype(fn())>{asyncImpl(makeProducer(fn))};
    }

    template <typename F>
    auto async(CoroExecutor* exec, F fn) {
        return Future<decltype(fn())>{asyncImpl(makeProducer(fn), exec)};
    }

    template <typename F>
    auto async(ThreadPool* pool, F fn) {
        return Future<decltype(fn())>{asyncImpl(makeProducer(fn), pool)};
    }
}
./std/thr/channel.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct CoroExecutor;

    struct Channel {
        virtual void enqueue(void* v) noexcept = 0;
        virtual void enqueue(void** from, size_t len) noexcept = 0;

        virtual bool dequeue(void** out) noexcept = 0;
        virtual size_t dequeue(void** to, size_t len) noexcept = 0;

        virtual bool tryEnqueue(void* v) noexcept = 0;
        virtual bool tryDequeue(void** out) noexcept = 0;

        virtual void close() noexcept = 0;

        static Channel* create(ObjPool* pool);
        static Channel* create(ObjPool* pool, size_t cap);
        static Channel* create(ObjPool* pool, CoroExecutor* exec);
        static Channel* create(ObjPool* pool, CoroExecutor* exec, size_t cap);
    };
}
./std/thr/cond_var.h
#pragma once

namespace stl {
    class ObjPool;

    struct Mutex;
    struct CoroExecutor;

    struct CondVar {
        virtual void signal() noexcept = 0;
        virtual void broadcast() noexcept = 0;
        virtual void wait(Mutex* mutex) noexcept = 0;

        static CondVar* create(ObjPool* pool);
        static CondVar* create(ObjPool* pool, CoroExecutor* exec);
    };
}
./std/thr/context.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    struct Runable;

    struct Context {
        virtual ~Context();

        virtual void switchTo(Context& target) noexcept = 0;

        static size_t implSize() noexcept;
        static Context* create(void* buf) noexcept;
        static Context* create(void* buf, void* stackPtr, size_t stackSize, Runable& entry) noexcept;
    };
}
./std/thr/coro.h
#pragma once

#include "runable.h"

#include <std/sys/types.h>

namespace stl {
    class ObjPool;
    class IntrusiveList;

    struct Task;
    struct Mutex;
    struct Thread;
    struct CondVar;
    struct Semaphore;
    struct IoReactor;
    struct ThreadPool;
    struct Event;
    struct CoroExecutor;

    struct SpawnParams {
        size_t stackSize;
        void* stackPtr;
        Runable* runable;

        SpawnParams() noexcept;

        SpawnParams& setStackPtr(void* v) noexcept;
        SpawnParams& setStackSize(size_t v) noexcept;
        SpawnParams& setRunablePtr(Runable* v) noexcept;
        SpawnParams& setStack(void* v, size_t len) noexcept;
        SpawnParams& setStack(ObjPool* v, size_t len) noexcept;

        template <typename F>
        SpawnParams& setRunable(F f) {
            return setRunablePtr(makeRunablePtr(f));
        }
    };

    struct CoroExecutor {
        virtual void join() noexcept = 0;
        virtual void yield() noexcept = 0;
        virtual void spawnRun(SpawnParams params) = 0;
        virtual void parkWith(Runable&&, Task**) noexcept = 0;
        virtual void offloadRun(ThreadPool* pool, Runable&& work) = 0;

        virtual IoReactor* io() noexcept = 0;
        virtual ThreadPool* pool() noexcept = 0;

        virtual Event* createEvent(void* buf) = 0;
        virtual Thread* createThread(ObjPool* pool) = 0;
        virtual CondVar* createCondVar(ObjPool* pool) = 0;
        virtual Mutex* createSemaphoreImpl(ObjPool* pool, size_t initial) = 0;

        Mutex* createMutex(ObjPool* pool);
        Semaphore* createSemaphore(ObjPool* pool, size_t initial);

        virtual void* currentCoroId() const noexcept = 0;

        template <typename F>
        void spawn(F f) {
            spawnRun(SpawnParams().setRunable(f));
        }

        template <typename F>
        void offload(ThreadPool* pool, F f) {
            offloadRun(pool, makeRunable(f));
        }

        static CoroExecutor* create(ObjPool* pool, size_t threads);
    };
}
./std/thr/event.h
#pragma once

#include "runable.h"

namespace stl {
    struct CoroExecutor;

    struct Event {
        struct alignas(64) Buf {
            char data[64];
        };

        virtual ~Event() noexcept;

        virtual void signal() noexcept = 0;
        virtual void wait(Runable& cb) noexcept = 0;

        void wait(Runable&& cb) noexcept {
            wait(cb);
        }

        static Event* create(Buf& buf);
        static Event* create(Buf& buf, CoroExecutor* exec);
    };
}
./std/thr/future.h
#pragma once

#include "future_iface.h"

#include <std/ptr/intrusive.h>

namespace stl {
    using FutureIfaceRef = IntrusivePtr<FutureIface>;

    template <typename T>
    struct Future {
        FutureIfaceRef impl;

        T& wait() noexcept {
            return *(T*)impl->wait();
        }

        T* posted() noexcept {
            return (T*)impl->posted();
        }

        T* release() noexcept {
            return (T*)impl->release();
        }

        T* consume() noexcept {
            return (wait(), release());
        }
    };
}
./std/thr/future_iface.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    struct FutureIface {
        virtual ~FutureIface() noexcept;

        virtual i32 ref() noexcept = 0;
        virtual i32 unref() noexcept = 0;
        virtual i32 refCount() const noexcept = 0;

        virtual void* wait() noexcept = 0;
        virtual void* posted() noexcept = 0;
        virtual void* release() noexcept = 0;
    };
}
./std/thr/guard.h
#pragma once

namespace stl {
    struct Mutex;

    class LockGuard {
        Mutex* mutex_;

    public:
        LockGuard(Mutex* m);
        ~LockGuard() noexcept;

        template <typename F>
        auto run(F f) {
            return f();
        }

        void drop() noexcept {
            mutex_ = nullptr;
        }
    };

    class UnlockGuard {
        Mutex* mutex_;

    public:
        UnlockGuard(Mutex* m);
        ~UnlockGuard() noexcept;

        template <typename F>
        auto run(F f) {
            return f();
        }
    };
}
./std/thr/io_classic.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct IoReactor;
    struct CoroExecutor;

    IoReactor* createPollIoReactor(ObjPool* pool, CoroExecutor* exec, size_t reactors);
}
./std/thr/io_uring.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct IoReactor;
    struct CoroExecutor;

    IoReactor* createIoUringReactor(ObjPool* pool, CoroExecutor* exec, size_t threads);
}
./std/thr/mutex.h
#pragma once

#include "semaphore.h"

namespace stl {
    struct Mutex: public Semaphore {
        void lock() noexcept {
            wait();
        }

        void unlock() noexcept {
            post();
        }

        bool tryLock() noexcept {
            return tryWait();
        }

        static Mutex* create(ObjPool* pool);
        static Mutex* create(ObjPool* pool, CoroExecutor* exec);

        static Mutex* createSpinLock(ObjPool* pool);
        static Mutex* createSpinLock(ObjPool* pool, CoroExecutor* exec);
    };
}
./std/thr/parker.h
#pragma once

#include <std/sys/event_fd.h>

namespace stl {
    class Parker {
        EventFD ev_;
        alignas(64) bool sleeping_;

        void parkEnter() noexcept;
        void parkLeave() noexcept;

    public:
        Parker();
        ~Parker() noexcept;

        int fd() const noexcept;

        void drain() noexcept;
        void unpark() noexcept;
        void signal() noexcept;

        template <typename F>
        void park(F f) {
            parkEnter();
            f();
            parkLeave();
        }
    };
}
./std/thr/poll_fd.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    namespace PollFlag {
        constexpr u32 In = 1;
        constexpr u32 Out = 4;
        constexpr u32 Err = 8;
        constexpr u32 Hup = 16;
    }

    struct PollFD {
        int fd;
        u32 flags;

        short toPollEvents() const noexcept;

        static u32 fromPollEvents(short events) noexcept;
    };
}
./std/thr/pollable.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    struct PollFD;

    struct Pollable {
        virtual u32 poll(PollFD pfd, u64 deadlineUs) = 0;
    };
}
./std/thr/poller.h
#pragma once

#include <std/sys/types.h>
#include <std/lib/visitor.h>

namespace stl {
    class ObjPool;

    struct PollFD;
    struct Pollable;

    struct PollerIface {
        // add or re-arm fd with ONESHOT semantics
        virtual void arm(PollFD pfd) = 0;
        // remove fd from poller
        virtual void disarm(int fd) = 0;
        // wait for events, always finite timeout
        virtual void waitImpl(VisitorFace& v, u32 timeoutUs) = 0;

        void waitBase(VisitorFace&& v, u64 deadlineUs);

        template <typename V>
        void wait(V v, u64 deadlineUs) {
            // clang-format off
            waitBase(makeVisitor([v](void* ptr) {
                v((PollFD*)ptr);
            }), deadlineUs);
            // clang-format on
        }

        static PollerIface* create(ObjPool* pool);
        static PollerIface* createMultishot(ObjPool* pool, PollerIface* slave);
    };

    struct WaitablePoller: public PollerIface {
        virtual int fd() = 0;

        static WaitablePoller* create(ObjPool* pool);
        static WaitablePoller* create(ObjPool* pool, Pollable* reactor);
    };
}
./std/thr/pool.h
#pragma once

#include "task.h"

#include <std/sys/types.h>

namespace stl {
    struct Mutex;
    class ObjPool;
    struct CondVar;
    class IntrusiveList;

    struct ThreadPoolHooks {
        virtual Mutex* createMutex(ObjPool* pool) = 0;
        virtual CondVar* createCondVar(ObjPool* pool, size_t index) = 0;
    };

    struct ThreadPool {
        virtual void join() noexcept = 0;
        virtual void flushLocal() noexcept = 0;
        virtual bool workerId(size_t* id) noexcept = 0;
        virtual void submitTasks(IntrusiveList& tasks) noexcept = 0;

        void submitTask(Task* task) noexcept;

        template <typename F>
        void submit(F f) {
            submitTask(makeTask(f));
        }

        static ThreadPool* sync(ObjPool* pool);
        static ThreadPool* simple(ObjPool* pool, size_t threads);
        static ThreadPool* workStealing(ObjPool* pool, size_t threads);
        static ThreadPool* workStealing(ObjPool* pool, size_t threads, ThreadPoolHooks* hooks);
    };
}
./std/thr/reactor_poll.h
#pragma once

#include "pollable.h"

namespace stl {
    class ObjPool;

    struct PollerIface;
    struct CoroExecutor;

    struct ReactorIface: public Pollable {
        virtual PollerIface* createPoller(ObjPool* pool) = 0;

        static ReactorIface* create(CoroExecutor* exec, ObjPool* opool);
    };
}
./std/thr/runable.h
#pragma once

namespace stl {
    struct Runable {
        virtual void run() = 0;
    };

    template <typename V, bool del>
    struct RunableImpl final: public Runable {
        V v;

        RunableImpl(V vv)
            : v(vv)
        {
        }

        void run() override {
            v();

            if constexpr (del) {
                delete this;
            }
        }
    };

    template <typename T>
    auto makeRunable(T t) {
        return RunableImpl<T, false>(t);
    }

    template <typename T>
    auto makeRunablePtr(T t) {
        return new RunableImpl<T, true>(t);
    }
}
./std/thr/semaphore.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct CoroExecutor;

    struct Semaphore {
        virtual void post() noexcept = 0;
        virtual void wait() noexcept = 0;
        virtual bool tryWait() noexcept = 0;

        virtual void* nativeHandle() noexcept;

        static Semaphore* create(ObjPool* pool, size_t initial);
        static Semaphore* create(ObjPool* pool, size_t initial, CoroExecutor* exec);
    };
}
./std/thr/task.h
#pragma once

#include "runable.h"

#include <std/lib/node.h>
#include <std/sys/types.h>

namespace stl {
    struct Task: public Runable, public IntrusiveNode {
    };

    template <typename V>
    struct TaskImpl final: public Task {
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
./std/thr/thread.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;
    struct Runable;
    struct CoroExecutor;

    struct Thread {
        virtual void start(Runable& runable) = 0;
        virtual void join() noexcept = 0;
        virtual u64 threadId() const noexcept = 0;

        static u64 currentThreadId() noexcept;

        static Thread* create(ObjPool* pool, Runable& runable);
        static Thread* create(ObjPool* pool, CoroExecutor* exec, Runable& runable);
    };
}
./std/thr/wait_group.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct CoroExecutor;

    struct WaitGroup {
        virtual void done() noexcept = 0;
        virtual void wait() noexcept = 0;
        virtual void add(size_t n) noexcept = 0;

        void inc() noexcept {
            add(1);
        }

        static WaitGroup* create(ObjPool* pool, size_t init);
        static WaitGroup* create(ObjPool* pool, size_t init, CoroExecutor* exec);
    };
}
./std/thr/wait_queue.h
#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct WaitQueue {
        struct Item {
            Item* next = nullptr;
            u8 index = 0;
        };

        virtual Item* dequeue() noexcept = 0;
        virtual size_t sleeping() const noexcept = 0;
        virtual void enqueue(Item* item) noexcept = 0;

        static WaitQueue* construct(ObjPool* pool, size_t maxWaiters);
    };
}
./std/thr/io_reactor.h
#pragma once

#include "pollable.h"

struct iovec;
struct sockaddr;

namespace stl {
    class ObjPool;

    struct PollerIface;
    struct CondVar;
    struct CoroExecutor;
    struct ThreadPoolHooks;

    struct IoReactor: public Pollable {
        virtual int connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) = 0;
        virtual int recv(int fd, size_t* nRead, void* buf, size_t len, u64 deadlineUs) = 0;
        virtual int accept(int fd, int* newFd, sockaddr* addr, u32* addrLen, u64 deadlineUs) = 0;
        virtual int send(int fd, size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) = 0;
        virtual int writev(int fd, size_t* nWritten, iovec* iov, size_t iovcnt, u64 deadlineUs) = 0;

        virtual int recvfrom(int fd, size_t* nRead, void* buf, size_t len, sockaddr* addr, u32* addrLen, u64 deadlineUs) = 0;
        virtual int sendto(int fd, size_t* nWritten, const void* buf, size_t len, const sockaddr* addr, u32 addrLen, u64 deadlineUs) = 0;

        // Plain stream-fd I/O (no offset, no socket-only semantics) — for
        // character devices like /dev/net/tun, pipes, and any non-seekable
        // fd. recv/send won't work on those (ENOTSOCK), pread/pwrite needs
        // an offset.
        virtual int read(int fd, size_t* nRead, void* buf, size_t len, u64 deadlineUs) = 0;
        virtual int write(int fd, size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) = 0;

        virtual int fsync(int fd) = 0;
        virtual int fdatasync(int fd) = 0;
        virtual int pread(int fd, size_t* nRead, void* buf, size_t len, off_t offset) = 0;
        virtual int pwrite(int fd, size_t* nWritten, const void* buf, size_t len, off_t offset) = 0;

        virtual PollerIface* createPoller(ObjPool* pool) = 0;

        virtual void sleep(u64 deadlineUs) = 0;

        virtual ThreadPoolHooks* hooks() = 0;

        static IoReactor* create(ObjPool* pool, CoroExecutor* exec, size_t threads);
    };
}
./std/tst/args.h
#pragma once

#include <std/sym/s_map.h>

namespace stl {
    class ObjPool;

    class TestArgs: public SymbolMap<StringView> {
    public:
        TestArgs(ObjPool* pool)
            : HashMap(pool)
        {
        }

        void parse(StringView arg);

        TestArgs(ObjPool* pool, int argc, char** argv);
    };
}
./std/tst/ctx.h
#pragma once

namespace stl {
    struct Ctx {
        int argc;
        char** argv;

        virtual void printTB() const;

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
    class TestArgs;
    class ZeroCopyOutput;

    struct ExecContext {
        virtual ZeroCopyOutput& output() const noexcept = 0;
        virtual const TestArgs& args() const noexcept = 0;
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
./std/typ/intrin.h
#pragma once

// portable defines for compiler intrinsics

#define stdIsTriviallyCopyable __is_trivially_copyable
#define stdHasTrivialDestructor __is_trivially_destructible
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
./std/alg/advance.cpp
#include "advance.h"
./std/alg/bits.cpp
#include "bits.h"
./std/alg/defer.cpp
#include "defer.h"
./std/alg/destruct.cpp
#include "destruct.h"
./std/alg/exchange.cpp
#include "exchange.h"
./std/alg/minmax.cpp
#include "minmax.h"
./std/alg/qsort.cpp
#include "qsort.h"
./std/alg/range.cpp
#include "range.h"
./std/alg/reverse.cpp
#include "reverse.h"
./std/alg/shuffle.cpp
#include "shuffle.h"
./std/alg/xchg.cpp
#include "xchg.h"
./std/dbg/assert.cpp
#include "assert.h"
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
./std/dbg/verify.cpp
#include "verify.h"
./std/dns/ares.cpp
#include "ares.h"
#include "iface.h"
#include "config.h"
#include "record.h"
#include "result.h"

#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/lib/node.h>
#include <std/thr/coro.h>
#include <std/thr/event.h>
#include <std/thr/mutex.h>
#include <std/thr/guard.h>
#include <std/alg/defer.h>
#include <std/thr/parker.h>
#include <std/thr/poller.h>
#include <std/thr/poll_fd.h>
#include <std/mem/obj_pool.h>
#include <std/thr/io_reactor.h>

#if __has_include(<ares.h>)
    #include <ares.h>
#endif

#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace stl;

#if __has_include(<ares.h>)
namespace {
    struct DnsResolverImpl;

    struct DnsRecordImpl: public DnsRecord {
        DnsRecordImpl(ObjPool* pool, struct ares_addrinfo_node* node);
    };

    struct DnsResultImpl: public DnsResult {
        DnsResultImpl(ObjPool* pool, int status, struct ares_addrinfo* ai);
        StringView errorDescr() const noexcept override;
    };

    struct DnsRequest: public IntrusiveNode {
        ObjPool* pool;
        Event* event = nullptr;
        DnsResult* result = nullptr;
        bool submitted = false;
        const char* name;

        void complete(int status, struct ares_addrinfo* ai);

        static void callback(void* arg, int status, int, struct ares_addrinfo* ai) {
            ((DnsRequest*)arg)->complete(status, ai);
        }
    };

    struct DnsResolverImpl: public DnsResolver {
        CoroExecutor* exec_;
        ares_channel channel_;
        struct ares_addrinfo_hints hints_;
        Mutex* lock_;
        bool driving_ = false;
        IntrusiveList pending_;
        IntrusiveList waiters_;
        ObjPool::Ref pollerPool_;
        PollerIface* poller_ = nullptr;
        Parker parker_;

        DnsResolverImpl(ObjPool* pool, CoroExecutor* exec, const DnsConfig& cfg);
        ~DnsResolverImpl() noexcept;

        DnsResult* resolve(ObjPool* pool, const StringView& name) override;

        void submitPending();
        void driverLoop(DnsRequest& req);
        void onSockState(ares_socket_t fd, int readable, int writable);

        static void sockStateCb(void* data, ares_socket_t fd, int readable, int writable) {
            ((DnsResolverImpl*)data)->onSockState(fd, readable, writable);
        }
    };
}

DnsRecordImpl::DnsRecordImpl(ObjPool* pool, struct ares_addrinfo_node* node) {
    family = node->ai_family;
    addr = node->ai_addr;

    if (node->ai_next) {
        next = pool->make<DnsRecordImpl>(pool, node->ai_next);
    } else {
        next = nullptr;
    }
}

StringView DnsResultImpl::errorDescr() const noexcept {
    return ares_strerror(error);
}

DnsResultImpl::DnsResultImpl(ObjPool* pool, int status, struct ares_addrinfo* ai) {
    if (ai) {
        atExit(pool, [ai] {
            ares_freeaddrinfo(ai);
        });
    }

    if (status == ARES_ENOTFOUND) {
        error = 0;
        record = nullptr;
    } else if (status == ARES_EBADNAME) {
        error = 0;
        record = nullptr;
    } else if (status == ARES_ENODATA) {
        error = 0;
        record = nullptr;
    } else if (status != ARES_SUCCESS) {
        error = status;
        record = nullptr;
    } else if (!ai) {
        error = 0;
        record = nullptr;
    } else if (!ai->nodes) {
        error = 0;
        record = nullptr;
    } else {
        error = 0;
        record = pool->make<DnsRecordImpl>(pool, ai->nodes);
    }
}

void DnsRequest::complete(int status, struct ares_addrinfo* ai) {
    result = pool->make<DnsResultImpl>(pool, status, ai);

    if (event) {
        remove();
        event->signal();
    }
}

void DnsResolverImpl::onSockState(ares_socket_t fd, int readable, int writable) {
    u32 flags = (readable ? PollFlag::In : 0) | (writable ? PollFlag::Out : 0);

    if (flags) {
        poller_->arm({(int)fd, flags});
    } else {
        poller_->disarm((int)fd);
    }
}

DnsResolverImpl::DnsResolverImpl(ObjPool* pool, CoroExecutor* exec, const DnsConfig& cfg)
    : exec_(exec)
    , lock_(Mutex::createSpinLock(pool, exec))
    , pollerPool_(ObjPool::fromMemory())
{
    poller_ = PollerIface::createMultishot(pollerPool_.mutPtr(), exec->io()->createPoller(pollerPool_.mutPtr()));
    poller_->arm({parker_.fd(), PollFlag::In});
    ares_options opts;
    memset(&opts, 0, sizeof(opts));

    opts.timeout = cfg.timeout;
    opts.tries = cfg.tries;
    opts.udp_max_queries = cfg.udpMaxQueries;
    opts.sock_state_cb = sockStateCb;
    opts.sock_state_cb_data = this;

    int optmask = ARES_OPT_TIMEOUTMS | ARES_OPT_TRIES | ARES_OPT_UDP_MAX_QUERIES | ARES_OPT_SOCK_STATE_CB;

    if (cfg.tcp) {
        opts.flags = ARES_FLAG_USEVC | ARES_FLAG_STAYOPEN;
        optmask |= ARES_OPT_FLAGS;
    }

    ares_init_options(&channel_, &opts, optmask);

    if (cfg.server.length()) {
        ares_set_servers_ports_csv(channel_, (const char*)pool->intern(cfg.server).data());
    }

    memset(&hints_, 0, sizeof(hints_));
    hints_.ai_family = cfg.family;
}

DnsResolverImpl::~DnsResolverImpl() noexcept {
    ares_destroy(channel_);
}

DnsResult* DnsResolverImpl::resolve(ObjPool* pool, const StringView& name) {
    Event::Buf evbuf;
    auto* ev = Event::create(evbuf, exec_);
    STD_DEFER {
        delete ev;
    };

    DnsRequest req;

    req.pool = pool;
    req.event = ev;
    req.name = (const char*)pool->intern(name).data();

    lock_->lock();

    if (driving_) {
        pending_.pushBack(&req);

        req.event->wait(makeRunable([this] {
            lock_->unlock();
            parker_.unpark();
        }));

        if (req.result) {
            return req.result;
        }
    } else {
        driving_ = true;
        lock_->unlock();
    }

    req.event = nullptr;
    driverLoop(req);

    return req.result;
}

void DnsResolverImpl::submitPending() {
    IntrusiveList batch;

    LockGuard(lock_).run([&] {
        batch.xchg(pending_);
    });

    while (auto r = (DnsRequest*)batch.popFrontOrNull()) {
        waiters_.pushBack(r);
        ares_getaddrinfo(channel_, r->name, nullptr, &hints_, DnsRequest::callback, r);
        r->submitted = true;
    }
}

void DnsResolverImpl::driverLoop(DnsRequest& req) {
    if (!req.submitted) {
        ares_getaddrinfo(channel_, req.name, nullptr, &hints_, DnsRequest::callback, &req);
        req.submitted = true;
    }

    while (!req.result) {
        parker_.park([&] {
            submitPending();

            struct timeval tv;

            ares_timeout(channel_, nullptr, &tv);

            auto deadlineUs = monotonicNowUs() + (u64)tv.tv_sec * 1000000 + (u64)tv.tv_usec;

            poller_->wait([this](PollFD* ev) {
                if (ev->fd == parker_.fd()) {
                    parker_.drain();
                } else {
                    ares_socket_t rfd = (ev->flags & PollFlag::In) ? ev->fd : ARES_SOCKET_BAD;
                    ares_socket_t wfd = (ev->flags & PollFlag::Out) ? ev->fd : ARES_SOCKET_BAD;

                    ares_process_fd(channel_, rfd, wfd);
                }
            }, deadlineUs);
        });

        ares_process_fd(channel_, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
    }

    lock_->lock();

    if (auto next = (DnsRequest*)waiters_.popFrontOrNull()) {
        lock_->unlock();
        next->event->signal();
    } else if (auto next = (DnsRequest*)pending_.popFrontOrNull()) {
        lock_->unlock();
        next->event->signal();
    } else {
        driving_ = false;
        lock_->unlock();
    }
}

DnsResolver* stl::createAresResolver(ObjPool* pool, CoroExecutor* exec, const DnsConfig& cfg) {
    return pool->make<DnsResolverImpl>(pool, exec, cfg);
}
#else
DnsResolver* stl::createAresResolver(ObjPool*, CoroExecutor*, const DnsConfig&) {
    return nullptr;
}
#endif
./std/dns/config.cpp
#include "config.h"

#include <sys/socket.h>

using namespace stl;

DnsConfig::DnsConfig() noexcept
    : family(AF_UNSPEC)
    , timeout(100)
    , tries(3)
    , udpMaxQueries(0)
    , tcp(false)
    , server()
{
}
./std/dns/iface.cpp
#include "iface.h"
#include "ares.h"
#include "system.h"

#include <std/thr/pool.h>
#include <std/mem/obj_pool.h>

#include <stdlib.h>

using namespace stl;

DnsResolver* DnsResolver::create(ObjPool* pool, CoroExecutor* exec, ThreadPool* tp, const DnsConfig& cfg) {
    if (!getenv("USE_SYSTEM_RESOLVER")) {
        if (auto res = createAresResolver(pool, exec, cfg); res) {
            return res;
        }
    }

    if (!tp) {
        tp = ThreadPool::simple(pool, 4);
    }

    return createSystemDnsResolver(pool, exec, tp, cfg);
}
./std/dns/record.cpp
#include "record.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/ios/out_zc.h>
#include <std/ios/outable.h>

#include <arpa/inet.h>
#include <netinet/in.h>

using namespace stl;

#if defined(__APPLE__)
template <>
void stl::output<ZeroCopyOutput, DnsRecord>(ZeroCopyOutput& out, const DnsRecord& rec) {
#else
template <>
void stl::output<ZeroCopyOutput, DnsRecord>(ZeroCopyOutput& out, DnsRecord rec) {
#endif
    size_t avail = 64;
    auto buf = (char*)out.imbue(&avail);

    if (rec.family == AF_INET) {
        inet_ntop(AF_INET, &((const sockaddr_in*)rec.addr)->sin_addr, buf, avail);
    } else {
        inet_ntop(AF_INET6, &((const sockaddr_in6*)rec.addr)->sin6_addr, buf, avail);
    }

    out.commit(strLen((const u8*)buf));
}

template <>
void stl::output<ZeroCopyOutput, DnsRecord*>(ZeroCopyOutput& out, DnsRecord* rec) {
    if (!rec) {
        out << StringView(u8"(null)");

        return;
    }

    out << *rec;

    if (rec->next) {
        out << StringView(u8", ") << rec->next;
    }
}

template <>
void stl::output<ZeroCopyOutput, const DnsRecord*>(ZeroCopyOutput& out, const DnsRecord* rec) {
    output<ZeroCopyOutput, DnsRecord*>(out, const_cast<DnsRecord*>(rec));
}
./std/dns/result.cpp
#include "result.h"
#include "record.h"

#include <std/ios/outable.h>
#include <std/ios/out_zc.h>
#include <std/str/view.h>

using namespace stl;

template <>
void stl::output<ZeroCopyOutput, DnsResult*>(ZeroCopyOutput& out, DnsResult* res) {
    if (!res) {
        out << StringView(u8"(null)");

        return;
    }

    if (!res->ok()) {
        out << StringView(u8"error: ") << res->errorDescr();

        return;
    }

    out << res->record;
}

template <>
void stl::output<ZeroCopyOutput, const DnsResult*>(ZeroCopyOutput& out, const DnsResult* res) {
    output<ZeroCopyOutput, DnsResult*>(out, const_cast<DnsResult*>(res));
}
./std/dns/system.cpp
#include "iface.h"
#include "system.h"
#include "config.h"
#include "record.h"
#include "result.h"

#include <std/sys/crt.h>
#include <std/thr/coro.h>
#include <std/thr/pool.h>
#include <std/alg/defer.h>
#include <std/mem/obj_pool.h>

#include <netdb.h>
#include <string.h>
#include <sys/socket.h>

using namespace stl;

namespace {
    struct DnsSysRecordImpl: public DnsRecord {
        DnsSysRecordImpl(ObjPool* pool, struct addrinfo* node);
    };

    struct DnsSysResultImpl: public DnsResult {
        DnsSysResultImpl(ObjPool* pool, int gaierr, struct addrinfo* ai);
        StringView errorDescr() const noexcept override;
    };

    struct DnsSysResolverImpl: public DnsResolver {
        CoroExecutor* exec_;
        ThreadPool* tp_;
        struct addrinfo hints_;

        DnsSysResolverImpl(CoroExecutor* exec, ThreadPool* tp, const DnsConfig& cfg);

        DnsResult* resolve(ObjPool* pool, const StringView& name) override;
    };
}

DnsSysRecordImpl::DnsSysRecordImpl(ObjPool* pool, struct addrinfo* node) {
    family = node->ai_family;
    addr = node->ai_addr;

    if (node->ai_next) {
        next = pool->make<DnsSysRecordImpl>(pool, node->ai_next);
    } else {
        next = nullptr;
    }
}

StringView DnsSysResultImpl::errorDescr() const noexcept {
    return gai_strerror(error);
}

DnsSysResultImpl::DnsSysResultImpl(ObjPool* pool, int gaierr, struct addrinfo* ai) {
    if (ai) {
        atExit(pool, [ai] {
            freeaddrinfo(ai);
        });
    }

    if (gaierr == EAI_NONAME) {
        error = 0;
        record = nullptr;
    } else if (gaierr != 0) {
        error = gaierr;
        record = nullptr;
    } else if (!ai) {
        error = 0;
        record = nullptr;
    } else {
        error = 0;
        record = pool->make<DnsSysRecordImpl>(pool, ai);
    }
}

DnsSysResolverImpl::DnsSysResolverImpl(CoroExecutor* exec, ThreadPool* tp, const DnsConfig& cfg)
    : exec_(exec)
    , tp_(tp)
{
    memset(&hints_, 0, sizeof(hints_));
    hints_.ai_family = cfg.family;
    hints_.ai_socktype = SOCK_STREAM;
}

DnsResult* DnsSysResolverImpl::resolve(ObjPool* pool, const StringView& name) {
    const char* host = (const char*)pool->intern(name).data();
    struct addrinfo* ai = nullptr;
    int gaierr = 0;

    exec_->offload(tp_, [&] {
        gaierr = getaddrinfo(host, nullptr, &hints_, &ai);
    });

    return pool->make<DnsSysResultImpl>(pool, gaierr, ai);
}

stl::DnsResolver* stl::createSystemDnsResolver(ObjPool* pool, CoroExecutor* exec, ThreadPool* tp, const DnsConfig& cfg) {
    return pool->make<DnsSysResolverImpl>(exec, tp, cfg);
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
./std/ios/in.cpp
#include "in.h"
#include "in_fd.h"
#include "in_buf.h"
#include "in_mem.h"
#include "in_zero.h"
#include "in_fd_coro.h"

#include <std/mem/obj_pool.h>

using namespace stl;

Input* stl::createFDInput(ObjPool* pool, FD& fd) {
    return pool->make<FDInput>(fd);
}

Input* stl::createCoroFDInput(ObjPool* pool, FD& fd, CoroExecutor* exec) {
    return pool->make<CoroFDInput>(fd, exec);
}

ZeroCopyInput* stl::createMemoryInput(ObjPool* pool, const void* data, size_t len) {
    return pool->make<MemoryInput>(data, len);
}

ZeroCopyInput* stl::createInBuf(ObjPool* pool, Input& in) {
    return pool->make<InBuf>(in);
}

ZeroCopyInput* stl::createInBuf(ObjPool* pool, Input& in, size_t chunkSize) {
    return pool->make<InBuf>(in, chunkSize);
}

ZeroCopyInput* stl::createZeroInput(ObjPool* pool) {
    return pool->make<ZeroInput>();
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
./std/ios/in_fd_coro.cpp
#include "in_fd_coro.h"

#include <std/sys/fd.h>
#include <std/str/view.h>
#include <std/thr/coro.h>
#include <std/thr/io_reactor.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>

using namespace stl;

CoroFDInput::CoroFDInput(FD& _fd, CoroExecutor* _exec) noexcept
    : fd(&_fd)
    , exec(_exec)
    , offset(0)
{
}

CoroFDInput::~CoroFDInput() noexcept {
}

size_t CoroFDInput::readImpl(void* data, size_t len) {
    size_t n = 0;

    if (int r = exec->io()->pread(fd->get(), &n, data, len, offset)) {
        Errno(r).raise(StringBuilder() << StringView(u8"pread() failed"));
    }

    offset += n;

    return n;
}

size_t CoroFDInput::hintImpl() const noexcept {
    return 1 << 14;
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

bool ZeroCopyInput::readLineZc(StringView& out, Buffer& fallback) {
    const void* chunk;
    size_t len = next(&chunk);

    if (!len) {
        return false;
    }

    StringView part((const u8*)chunk, len);

    if (auto p = part.memChr(u8'\n')) {
        out = StringView(part.begin(), p);
        commit(out.length() + 1);

        return true;
    }

    fallback.reset();

    if (!readTo(fallback, u8'\n')) {
        return false;
    }

    out = StringView(fallback);

    return true;
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
    } while ((len = next(&chunk)));

    return true;
}

void ZeroCopyInput::drain() {
    const void* chunk;
    size_t n;

    while ((n = next(&chunk))) {
        commit(n);
    }
}
./std/ios/in_zero.cpp
#include "in_zero.h"

using namespace stl;

size_t ZeroInput::readImpl(void* data, size_t len) {
    (void)data;
    (void)len;
    return 0;
}

size_t ZeroInput::nextImpl(const void** chunk) {
    *chunk = "";

    return 0;
}

void ZeroInput::commitImpl(size_t len) noexcept {
    (void)len;
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
./std/ios/out.cpp
#include "out.h"
#include "out_fd.h"
#include "out_buf.h"
#include "out_mem.h"
#include "out_null.h"
#include "out_fd_coro.h"

#include <std/mem/obj_pool.h>

using namespace stl;

Output* stl::createFDOutput(ObjPool* pool, FD& fd) {
    return pool->make<FDOutput>(fd);
}

Output* stl::createFDRegular(ObjPool* pool, FD& fd) {
    return pool->make<FDRegular>(fd);
}

Output* stl::createFDCharacter(ObjPool* pool, FD& fd) {
    return pool->make<FDCharacter>(fd);
}

Output* stl::createFDPipe(ObjPool* pool, FD& fd) {
    return pool->make<FDPipe>(fd);
}

Output* stl::createCoroFDOutput(ObjPool* pool, FD& fd, CoroExecutor* exec) {
    return pool->make<CoroFDOutput>(fd, exec);
}

ZeroCopyOutput* stl::createMemoryOutput(ObjPool* pool, void* ptr) {
    return pool->make<MemoryOutput>(ptr);
}

ZeroCopyOutput* stl::createOutBuf(ObjPool* pool, Output& out) {
    return pool->make<OutBuf>(out);
}

ZeroCopyOutput* stl::createOutBuf(ObjPool* pool, Output& out, size_t chunkSize) {
    return pool->make<OutBuf>(out, chunkSize);
}

Output* stl::createNullOutput(ObjPool* pool) {
    return pool->make<NullOutput>();
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
./std/ios/out_fd_coro.cpp
#include "out_fd_coro.h"

#include <std/sys/fd.h>
#include <std/str/view.h>
#include <std/thr/coro.h>
#include <std/thr/io_reactor.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>

using namespace stl;

CoroFDOutput::CoroFDOutput(FD& _fd, CoroExecutor* _exec) noexcept
    : fd(&_fd)
    , exec(_exec)
    , offset(0)
{
}

CoroFDOutput::~CoroFDOutput() noexcept {
}

size_t CoroFDOutput::writeImpl(const void* data, size_t len) {
    size_t n = 0;

    if (int r = exec->io()->pwrite(fd->get(), &n, data, len, offset)) {
        Errno(r).raise(StringBuilder() << StringView(u8"pwrite() failed"));
    }

    offset += n;

    return n;
}

size_t CoroFDOutput::hintImpl() const noexcept {
    return 1 << 14;
}

void CoroFDOutput::sync() {
    if (int r = exec->io()->fsync(fd->get())) {
        Errno(r).raise(StringBuilder() << StringView(u8"fsync() failed"));
    }
}

void CoroFDOutput::dataSync() {
    if (int r = exec->io()->fdatasync(fd->get())) {
        Errno(r).raise(StringBuilder() << StringView(u8"fdatasync() failed"));
    }
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
./std/ios/out_null.cpp
#include "out_null.h"

using namespace stl;

NullOutput::~NullOutput() noexcept {
}

size_t NullOutput::writeImpl(const void* data, size_t len) {
    (void)data;

    return len;
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

ZeroCopyOutput* ZeroCopyOutput::upgrade() noexcept {
    return this;
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

ZeroCopyOutput* Output::upgrade() noexcept {
    return nullptr;
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

        while (iovcnt > 0 && written >= iov->iov_len) {
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
./std/ios/stream_tcp.cpp
#include "stream_tcp.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>
#include <std/net/tcp_socket.h>

#include <sys/uio.h>

using namespace stl;

TcpStream::TcpStream(TcpSocket& sock) noexcept
    : sock(&sock)
{
}

TcpStream::~TcpStream() noexcept {
}

size_t TcpStream::readImpl(void* data, size_t len) {
    size_t n = 0;

    if (int r = sock->readInf(&n, data, len); r) {
        Errno(r).raise(StringBuilder() << StringView(u8"tcp read() failed"));
    }

    return n;
}

size_t TcpStream::writeImpl(const void* data, size_t len) {
    size_t n = 0;

    if (int r = sock->writeInf(&n, data, len); r) {
        Errno(r).raise(StringBuilder() << StringView(u8"tcp write() failed"));
    }

    return n;
}

size_t TcpStream::writeVImpl(iovec* iov, size_t count) {
    size_t n = 0;

    if (int r = sock->writevInf(&n, iov, count); r) {
        Errno(r).raise(StringBuilder() << StringView(u8"tcp writev() failed"));
    }

    return n;
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

void IntrusiveList::cutHalf(IntrusiveList& other) noexcept {
    if (almostEmpty()) {
        return;
    }

    auto slow = head.next;
    auto fast = head.next;

    while (fast->next != &head && fast->next->next != &head) {
        slow = slow->next;
        fast = fast->next->next;
    }

    auto mid = slow->next;
    auto back = head.prev;

    link(slow, &head);
    link(&other.head, mid);
    link(back, &other.head);
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

void IntrusiveList::pushFront(IntrusiveList& lst) noexcept {
    lst.pushBack(*this);
    lst.xchgWithEmptyList(*this);
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
    if (singular()) {
        return;
    }

    prev->next = next;
    next->prev = prev;
    reset();
}

void IntrusiveNode::reset() noexcept {
    prev = this;
    next = this;
}

bool IntrusiveNode::singular() const noexcept {
    return prev == this;
}

bool IntrusiveNode::almostEmpty() const noexcept {
    return next->next == this;
}
./std/lib/producer.cpp
#include "producer.h"

using namespace stl;

ProducerIface::~ProducerIface() noexcept {
}
./std/lib/vector.cpp
#include "vector.h"

using namespace stl;

static_assert(sizeof(Vector<void*>) == sizeof(void*));
./std/lib/visitor.cpp
#include "visitor.h"
./std/map/map.cpp
#include "map.h"
./std/map/treap.cpp
#include "treap.h"
#include "treap_node.h"

#include <std/alg/xchg.h>
#include <std/lib/vector.h>
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
            *ptr = merge(current->left, current->right);
            current->left = nullptr;
            current->right = nullptr;

            return current;
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

TreapNode* Treap::min() const noexcept {
    auto t = root;

    while (t && t->left) {
        t = t->left;
    }

    return t;
}

void Treap::xchg(Treap& other) noexcept {
    stl::xchg(root, other.root);
}

size_t Treap::height() const noexcept {
    if (root) {
        return root->height();
    }

    return 0;
}
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
./std/mem/disposable.cpp
#include "disposable.h"

using namespace stl;

Disposable::~Disposable() noexcept {
}
./std/mem/disposer.cpp
#include "disposer.h"

#include <std/alg/exchange.h>
#include <std/alg/destruct.h>

using namespace stl;

void Disposer::dispose() noexcept {
    auto cur = exchange(end, (Disposable*)0);

    while (cur) {
        destruct(exchange(cur, cur->prev));
    }
}

unsigned Disposer::length() const noexcept {
    unsigned res = 0;

    for (auto cur = end; cur; cur = cur->prev) {
        ++res;
    }

    return res;
}
./std/mem/embed.cpp
#include "embed.h"
./std/mem/free_list.cpp
#include "free_list.h"
#include "mem_pool.h"
#include "obj_pool.h"

#include <std/alg/minmax.h>
#include <std/alg/exchange.h>

using namespace stl;

namespace {
    struct Node {
        Node* next;
    };

    struct Impl: public FreeList {
        MemoryPool* mp;
        size_t objSize;
        Node* freeList;

        Impl(MemoryPool* mp, size_t os) noexcept
            : mp(mp)
            , objSize(max(os, sizeof(Node)))
            , freeList(nullptr)
        {
        }

        void* allocate() override {
            if (freeList) {
                return exchange(freeList, freeList->next);
            }

            return mp->allocate(objSize);
        }

        void release(void* ptr) noexcept override {
            auto node = (Node*)ptr;
            node->next = freeList;
            freeList = node;
        }
    };
}

FreeList::~FreeList() noexcept {
}

FreeList* FreeList::create(ObjPool* pool, size_t objSize) {
    return pool->make<Impl>(pool->memoryPool(), objSize);
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
}
./std/mem/new.cpp
#include "new.h"
./std/mem/obj_list.cpp
#include "obj_list.h"
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

        MemoryPool* memoryPool() noexcept override {
            return &mp;
        }
    };

    static_assert(sizeof(Pool) == 256);
}

ObjPool::~ObjPool() noexcept {
}

void* ObjPool::allocateOverAligned(size_t len, size_t align) {
    auto raw = (uintptr_t)allocate(len + align);

    return (void*)((raw + align - 1) & ~(align - 1));
}

ObjPool* ObjPool::create(ObjPool* pool) {
    return pool->make<Pool>();
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
./std/net/http_client.cpp
#include "http_client.h"

#include "http_io.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/ios/in_zc.h>
#include <std/ios/output.h>
#include <std/sym/s_map.h>
#include <std/lib/buffer.h>
#include <std/lib/vector.h>
#include <std/ios/in.h>
#include <std/str/builder.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    struct HttpClientResponseImpl: public HttpClientResponse {
        ObjPool* pool;
        u32 statusCode;
        StringView statusReason;
        SymbolMap<StringView> headers;
        ZeroCopyInput* bodyIn;
        Buffer line;
        Buffer lcName;

        HttpClientResponseImpl(ObjPool* pool, ZeroCopyInput* in);

        u32 status() override;
        StringView reason() override;
        StringView* header(StringView name) override;
        ZeroCopyInput* body() override;
    };

    struct HttpClientRequestImpl: public HttpClientRequest {
        struct Header {
            StringView name;
            StringView value;
        };

        ObjPool* pool;
        ZeroCopyInput* in;
        Output* rawOut;
        StringView reqMethod;
        StringView reqPath;
        Vector<Header*> headers;
        SymbolMap<Header*> headerIndex;
        Buffer line;
        Buffer lcName;

        HttpClientRequestImpl(ObjPool* pool, ZeroCopyInput* in, Output* out);

        void setMethod(StringView method) override;
        void setPath(StringView path) override;
        void addHeader(StringView name, StringView value) override;
        void endHeaders() override;
        Output* out() override;
        HttpClientResponse* response() override;

        void serialize(StringBuilder& sb);
    };
}

HttpClientResponseImpl::HttpClientResponseImpl(ObjPool* pool, ZeroCopyInput* in)
    : pool(pool)
    , statusCode(0)
    , headers(pool)
    , bodyIn(nullptr)
{
    in->readLine(line);

    StringView first(line);

    first = first.stripCr();

    StringView version, rest, code, reason;

    first.split(' ', version, rest);
    rest.split(' ', code, reason);

    statusCode = code.stou();
    statusReason = pool->intern(reason);

    for (;;) {
        line.reset();
        in->readLine(line);

        StringView name, val;

        if (!StringView(line).stripCr().split(':', name, val)) {
            break;
        }

        headers.insert(name.lower(lcName), pool->intern(val.stripSpace()));
    }

    if (auto te = headers.find(StringView("transfer-encoding")); te && te->lower(line) == StringView("chunked")) {
        bodyIn = createChunkedInput(pool, in);
    } else if (auto cl = headers.find(StringView("content-length")); cl) {
        bodyIn = createLimitedInput(pool, in, cl->stou());
    } else {
        bodyIn = createZeroInput(pool);
    }
}

u32 HttpClientResponseImpl::status() {
    return statusCode;
}

StringView HttpClientResponseImpl::reason() {
    return statusReason;
}

StringView* HttpClientResponseImpl::header(StringView name) {
    return headers.find(name.lower(lcName));
}

ZeroCopyInput* HttpClientResponseImpl::body() {
    return bodyIn;
}

HttpClientRequestImpl::HttpClientRequestImpl(ObjPool* pool, ZeroCopyInput* in, Output* out)
    : pool(pool)
    , in(in)
    , rawOut(out)
    , reqMethod(StringView("GET"))
    , reqPath(StringView("/"))
    , headerIndex(pool)
{
}

void HttpClientRequestImpl::setMethod(StringView method) {
    reqMethod = pool->intern(method);
}

void HttpClientRequestImpl::setPath(StringView path) {
    reqPath = pool->intern(path);
}

void HttpClientRequestImpl::addHeader(StringView name, StringView value) {
    auto h = pool->make<Header>();

    h->name = pool->intern(name);
    h->value = pool->intern(value);

    headers.pushBack(h);
    headerIndex.insert(name.lower(lcName), h);
}

void HttpClientRequestImpl::serialize(StringBuilder& sb) {
    sb << reqMethod
       << StringView(u8" ")
       << reqPath
       << StringView(u8" HTTP/1.1\r\n");

    for (auto it = headers.begin(); it != headers.end(); ++it) {
        sb << (*it)->name << StringView(u8": ") << (*it)->value << StringView(u8"\r\n");
    }

    sb << StringView(u8"\r\n");
}

void HttpClientRequestImpl::endHeaders() {
    if (!headerIndex.find(StringView("content-length")) && !headerIndex.find(StringView("transfer-encoding"))) {
        if (reqMethod == StringView("POST") || reqMethod == StringView("PUT") || reqMethod == StringView("PATCH")) {
            addHeader(StringView("Transfer-Encoding"), StringView("chunked"));
        }
    }

    {
        StringBuilder sb;

        serialize(sb);
        rawOut->write(sb.data(), sb.used());
    }

    if (auto cl = headerIndex.find(StringView("content-length")); cl) {
        rawOut = createLimitedOutput(pool, rawOut, (*cl)->value.stou());
    } else if (auto te = headerIndex.find(StringView("transfer-encoding")); te && (*te)->value == StringView("chunked")) {
        rawOut = createChunkedOutput(pool, rawOut);
    }
}

Output* HttpClientRequestImpl::out() {
    return rawOut;
}

HttpClientResponse* HttpClientRequestImpl::response() {
    rawOut->finish();
    rawOut->flush();
    return pool->make<HttpClientResponseImpl>(pool, in);
}

HttpClientRequest* stl::HttpClientRequest::create(ObjPool* pool, ZeroCopyInput* in, Output* out) {
    return pool->make<HttpClientRequestImpl>(pool, in, out);
}
./std/net/http_io.cpp
#include "http_io.h"

#include <std/str/fmt.h>
#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/ios/in_zc.h>
#include <std/ios/output.h>
#include <std/lib/buffer.h>
#include <std/alg/minmax.h>
#include <std/dbg/insist.h>
#include <std/mem/obj_pool.h>

#include <sys/uio.h>

using namespace stl;

namespace {
    struct LimitedInput: public ZeroCopyInput {
        ZeroCopyInput* inner;
        size_t remaining;

        LimitedInput(ZeroCopyInput* inner, size_t limit);

        size_t readImpl(void* data, size_t len) override;
        size_t nextImpl(const void** chunk) override;
        void commitImpl(size_t len) noexcept override;
    };

    struct LimitedOutput: public Output {
        Output* inner;
        size_t remaining;

        LimitedOutput(Output* inner, size_t limit);

        size_t writeImpl(const void* data, size_t len) override;
        void flushImpl() override;
    };

    struct ChunkedOutput: public Output {
        Output* inner;

        ChunkedOutput(Output* inner);

        size_t writeImpl(const void* data, size_t len) override;
        void flushImpl() override;
        void finishImpl() override;
    };

    struct ChunkedInput: public ZeroCopyInput {
        ZeroCopyInput* inner;
        size_t chunkRemaining;
        bool eof;
        bool first;
        Buffer sizeBuf;

        ChunkedInput(ZeroCopyInput* inner);

        size_t nextImpl(const void** chunk) override;
        void commitImpl(size_t len) noexcept override;

        bool loadChunk();
    };
}

LimitedInput::LimitedInput(ZeroCopyInput* inner, size_t limit)
    : inner(inner)
    , remaining(limit)
{
}

size_t LimitedInput::readImpl(void* data, size_t len) {
    size_t n = inner->read(data, min(len, remaining));

    remaining -= n;

    return n;
}

size_t LimitedInput::nextImpl(const void** chunk) {
    if (!remaining) {
        return 0;
    }

    return min(inner->next(chunk), remaining);
}

void LimitedInput::commitImpl(size_t len) noexcept {
    inner->commit(len);
    remaining -= len;
}

ChunkedInput::ChunkedInput(ZeroCopyInput* inner)
    : inner(inner)
    , chunkRemaining(0)
    , eof(false)
    , first(true)
{
}

bool ChunkedInput::loadChunk() {
    if (!first) {
        char crlf[2];
        inner->read(crlf, 2);
    }

    first = false;
    sizeBuf.reset();
    inner->readLine(sizeBuf);
    chunkRemaining = StringView(sizeBuf).stripCr().stoh();

    if (!chunkRemaining) {
        char crlf[2];
        inner->read(crlf, 2);
        eof = true;
        return false;
    }

    return true;
}

size_t ChunkedInput::nextImpl(const void** chunk) {
    if (eof) {
        return 0;
    }

    if (!chunkRemaining && !loadChunk()) {
        return 0;
    }

    return min(inner->next(chunk), chunkRemaining);
}

void ChunkedInput::commitImpl(size_t len) noexcept {
    inner->commit(len);
    chunkRemaining -= len;
}

LimitedOutput::LimitedOutput(Output* inner, size_t limit)
    : inner(inner)
    , remaining(limit)
{
}

size_t LimitedOutput::writeImpl(const void* data, size_t len) {
    STD_INSIST(remaining > 0);

    size_t n = inner->write(data, min(len, remaining));

    remaining -= n;

    return n;
}

void LimitedOutput::flushImpl() {
    inner->flush();
}

ChunkedOutput::ChunkedOutput(Output* inner)
    : inner(inner)
{
}

size_t ChunkedOutput::writeImpl(const void* data, size_t len) {
    u8 buf[20];
    u8* e = (u8*)formatU64Base16(len, buf);

    *e++ = u8'\r';
    *e++ = u8'\n';

    iovec iov[3] = {
        {buf, (size_t)(e - buf)},
        {(void*)data, len},
        {(void*)"\r\n", 2},
    };

    inner->writeV(iov, 3);

    return len;
}

void ChunkedOutput::flushImpl() {
    inner->flush();
}

void ChunkedOutput::finishImpl() {
    inner->write(u8"0\r\n\r\n", 5);
}

ZeroCopyInput* stl::createLimitedInput(ObjPool* pool, ZeroCopyInput* inner, size_t limit) {
    return pool->make<LimitedInput>(inner, limit);
}

ZeroCopyInput* stl::createChunkedInput(ObjPool* pool, ZeroCopyInput* inner) {
    return pool->make<ChunkedInput>(inner);
}

Output* stl::createLimitedOutput(ObjPool* pool, Output* inner, size_t limit) {
    return pool->make<LimitedOutput>(inner, limit);
}

Output* stl::createChunkedOutput(ObjPool* pool, Output* inner) {
    return pool->make<ChunkedOutput>(inner);
}
./std/net/http_reason.cpp
#include "http_reason.h"

using namespace stl;

StringView stl::reasonPhrase(u32 code) {
    switch (code) {
        case 200:
            return StringView(u8"OK");
        case 201:
            return StringView(u8"Created");
        case 204:
            return StringView(u8"No Content");
        case 301:
            return StringView(u8"Moved Permanently");
        case 302:
            return StringView(u8"Found");
        case 304:
            return StringView(u8"Not Modified");
        case 400:
            return StringView(u8"Bad Request");
        case 401:
            return StringView(u8"Unauthorized");
        case 403:
            return StringView(u8"Forbidden");
        case 404:
            return StringView(u8"Not Found");
        case 405:
            return StringView(u8"Method Not Allowed");
        case 500:
            return StringView(u8"Internal Server Error");
        case 502:
            return StringView(u8"Bad Gateway");
        case 503:
            return StringView(u8"Service Unavailable");
    }

    return StringView(u8"Unknown");
}
./std/net/http_srv.cpp
#include "http_srv.h"

#include "http_io.h"
#include "tcp_socket.h"
#include "ssl_socket.h"
#include "http_reason.h"

#include <std/sys/fd.h>
#include <std/sys/crt.h>
#include <std/ios/sys.h>
#include <std/thr/coro.h>
#include <std/alg/defer.h>
#include <std/ios/input.h>
#include <std/sym/s_map.h>
#include <std/sys/throw.h>
#include <std/sys/atomic.h>
#include <std/dbg/insist.h>
#include <std/dbg/verify.h>
#include <std/ios/in.h>
#include <std/ios/in_zc.h>
#include <std/ios/out.h>
#include <std/ios/output.h>
#include <std/lib/buffer.h>
#include <std/lib/vector.h>
#include <std/sys/atomic.h>
#include <std/sys/num_cpu.h>
#include <std/str/builder.h>
#include <std/mem/obj_pool.h>
#include <std/ios/stream_tcp.h>
#include <std/thr/wait_group.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace stl;

namespace {
    struct HttpConnection;

    struct HttpServerRequestImpl: public HttpServerRequest {
        ObjPool* pool;
        HttpConnection* conn;
        StringView reqMethod;
        StringView reqPath;
        StringView reqQuery;
        SymbolMap<StringView> headers{pool};
        ZeroCopyInput* reqIn;
        bool keepAlive = false;

        HttpServerRequestImpl(ObjPool* pool, HttpConnection* conn, StringView reqLine);

        bool serve();

        StringView path() override;
        StringView query() override;
        StringView method() override;
        ZeroCopyInput* in() override;
        StringView* header(StringView name) override;
    };

    struct HttpServerCtlImpl: public HttpServerCtl {
        HttpServeOpts* opts_;
        bool stopped;

        HttpServerCtlImpl(HttpServeOpts* opts);

        void stop() override;
        TcpSocket* listen(ObjPool* pool);
        void run(TcpSocket* srv);
    };

    struct HttpConnection {
        HttpServeOpts* opts;
        ObjPool* pool;
        TcpSocket sock;
        ZeroCopyInput* in;
        Output* out;
        Buffer line;
        Buffer lcName;

        HttpConnection(HttpServeOpts* opts, ObjPool* pool, FD* client);

        void run();
        bool serve();
    };

    struct HttpServerResponseImpl: public HttpServerResponse {
        struct Header {
            StringView name;
            StringView value;
        };

        HttpServerRequestImpl* req;
        Output* rawOut;
        Vector<Header*> headers;
        SymbolMap<Header*> headerIndex{req->pool};
        u32 status;

        HttpServerResponseImpl(HttpServerRequestImpl* req);

        Output* out() override;
        void endHeaders() override;
        void setStatus(u32 code) override;
        void serialize(ZeroCopyOutput& out);
        HttpServerRequest* request() override;
        void addHeader(StringView name, StringView value) override;
    };

    HttpServeOpts* internOpts(ObjPool* pool, HttpServeOpts opts) {
        if (!opts.exec) {
            opts.exec = CoroExecutor::create(pool, numCpu());
        }

        if (!opts.wg) {
            opts.wg = WaitGroup::create(pool, 0);
        }

        auto storage = pool->make<sockaddr_storage>();

        memCpy(storage, opts.addr, opts.addrLen);

        opts.addr = (const sockaddr*)storage;

        return pool->make<HttpServeOpts>(opts);
    }
}

HttpServerRequestImpl::HttpServerRequestImpl(ObjPool* pool, HttpConnection* conn, StringView reqLine)
    : pool(pool)
    , conn(conn)
{
    StringView method, rest, path, version;

    STD_VERIFY(reqLine.stripCr().split(' ', method, rest));

    rest.split(' ', path, version);

    reqMethod = pool->intern(method);

    StringView rawPath = path.empty() ? rest : path;
    StringView pathPart, queryPart;

    if (rawPath.split('?', pathPart, queryPart)) {
        reqPath = pool->intern(pathPart);
        reqQuery = pool->intern(queryPart);
    } else {
        reqPath = pool->intern(rawPath);
    }

    version = pool->intern(version);

    for (;;) {
        StringView hdrLine;

        if (!conn->in->readLineZc(hdrLine, conn->line)) {
            break;
        }

        StringView name, val;

        if (!hdrLine.stripCr().split(':', name, val)) {
            break;
        }

        headers.insert(name.lower(conn->lcName), pool->intern(val.stripSpace()));
    }

    if (auto h = headers.find(StringView("connection")); h) {
        keepAlive = h->lower(conn->line) == StringView("keep-alive");
    } else {
        keepAlive = version.lower(conn->line) == StringView("http/1.1");
    }

    if (auto te = headers.find(StringView("transfer-encoding")); te && te->lower(conn->line) == StringView("chunked")) {
        reqIn = createChunkedInput(pool, conn->in);
    } else if (auto cl = headers.find(StringView("content-length")); cl) {
        reqIn = createLimitedInput(pool, conn->in, cl->stou());
    } else {
        reqIn = createZeroInput(pool);
    }
}

bool HttpServerRequestImpl::serve() {
    HttpServerResponseImpl resp(this);

    conn->opts->handler->serve(resp);
    reqIn->drain();

    return keepAlive;
}

StringView HttpServerRequestImpl::method() {
    return reqMethod;
}

StringView HttpServerRequestImpl::path() {
    return reqPath;
}

StringView HttpServerRequestImpl::query() {
    return reqQuery;
}

ZeroCopyInput* HttpServerRequestImpl::in() {
    return reqIn;
}

StringView* HttpServerRequestImpl::header(StringView name) {
    return headers.find(name);
}

HttpServerResponseImpl::HttpServerResponseImpl(HttpServerRequestImpl* req)
    : req(req)
    , rawOut(req->conn->out)
    , status(200)
{
}

Output* HttpServerResponseImpl::out() {
    return rawOut;
}

HttpServerRequest* HttpServerResponseImpl::request() {
    return req;
}

void HttpServerResponseImpl::setStatus(u32 code) {
    status = code;
}

void HttpServerResponseImpl::addHeader(StringView name, StringView value) {
    auto pool = req->pool;
    auto h = pool->make<Header>();

    h->name = pool->intern(name);
    h->value = pool->intern(value);

    headers.pushBack(h);
    headerIndex.insert(name.lower(req->conn->lcName), h);
}

void HttpServerResponseImpl::serialize(ZeroCopyOutput& out) {
    size_t need = 128;

    for (auto it = headers.begin(); it != headers.end(); ++it) {
        need += (*it)->name.length() + (*it)->value.length() + 4;
    }

    need += 2;

    auto start = out.imbue(need);
    auto buf = start;

    buf = buf << StringView(u8"HTTP/1.1 ")
              << (u64)status
              << StringView(u8" ")
              << reasonPhrase(status)
              << StringView(u8"\r\n");

    for (auto it = headers.begin(); it != headers.end(); ++it) {
        buf = buf << (*it)->name << StringView(u8": ") << (*it)->value << StringView(u8"\r\n");
    }

    buf = buf << StringView(u8"\r\n");

    out.commit(start.distance(buf));
}

void HttpServerResponseImpl::endHeaders() {
    auto pool = req->pool;

    auto cl = headerIndex.find(StringView("content-length"));
    auto te = headerIndex.find(StringView("transfer-encoding"));
    bool chunked = te && (*te)->value == StringView("chunked");

    if (req->keepAlive && !cl && !te) {
        addHeader(StringView("Transfer-Encoding"), StringView("chunked"));
        chunked = true;
    }

    if (auto zc = rawOut->upgrade()) {
        serialize(*zc);
    } else {
        StringBuilder sb;

        sb.xchg(req->conn->line);
        sb.reset();
        serialize(sb);
        rawOut->write(sb.data(), sb.used());
        sb.xchg(req->conn->line);
    }

    if (cl) {
        rawOut = createLimitedOutput(pool, rawOut, (*cl)->value.stou());
    } else if (chunked) {
        rawOut = createChunkedOutput(pool, rawOut);
    }

    if (auto conn = headerIndex.find(StringView("connection")); conn) {
        if ((*conn)->value != StringView("keep-alive")) {
            req->keepAlive = false;
        }
    }
}

HttpServerCtlImpl::HttpServerCtlImpl(HttpServeOpts* opts)
    : opts_(opts)
    , stopped(false)
{
}

void HttpServerCtlImpl::stop() {
    stdAtomicStore(&stopped, true, stl::MemoryOrder::Release);

    opts_->exec->spawn([this] {
        int cfd;

        if (TcpSocket::connectInf(&cfd, opts_->exec, opts_->addr, opts_->addrLen) == 0) {
            ::close(cfd);
        }
    });
}

TcpSocket* HttpServerCtlImpl::listen(ObjPool* pool) {
    int sfd;

    STD_VERIFY(TcpSocket::socket(&sfd, opts_->addr->sa_family, SOCK_STREAM, 0) == 0);

    auto srv = TcpSocket::create(pool, sfd, opts_->exec);

    srv->setReuseAddr(true);

    STD_VERIFY(srv->bind(opts_->addr, opts_->addrLen) == 0);
    STD_VERIFY(srv->listen(opts_->backlog) == 0);

    return srv;
}

void HttpServerCtlImpl::run(TcpSocket* srv) {
    for (;;) {
        auto cpool = ObjPool::fromMemory();
        auto client = cpool->make<ScopedFD>();

        if (srv->acceptInf(*client, nullptr, nullptr) != 0) {
            break;
        }

        if (stdAtomicFetch(&stopped, stl::MemoryOrder::Acquire)) {
            break;
        }

        opts_->exec->spawn([this, cpool, client] mutable {
            try {
                HttpConnection(opts_, cpool.mutPtr(), client).run();
            } catch (...) {
                sysE << Exception::current() << endL << flsH;
            }
        });
    }
}

HttpConnection::HttpConnection(HttpServeOpts* opts, ObjPool* pool, FD* client)
    : opts(opts)
    , pool(pool)
    , sock(client->get(), opts->exec)
{
    sock.setNoDelay(true);

    auto stream = pool->make<TcpStream>(sock);

    Input* in = stream;
    out = createOutBuf(pool, *stream);

    if (auto ssl = opts->handler->ssl()) {
        u8 b;

        if (sock.peek(b) && b == 0x16) {
            auto s = ssl->create(pool, in, out);

            in = s;
            out = s;
        }
    }

    this->in = createInBuf(pool, *in);
}

void HttpConnection::run() {
    while (serve()) {
        out->flush();
    }

    out->finish();
}

bool HttpConnection::serve() {
    StringView reqLine;

    if (!in->readLineZc(reqLine, line)) {
        return false;
    }

    auto pool = ObjPool::fromMemory();

    return HttpServerRequestImpl(pool.mutPtr(), this, reqLine).serve();
}

HttpServerCtl* stl::serve(ObjPool* pool, HttpServeOpts opts) {
    auto popts = internOpts(pool, opts);
    auto ctl = pool->make<HttpServerCtlImpl>(popts);
    auto srv = ctl->listen(pool);

    popts->wg->inc();

    popts->exec->spawn([popts, ctl, srv] {
        STD_DEFER {
            popts->wg->done();
        };

        ctl->run(srv);
    });

    return ctl;
}

SslCtx* HttpServe::ssl() {
    return nullptr;
}
./std/net/ssl_socket.cpp
#include "ssl_socket.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>
#include <std/mem/obj_pool.h>

#include <stdlib.h>
#include <string.h>

using namespace stl;

#if __has_include(<openssl/ssl.h>)
    #include <openssl/ssl.h>
    #include <openssl/err.h>
    #include <openssl/bio.h>
    #define STD_HAVE_OPENSSL 1
#endif

#if __has_include(<s2n.h>)
    #include <s2n.h>
    #define STD_HAVE_S2N 1
#endif

#if __has_include(<mbedtls/ssl.h>)
    #include <mbedtls/ssl.h>
    #include <mbedtls/ctr_drbg.h>
    #include <mbedtls/entropy.h>
    #include <mbedtls/x509_crt.h>
    #define STD_HAVE_MBEDTLS 1
#endif

#if defined(STD_HAVE_OPENSSL)
namespace {
    namespace ossl {
        [[noreturn]]
        void raiseSsl() {
            unsigned long err = ERR_get_error();
            char buf[256];

            ERR_error_string_n(err, buf, sizeof(buf));
            Errno((int)err).raise(StringBuilder() << StringView(buf));
        }

        void checkSsl(int r) {
            if (r <= 0) {
                raiseSsl();
            }
        }

        struct BioMethod {
            BIO_METHOD* method;

            BioMethod();
            ~BioMethod() noexcept;
        };

        struct SslSocketImpl: public SslSocket {
            SSL* ssl;
            Input* in;
            Output* out;

            SslSocketImpl(SSL_CTX* ctx, BIO_METHOD* bioMethod, Input* in, Output* out);

            ~SslSocketImpl() noexcept {
                SSL_free(ssl);
            }

            size_t readImpl(void* data, size_t len) override;
            size_t writeImpl(const void* data, size_t len) override;
            void flushImpl() override;

            static int bioRead(BIO* bio, char* buf, int len);
            static int bioWrite(BIO* bio, const char* buf, int len);
            static long bioCtrl(BIO* bio, int cmd, long num, void* ptr);
        };

        struct SslCtxImpl: public SslCtx {
            SSL_CTX* ctx;
            BioMethod bioMethod;

            SslCtxImpl(StringView certData, StringView keyData);

            ~SslCtxImpl() noexcept {
                SSL_CTX_free(ctx);
            }

            SslSocket* create(ObjPool* pool, Input* in, Output* out) override;
        };
    }
}

ossl::BioMethod::BioMethod() {
    method = BIO_meth_new(BIO_get_new_index() | BIO_TYPE_SOURCE_SINK, "stl");

    BIO_meth_set_read(method, SslSocketImpl::bioRead);
    BIO_meth_set_write(method, SslSocketImpl::bioWrite);
    BIO_meth_set_ctrl(method, SslSocketImpl::bioCtrl);
}

ossl::BioMethod::~BioMethod() noexcept {
    BIO_meth_free(method);
}

int ossl::SslSocketImpl::bioRead(BIO* bio, char* buf, int len) {
    auto sock = (SslSocketImpl*)BIO_get_data(bio);

    sock->out->flush();

    size_t n = sock->in->read(buf, (size_t)len);

    if (n == 0) {
        BIO_set_retry_read(bio);

        return -1;
    }

    return (int)n;
}

int ossl::SslSocketImpl::bioWrite(BIO* bio, const char* buf, int len) {
    auto sock = (SslSocketImpl*)BIO_get_data(bio);

    sock->out->write(buf, (size_t)len);

    return len;
}

long ossl::SslSocketImpl::bioCtrl(BIO* bio, int cmd, long num, void* ptr) {
    (void)num;
    (void)ptr;

    if (cmd == BIO_CTRL_FLUSH) {
        ((SslSocketImpl*)BIO_get_data(bio))->out->flush();

        return 1;
    }

    return 0;
}

ossl::SslSocketImpl::SslSocketImpl(SSL_CTX* ctx, BIO_METHOD* bioMethod, Input* in, Output* out)
    : in(in)
    , out(out)
{
    ssl = SSL_new(ctx);

    BIO* bio = BIO_new(bioMethod);

    BIO_set_data(bio, this);
    BIO_set_init(bio, 1);
    SSL_set_bio(ssl, bio, bio);
    checkSsl(SSL_accept(ssl));
}

size_t ossl::SslSocketImpl::readImpl(void* data, size_t len) {
    int r = SSL_read(ssl, data, (int)len);

    if (r <= 0) {
        int err = SSL_get_error(ssl, r);

        if (err == SSL_ERROR_ZERO_RETURN) {
            return 0;
        }

        raiseSsl();
    }

    return (size_t)r;
}

size_t ossl::SslSocketImpl::writeImpl(const void* data, size_t len) {
    int r = SSL_write(ssl, data, (int)len);

    if (r <= 0) {
        raiseSsl();
    }

    return (size_t)r;
}

void ossl::SslSocketImpl::flushImpl() {
    out->flush();
}

ossl::SslCtxImpl::SslCtxImpl(StringView certData, StringView keyData) {
    ctx = SSL_CTX_new(TLS_server_method());

    BIO* certBio = BIO_new_mem_buf(certData.data(), (int)certData.length());
    X509* x509 = PEM_read_bio_X509(certBio, nullptr, nullptr, nullptr);

    checkSsl(SSL_CTX_use_certificate(ctx, x509));
    X509_free(x509);
    BIO_free(certBio);

    BIO* keyBio = BIO_new_mem_buf(keyData.data(), (int)keyData.length());
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(keyBio, nullptr, nullptr, nullptr);

    checkSsl(SSL_CTX_use_PrivateKey(ctx, pkey));
    EVP_PKEY_free(pkey);
    BIO_free(keyBio);
}

SslSocket* ossl::SslCtxImpl::create(ObjPool* pool, Input* in, Output* out) {
    return pool->make<SslSocketImpl>(ctx, bioMethod.method, in, out);
}
#endif

#if defined(STD_HAVE_S2N)
namespace {
    namespace s2n {
        [[noreturn]]
        void raiseSsl() {
            Errno(s2n_errno).raise(StringBuilder() << StringView(s2n_strerror(s2n_errno, "EN")));
        }

        void checkSsl(int r) {
            if (r < 0) {
                raiseSsl();
            }
        }

        struct SslSocketImpl: public SslSocket {
            struct s2n_connection* conn;
            Input* in;
            Output* out;

            SslSocketImpl(struct s2n_config* conf, Input* in, Output* out);

            ~SslSocketImpl() noexcept {
                s2n_connection_free(conn);
            }

            size_t readImpl(void* data, size_t len) override;
            size_t writeImpl(const void* data, size_t len) override;
            void flushImpl() override;

            static int recvCb(void* ctx, uint8_t* buf, uint32_t len);
            static int sendCb(void* ctx, const uint8_t* buf, uint32_t len);
        };

        struct SslCtxImpl: public SslCtx {
            struct s2n_config* conf;
            struct s2n_cert_chain_and_key* certKey;

            SslCtxImpl(StringView certData, StringView keyData);

            ~SslCtxImpl() noexcept {
                s2n_cert_chain_and_key_free(certKey);
                s2n_config_free(conf);
            }

            SslSocket* create(ObjPool* pool, Input* in, Output* out) override;
        };
    }
}

int s2n::SslSocketImpl::recvCb(void* ctx, uint8_t* buf, uint32_t len) {
    auto sock = (SslSocketImpl*)ctx;

    sock->out->flush();

    size_t n = sock->in->read(buf, (size_t)len);

    if (n == 0) {
        errno = EWOULDBLOCK;

        return -1;
    }

    return (int)n;
}

int s2n::SslSocketImpl::sendCb(void* ctx, const uint8_t* buf, uint32_t len) {
    ((SslSocketImpl*)ctx)->out->write(buf, (size_t)len);

    return (int)len;
}

s2n::SslSocketImpl::SslSocketImpl(struct s2n_config* conf, Input* in, Output* out)
    : in(in)
    , out(out)
{
    conn = s2n_connection_new(S2N_SERVER);
    s2n_connection_set_config(conn, conf);
    s2n_connection_set_recv_cb(conn, recvCb);
    s2n_connection_set_send_cb(conn, sendCb);
    s2n_connection_set_recv_ctx(conn, this);
    s2n_connection_set_send_ctx(conn, this);

    s2n_blocked_status blocked;

    checkSsl(s2n_negotiate(conn, &blocked));
}

size_t s2n::SslSocketImpl::readImpl(void* data, size_t len) {
    s2n_blocked_status blocked;
    ssize_t r = s2n_recv(conn, data, (ssize_t)len, &blocked);

    if (r == 0) {
        return 0;
    }

    if (r < 0) {
        raiseSsl();
    }

    return (size_t)r;
}

size_t s2n::SslSocketImpl::writeImpl(const void* data, size_t len) {
    s2n_blocked_status blocked;
    ssize_t r = s2n_send(conn, data, (ssize_t)len, &blocked);

    if (r < 0) {
        raiseSsl();
    }

    return (size_t)r;
}

void s2n::SslSocketImpl::flushImpl() {
    out->flush();
}

s2n::SslCtxImpl::SslCtxImpl(StringView certData, StringView keyData) {
    s2n_init();

    conf = s2n_config_new();
    certKey = s2n_cert_chain_and_key_new();
    checkSsl(s2n_cert_chain_and_key_load_pem_bytes(certKey, (uint8_t*)certData.data(), (uint32_t)certData.length(), (uint8_t*)keyData.data(), (uint32_t)keyData.length()));
    checkSsl(s2n_config_add_cert_chain_and_key_to_store(conf, certKey));
}

SslSocket* s2n::SslCtxImpl::create(ObjPool* pool, Input* in, Output* out) {
    return pool->make<SslSocketImpl>(conf, in, out);
}
#endif

#if defined(STD_HAVE_MBEDTLS)
namespace {
    namespace mbed {
        [[noreturn]]
        void raiseSsl(int err) {
            Errno(err).raise(StringBuilder() << StringView(u8"ssl error"));
        }

        void checkSsl(int r) {
            if (r != 0) {
                raiseSsl(r);
            }
        }

        struct SslContext: public mbedtls_ssl_context {
            SslContext() noexcept {
                mbedtls_ssl_init(this);
            }

            ~SslContext() noexcept {
                mbedtls_ssl_free(this);
            }
        };

        struct SslSocketImpl: public SslSocket {
            SslContext ssl;
            Input* in;
            Output* out;

            SslSocketImpl(mbedtls_ssl_config* conf, Input* in, Output* out);

            size_t readImpl(void* data, size_t len) override;
            size_t writeImpl(const void* data, size_t len) override;
            void flushImpl() override;

            static int recv(void* ctx, unsigned char* buf, size_t len);
            static int send(void* ctx, const unsigned char* buf, size_t len);
        };

        struct SslConf: public mbedtls_ssl_config {
            SslConf() noexcept {
                mbedtls_ssl_config_init(this);
            }

            ~SslConf() noexcept {
                mbedtls_ssl_config_free(this);
            }
        };

        struct SslCert: public mbedtls_x509_crt {
            SslCert() noexcept {
                mbedtls_x509_crt_init(this);
            }

            ~SslCert() noexcept {
                mbedtls_x509_crt_free(this);
            }
        };

        struct SslKey: public mbedtls_pk_context {
            SslKey() noexcept {
                mbedtls_pk_init(this);
            }

            ~SslKey() noexcept {
                mbedtls_pk_free(this);
            }
        };

        struct SslEntropy: public mbedtls_entropy_context {
            SslEntropy() noexcept {
                mbedtls_entropy_init(this);
            }

            ~SslEntropy() noexcept {
                mbedtls_entropy_free(this);
            }
        };

        struct SslCtrDrbg: public mbedtls_ctr_drbg_context {
            SslCtrDrbg() noexcept {
                mbedtls_ctr_drbg_init(this);
            }

            ~SslCtrDrbg() noexcept {
                mbedtls_ctr_drbg_free(this);
            }
        };

        struct SslCtxImpl: public SslCtx {
            SslConf conf;
            SslCert cert;
            SslKey key;
            SslEntropy entropy;
            SslCtrDrbg ctr_drbg;

            SslCtxImpl(StringView certData, StringView keyData);

            SslSocket* create(ObjPool* pool, Input* in, Output* out) override;
        };
    }
}

int mbed::SslSocketImpl::recv(void* ctx, unsigned char* buf, size_t len) {
    auto sock = (SslSocketImpl*)ctx;

    sock->out->flush();

    size_t n = sock->in->read(buf, len);

    if (n == 0) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    return (int)n;
}

int mbed::SslSocketImpl::send(void* ctx, const unsigned char* buf, size_t len) {
    ((SslSocketImpl*)ctx)->out->write(buf, len);

    return (int)len;
}

mbed::SslSocketImpl::SslSocketImpl(mbedtls_ssl_config* conf, Input* in, Output* out)
    : in(in)
    , out(out)
{
    checkSsl(mbedtls_ssl_setup(&ssl, conf));
    mbedtls_ssl_set_bio(&ssl, this, send, recv, nullptr);
    checkSsl(mbedtls_ssl_handshake(&ssl));
}

size_t mbed::SslSocketImpl::readImpl(void* data, size_t len) {
    int r = mbedtls_ssl_read(&ssl, (unsigned char*)data, len);

    if (r == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || r == 0) {
        return 0;
    }

    if (r < 0) {
        raiseSsl(r);
    }

    return (size_t)r;
}

size_t mbed::SslSocketImpl::writeImpl(const void* data, size_t len) {
    int r = mbedtls_ssl_write(&ssl, (const unsigned char*)data, len);

    if (r < 0) {
        raiseSsl(r);
    }

    return (size_t)r;
}

void mbed::SslSocketImpl::flushImpl() {
    out->flush();
}

mbed::SslCtxImpl::SslCtxImpl(StringView certData, StringView keyData) {
    checkSsl(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, nullptr, 0));
    checkSsl(mbedtls_x509_crt_parse(&cert, (const unsigned char*)certData.data(), certData.length() + 1));
    checkSsl(mbedtls_pk_parse_key(&key, (const unsigned char*)keyData.data(), keyData.length() + 1, nullptr, 0, mbedtls_ctr_drbg_random, &ctr_drbg));
    checkSsl(mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT));
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_ca_chain(&conf, cert.next, nullptr);
    checkSsl(mbedtls_ssl_conf_own_cert(&conf, &cert, &key));
}

SslSocket* mbed::SslCtxImpl::create(ObjPool* pool, Input* in, Output* out) {
    return pool->make<SslSocketImpl>(&conf, in, out);
}
#endif

namespace {
    SslCtx* createOpenSsl([[maybe_unused]] ObjPool* pool, [[maybe_unused]] StringView cert, [[maybe_unused]] StringView key) {
#if defined(STD_HAVE_OPENSSL)
        return pool->make<ossl::SslCtxImpl>(cert, key);
#else
        return nullptr;
#endif
    }

    SslCtx* createS2n([[maybe_unused]] ObjPool* pool, [[maybe_unused]] StringView cert, [[maybe_unused]] StringView key) {
#if defined(STD_HAVE_S2N)
        return pool->make<s2n::SslCtxImpl>(cert, key);
#else
        return nullptr;
#endif
    }

    SslCtx* createMbedTls([[maybe_unused]] ObjPool* pool, [[maybe_unused]] StringView cert, [[maybe_unused]] StringView key) {
#if defined(STD_HAVE_MBEDTLS)
        return pool->make<mbed::SslCtxImpl>(cert, key);
#else
        return nullptr;
#endif
    }
}

SslCtx* stl::SslCtx::create(ObjPool* pool, StringView cert, StringView key) {
    auto env = getenv("USE_SSL_ENGINE");

    if (env) {
        if (strcmp(env, "openssl") == 0) {
            return createOpenSsl(pool, cert, key);
        }

        if (strcmp(env, "s2n") == 0) {
            return createS2n(pool, cert, key);
        }

        if (strcmp(env, "mbedtls") == 0) {
            return createMbedTls(pool, cert, key);
        }
    }

    if (auto ctx = createOpenSsl(pool, cert, key); ctx) {
        return ctx;
    }

    if (auto ctx = createS2n(pool, cert, key); ctx) {
        return ctx;
    }

    return createMbedTls(pool, cert, key);
}
./std/net/tcp_socket.cpp
#include "tcp_socket.h"

#include <std/sys/fd.h>
#include <std/sys/crt.h>
#include <std/thr/coro.h>
#include <std/thr/poll_fd.h>
#include <std/mem/obj_pool.h>
#include <std/thr/io_reactor.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#ifndef SOCK_NONBLOCK
    #define SOCK_NONBLOCK 0
    #define STD_SOCK_EMULATE_NB 1
#endif

#ifndef SOCK_CLOEXEC
    #define SOCK_CLOEXEC 0
    #define STD_SOCK_EMULATE_CE 1
#endif

#ifndef MSG_NOSIGNAL
    #define MSG_NOSIGNAL 0
#endif

using namespace stl;

namespace {
    void setSockFlags([[maybe_unused]] int fd) {
#ifdef STD_SOCK_EMULATE_NB
        ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK);
#endif

#ifdef STD_SOCK_EMULATE_CE
        ::fcntl(fd, F_SETFD, ::fcntl(fd, F_GETFD) | FD_CLOEXEC);
#endif

#ifdef SO_NOSIGPIPE
        int one = 1;
        ::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
    }
}

TcpSocket::TcpSocket(int fd, CoroExecutor* exec) noexcept
    : fd(fd)
    , io(exec->io())
{
    setNoDelay(true);
}

int TcpSocket::socket(int* out, int domain, int type, int protocol) {
    int fd = ::socket(domain, type | SOCK_NONBLOCK | SOCK_CLOEXEC, protocol);

    if (fd < 0) {
        return errno;
    }

    setSockFlags(fd);
    *out = fd;

    return 0;
}

void TcpSocket::close() {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

void TcpSocket::shutdown(int how) {
    ::shutdown(fd, how);
}

int TcpSocket::setReuseAddr(bool on) {
    int opt = on ? 1 : 0;

    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        return errno;
    }

    return 0;
}

int TcpSocket::setNoDelay(bool on) {
    int opt = on ? 1 : 0;

    if (::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
        return errno;
    }

    return 0;
}

int TcpSocket::bind(const sockaddr* addr, u32 addrLen) {
    if (int r = ::bind(fd, addr, addrLen); r < 0) {
        return errno;
    }

    return 0;
}

int TcpSocket::listen(int backlog) {
    if (int r = ::listen(fd, backlog); r < 0) {
        return errno;
    }

    return 0;
}

int TcpSocket::connect(int* out, CoroExecutor* exec, const sockaddr* addr, u32 addrLen, u64 deadlineUs) {
    int fd = ::socket(addr->sa_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

    if (fd < 0) {
        return errno;
    }

    setSockFlags(fd);

    if (int r = exec->io()->connect(fd, addr, addrLen, deadlineUs)) {
        ::close(fd);
        return r;
    }

    *out = fd;

    return 0;
}

int TcpSocket::connectTout(int* out, CoroExecutor* exec, const sockaddr* addr, u32 addrLen, u64 timeoutUs) {
    return connect(out, exec, addr, addrLen, monotonicNowUs() + timeoutUs);
}

int TcpSocket::connectInf(int* out, CoroExecutor* exec, const sockaddr* addr, u32 addrLen) {
    return connect(out, exec, addr, addrLen, UINT64_MAX);
}

int TcpSocket::accept(ScopedFD& out, sockaddr* addr, u32* addrLen, u64 deadlineUs) {
    int newFd;

    if (int r = io->accept(fd, &newFd, addr, addrLen, deadlineUs)) {
        return r;
    }

    setSockFlags(newFd);

    ScopedFD(newFd).xchg(out);

    return 0;
}

int TcpSocket::acceptTout(ScopedFD& out, sockaddr* addr, u32* addrLen, u64 timeoutUs) {
    return accept(out, addr, addrLen, monotonicNowUs() + timeoutUs);
}

int TcpSocket::acceptInf(ScopedFD& out, sockaddr* addr, u32* addrLen) {
    return accept(out, addr, addrLen, UINT64_MAX);
}

int TcpSocket::read(size_t* nRead, void* buf, size_t len, u64 deadlineUs) {
    return io->recv(fd, nRead, buf, len, deadlineUs);
}

int TcpSocket::readTout(size_t* nRead, void* buf, size_t len, u64 timeoutUs) {
    return read(nRead, buf, len, monotonicNowUs() + timeoutUs);
}

int TcpSocket::readInf(size_t* nRead, void* buf, size_t len) {
    return read(nRead, buf, len, UINT64_MAX);
}

int TcpSocket::write(size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) {
    return io->send(fd, nWritten, buf, len, deadlineUs);
}

int TcpSocket::writeTout(size_t* nWritten, const void* buf, size_t len, u64 timeoutUs) {
    return write(nWritten, buf, len, monotonicNowUs() + timeoutUs);
}

int TcpSocket::writeInf(size_t* nWritten, const void* buf, size_t len) {
    return write(nWritten, buf, len, UINT64_MAX);
}

int TcpSocket::writev(size_t* nWritten, iovec* iov, size_t iovcnt, u64 deadlineUs) {
    return io->writev(fd, nWritten, iov, iovcnt, deadlineUs);
}

int TcpSocket::writevInf(size_t* nWritten, iovec* iov, size_t iovcnt) {
    return writev(nWritten, iov, iovcnt, UINT64_MAX);
}

bool TcpSocket::peek(u8& out) {
    if (!io->poll({fd, PollFlag::In}, UINT64_MAX)) {
        return false;
    }

    return ::recv(fd, &out, 1, MSG_PEEK) == 1;
}

TcpSocket* TcpSocket::create(ObjPool* pool, int fd, CoroExecutor* exec) {
    struct ScopedTcpSocket: public TcpSocket {
        using TcpSocket::TcpSocket;

        ~ScopedTcpSocket() noexcept {
            close();
        }
    };

    return pool->make<ScopedTcpSocket>(fd, exec);
}
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
./std/ptr/intrusive.cpp
#include "intrusive.h"
./std/ptr/refcount.cpp
#include "refcount.h"

#include <std/alg/xchg.h>

void stl::xchgPtr(void** l, void** r) {
    xchg(*l, *r);
}
./std/ptr/scoped.cpp
#include "scoped.h"
./std/ptr/shared.cpp
#include "shared.h"
./std/rng/mix.cpp
#include "mix.h"
#include "split_mix_64.h"

using namespace stl;

u64 stl::mix(const void* a) noexcept {
    return splitMix64((uintptr_t)a);
}

u64 stl::mix(const void* a, const void* b) noexcept {
    return splitMix64((uintptr_t)a ^ mix(b));
}

u64 stl::mix(const void* a, const void* b, const void* c) noexcept {
    return splitMix64((uintptr_t)a ^ mix(b, c));
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

u64 PCG32::nextU64() noexcept {
    return splitMix64((u64)nextU32() << 32 | nextU32());
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

void* stl::formatU64Base16(u64 v, void* ptr) noexcept {
    u8* b = (u8*)ptr;
    u8* e = b;

    do {
        u8 d = v & 0xf;

        *e++ = d < 10 ? u8'0' + d : u8'a' + d - 10;
        v >>= 4;
    } while (v);

    reverse(b, e);

    return e;
}

void* stl::formatLongDouble(long double v, void* buf) noexcept {
    return advancePtr(buf, sprintf((char*)buf, "%Lf", v));
}
./std/str/hash.cpp
#include "hash.h"

#include <std/rng/split_mix_64.h>

#if __has_include(<rapidhash.h>)
    #include <rapidhash.h>
#elif __has_include(<xxhash.h>)
    #include <xxhash.h>
#endif

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
#if __has_include(<rapidhash.h>)
    return rapidhash(data, len);
#elif __has_include(<xxhash.h>)
    return XXH3_64bits(data, len);
#else
    u64 h = 14695981039346656037ull;
    const u8* p = (const u8*)data;

    for (size_t i = 0; i < len; ++i) {
        h ^= p[i];
        h *= 1099511628211ull;
    }

    return splitMix64(h);
#endif
}
./std/str/view.cpp
#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif
#include <string.h>

#include "view.h"
#include "hash.h"

#include <std/sys/crt.h>
#include <std/lib/buffer.h>
#include <std/alg/minmax.h>
#include <std/ios/out_zc.h>

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
    if (substr.empty()) {
        return data();
    }

    return (const u8*)memmem(fix(data()), length(), fix(substr.data()), substr.length());
}

const u8* StringView::memChr(u8 ch) const noexcept {
    return (const u8*)memchr(fix(data()), ch, length());
}

StringView StringView::stripCr() const noexcept {
    if (!empty() && back() == '\r') {
        return prefix(length() - 1);
    }

    return *this;
}

StringView StringView::stripSpace() const noexcept {
    const u8* p = data();
    const u8* e = end();

    while (p < e && *p == ' ') {
        ++p;
    }

    while (e > p && *(e - 1) == ' ') {
        --e;
    }

    return StringView(p, e);
}

bool StringView::split(u8 delim, StringView& before, StringView& after) const noexcept {
    if (auto p = memChr(delim); p) {
        before = StringView(data(), p);
        after = StringView(p + 1, end());

        return true;
    }

    return false;
}

StringView StringView::lower(u8* buffer) const noexcept {
    for (size_t i = 0; i < len_; ++i) {
        u8 ch = ptr_[i];
        buffer[i] = (ch >= 'A' && ch <= 'Z') ? (u8)(ch + ('a' - 'A')) : ch;
    }

    return StringView(buffer, len_);
}

StringView StringView::lower(Buffer& buffer) const noexcept {
    buffer.grow(length());

    return lower((u8*)buffer.mutData());
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

u64 StringView::stoh() const noexcept {
    u64 result = 0;

    for (auto ch : *this) {
        u8 digit;

        if (ch >= '0' && ch <= '9') {
            digit = ch - '0';
        } else if (ch >= 'a' && ch <= 'f') {
            digit = ch - 'a' + 10;
        } else if (ch >= 'A' && ch <= 'F') {
            digit = ch - 'A' + 10;
        } else {
            break;
        }

        result = result * 16 + digit;
    }

    return result;
}

template <>
void stl::output<ZeroCopyOutput, StringView>(ZeroCopyOutput& out, StringView str) {
    out.write(str.data(), str.length());
}
./std/sym/h_map.cpp
#include "h_map.h"
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

HashTable::Node* HashTable::any() const noexcept {
    for (auto node : buckets(buf)) {
        if (node) {
            return node;
        }
    }

    return nullptr;
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
./std/sym/i_map.cpp
#include "i_map.h"

#include <std/rng/split_mix_64.h>

using namespace stl;

u64 IntHasher::hash(u64 k) noexcept {
    return splitMix64(k);
}
./std/sym/s_map.cpp
#include "s_map.h"
./std/sys/atomic.cpp
#include "atomic.h"
./std/sys/crt.cpp
#include "crt.h"

#include <std/dbg/insist.h>
#include <std/ios/output.h>

#include <time.h>
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

u64 stl::monotonicNowUs() noexcept {
    timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (u64)ts.tv_sec * 1000000ULL + (u64)ts.tv_nsec / 1000;
}
./std/sys/event_fd.cpp
#include "event_fd.h"

#include <std/sys/fd.h>
#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>

#include <unistd.h>

#if defined(__linux__)
    #include <sys/eventfd.h>
#endif

using namespace stl;

struct EventFD::Impl {
#if defined(__linux__)
    ScopedFD efd;

    Impl() {
        int r = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

        if (r < 0) {
            Errno().raise(StringBuilder() << StringView(u8"eventfd() failed"));
        }

        ScopedFD(r).xchg(efd);
    }

    int fd() const noexcept {
        return efd.get();
    }

    void signal() {
        u64 val = 1;
        ::write(efd.get(), &val, sizeof(val));
    }

    void drain() {
        u64 val;
        ::read(efd.get(), &val, sizeof(val));
    }
#else
    ScopedFD rfd;
    ScopedFD wfd;

    Impl() {
        createPipeFD(rfd, wfd);
        rfd.setNonBlocking();
        wfd.setNonBlocking();
    }

    int fd() const noexcept {
        return rfd.get();
    }

    void signal() {
        char val = 1;
        ::write(wfd.get(), &val, sizeof(val));
    }

    void drain() {
        char buf[256];

        while (::read(rfd.get(), buf, sizeof(buf)) > 0) {
        }
    }
#endif
};

EventFD::EventFD()
    : impl(new Impl())
{
}

EventFD::~EventFD() noexcept {
    delete impl;
}

int EventFD::fd() const noexcept {
    return impl->fd();
}

void EventFD::signal() {
    impl->signal();
}

void EventFD::drain() {
    impl->drain();
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

void FD::setNonBlocking() {
    if (::fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        Errno().raise(StringBuilder() << StringView(u8"fcntl(O_NONBLOCK) failed"));
    }
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

ScopedFD::~ScopedFD() {
    close();
}

FD ScopedFD::release() noexcept {
    FD tmp;
    xchg(tmp);
    return tmp;
}

void stl::createPipeFD(ScopedFD& in, ScopedFD& out) {
    int fd[2];

    if (auto res = pipe(fd); res < 0) {
        Errno().raise(StringBuilder() << StringView(u8"pipe() failed"));
    }

    ScopedFD(fd[0]).xchg(in);
    ScopedFD(fd[1]).xchg(out);
}
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
./std/sys/mem_fd.cpp
#include "mem_fd.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>

#if defined(__linux__)
    #include <sys/mman.h>
#else
    #include <unistd.h>
    #include <stdlib.h>
#endif

using namespace stl;

namespace {
#if !defined(__linux__)
    int memFDFallback(const char* name) {
        (void)name;

        const char* dir = getenv("TMPDIR");

        if (!dir) {
            dir = "/tmp";
        }

        StringBuilder sb;
        sb << StringView(dir) << StringView(u8"/memfd.XXXXXX");

        int fd = mkstemp(sb.cStr());

        if (fd < 0) {
            Errno().raise(StringBuilder() << StringView(u8"mkstemp() failed"));
        }

        unlink(sb.cStr());

        return fd;
    }
#endif
}

int stl::memFD(const char* name) {
#if defined(__linux__)
    int fd = memfd_create(name, 0);

    if (fd < 0) {
        Errno().raise(StringBuilder() << StringView(u8"memfd_create() failed"));
    }

    return fd;
#else
    return memFDFallback(name);
#endif
}
./std/sys/num_cpu.cpp
#include "num_cpu.h"

#include <std/str/view.h>
#include <std/lib/buffer.h>
#include <std/ios/fs_utils.h>

#include <stdlib.h>

#if defined(__linux__)
    #include <sched.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
    #include <sys/sysctl.h>
#endif

using namespace stl;

namespace {
    static u32 fromEnv() noexcept {
        if (auto v = getenv("GOMAXPROCS")) {
            auto n = StringView(v).stou();

            if (n > 0) {
                return (u32)n;
            }
        }

        return 0;
    }

#if defined(__linux__)
    static u32 readCgroupVal(const char* path) {
        Buffer p{StringView(path)};
        Buffer buf;

        readFileContent(p, buf);

        return (u32)StringView(buf).stripCr().stou();
    }

    static u32 fromCgroupV2() {
        Buffer p(StringView("/sys/fs/cgroup/cpu.max"));
        Buffer buf;

        readFileContent(p, buf);

        StringView line(buf);
        StringView quotaStr, periodStr;

        if (line.stripCr().split(' ', quotaStr, periodStr) && quotaStr != StringView("max")) {
            auto quota = quotaStr.stou();
            auto period = periodStr.stou();

            if (quota > 0 && period > 0) {
                return (u32)((quota + period - 1) / period);
            }
        }

        return 0;
    }

    static u32 fromCgroupV1() {
        auto quota = readCgroupVal("/sys/fs/cgroup/cpu/cpu.cfs_quota_us");
        auto period = readCgroupVal("/sys/fs/cgroup/cpu/cpu.cfs_period_us");

        if (!period) {
            period = 100000;
        }

        if (quota > 0 && period > 0) {
            return (u32)((quota + period - 1) / period);
        }

        return 0;
    }

    static u32 fromOs() noexcept {
        cpu_set_t set;

        if (sched_getaffinity(0, sizeof(set), &set) == 0) {
            return (u32)CPU_COUNT(&set);
        }

        return 1;
    }
#elif defined(__APPLE__) || defined(__FreeBSD__)
    static u32 fromOs() noexcept {
        int n = 0;
        size_t len = sizeof(n);

        if (sysctlbyname("hw.ncpu", &n, &len, nullptr, 0) == 0 && n > 0) {
            return (u32)n;
        }

        return 1;
    }
#else
    static u32 fromOs() noexcept {
        return 1;
    }
#endif
}

u32 stl::numCpu() noexcept {
    if (auto n = fromEnv()) {
        return n;
    }

#if defined(__linux__)
    try {
        if (auto n = fromCgroupV2()) {
            return n;
        }
    } catch (...) {
    }

    try {
        if (auto n = fromCgroupV1()) {
            return n;
        }
    } catch (...) {
    }
#endif

    return fromOs();
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
    struct VerifyError: public Exception {
        const u8* what_;
        u32 line_;
        const u8* file_;
        Buffer full;

        VerifyError(const u8* what, u32 line, const u8* file) noexcept
            : what_(what)
            , line_(line)
            , file_(file)
        {
        }

        ExceptionKind kind() const noexcept override {
            return ExceptionKind::Verify;
        }

        StringView description() override {
            if (!full.empty()) {
                return full;
            }

            (StringBuilder()
             << StringView((const char*)file_)
             << StringView(u8":")
             << line_
             << StringView(u8": verify failed: ")
             << StringView((const char*)what_))
                .xchg(full);

            return full;
        }
    };

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

void stl::raiseVerify(const u8* what, u32 line, const u8* file) {
    throw VerifyError(what, line, file);
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

Errno::Errno(int error) noexcept
    : error(error)
{
}

void Errno::raise(Buffer&& text) {
    throw ErrnoError(error, move(text));
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
./std/thr/async.cpp
#include "async.h"
#include "coro.h"
#include "pool.h"
#include "thread.h"
#include "semaphore.h"

#include <std/ptr/arc.h>
#include <std/mem/obj_pool.h>
#include <std/alg/exchange.h>

using namespace stl;

namespace {
    struct FutureImpl: public FutureIface {
        ARC arc;
        ObjPool::Ref semPool;
        Semaphore* sem;
        void* value;
        ProducerIface* prod;

        FutureImpl(ProducerIface* p) noexcept
            : semPool(ObjPool::fromMemory())
            , sem(Semaphore::create(semPool.mutPtr(), 0))
            , value(nullptr)
            , prod(p)
        {
        }

        FutureImpl(CoroExecutor* exec, ProducerIface* p) noexcept
            : semPool(ObjPool::fromMemory())
            , sem(Semaphore::create(semPool.mutPtr(), 0, exec))
            , value(nullptr)
            , prod(p)
        {
        }

        ~FutureImpl() noexcept override {
            prod->del(value);
            delete prod;
        }

        i32 ref() noexcept override {
            return arc.ref();
        }

        i32 unref() noexcept override {
            return arc.unref();
        }

        i32 refCount() const noexcept override {
            return arc.refCount();
        }

        void* wait() noexcept override {
            sem->wait();
            return value;
        }

        void* posted() noexcept override {
            return value;
        }

        void* release() noexcept override {
            return exchange(value, nullptr);
        }

        void execute() {
            value = prod->run();
            sem->post();
        }
    };
}

FutureIfaceRef stl::asyncImpl(ProducerIface* prod) {
    auto fi = makeIntrusivePtr(new FutureImpl(prod));
    auto pool = ObjPool::fromMemory();

    auto r = makeRunablePtr([fi, pool] mutable {
        fi->execute();
    });

    Thread::create(pool.mutPtr(), *r);

    return fi.mutPtr();
}

FutureIfaceRef stl::asyncImpl(ProducerIface* prod, ThreadPool* pool) {
    auto fi = makeIntrusivePtr(new FutureImpl(prod));

    pool->submit([fi] mutable {
        fi->execute();
    });

    return fi.mutPtr();
}

FutureIfaceRef stl::asyncImpl(ProducerIface* prod, CoroExecutor* exec) {
    auto fi = makeIntrusivePtr(exec->currentCoroId() ? new FutureImpl(exec, prod) : new FutureImpl(prod));

    exec->spawn([fi] mutable {
        fi->execute();
    });

    return fi.mutPtr();
}
./std/thr/channel.cpp
#include "channel.h"
#include "event.h"
#include "mutex.h"
#include "guard.h"

#include <std/sys/crt.h>
#include <std/alg/defer.h>
#include <std/mem/obj_pool.h>
#include <std/lib/list.h>
#include <std/dbg/insist.h>
#include <std/alg/bits.h>

using namespace stl;

namespace {
    struct Waiter: public IntrusiveNode {
        Event* ev;
        void* value;
        bool valueSet;
    };

    struct ChannelImpl: public Channel {
        CoroExecutor* exec_;
        Mutex* mu_;
        IntrusiveList senders_;
        IntrusiveList receivers_;
        bool closed_;

        ChannelImpl(Mutex* mu) noexcept
            : exec_(nullptr)
            , mu_(mu)
            , closed_(false)
        {
        }

        ChannelImpl(CoroExecutor* exec, Mutex* mu) noexcept
            : exec_(exec)
            , mu_(mu)
            , closed_(false)
        {
        }

        bool sendOne(void* v) noexcept;
        bool recvOne(void** out) noexcept;

        virtual bool bufferOne(void*) noexcept {
            return false;
        }

        virtual bool unbufferOne(void**) noexcept {
            return false;
        }

        __attribute__((noinline)) void enqueueSlow(void* v) noexcept;
        __attribute__((noinline)) bool dequeueSlow(void** out) noexcept;

        void enqueue(void* v) noexcept override;
        void enqueue(void** from, size_t len) noexcept override;
        bool dequeue(void** out) noexcept override;
        size_t dequeue(void** to, size_t len) noexcept override;

        bool tryEnqueue(void* v) noexcept override;
        bool tryDequeue(void** out) noexcept override;

        void close() noexcept override;
    };

    struct Pow2RingBuf {
        void** buf_;
        size_t mask_;
        size_t head_;
        size_t size_;

        Pow2RingBuf(void** buf, size_t capa) noexcept;

        bool empty() const noexcept {
            return size_ == 0;
        }

        bool full() const noexcept {
            return size_ == mask_ + 1;
        }

        void push(void* v) noexcept;
        void* pop() noexcept;
    };

    struct BufferedImpl: public ChannelImpl {
        Pow2RingBuf buf_;

        BufferedImpl(Mutex* mu, void** storage, size_t capa) noexcept
            : ChannelImpl(mu)
            , buf_(storage, capa)
        {
        }

        BufferedImpl(CoroExecutor* exec, Mutex* mu, void** storage, size_t capa) noexcept
            : ChannelImpl(exec, mu)
            , buf_(storage, capa)
        {
        }

        bool bufferOne(void* v) noexcept override;
        bool unbufferOne(void** out) noexcept override;
    };
}

bool ChannelImpl::sendOne(void* v) noexcept {
    if (auto w = (Waiter*)receivers_.popFrontOrNull(); w) {
        w->value = v;
        w->valueSet = true;
        w->ev->signal();

        return true;
    }

    return bufferOne(v);
}

void ChannelImpl::enqueue(void* v) noexcept {
    mu_->lock();

    STD_INSIST(!closed_);

    if (sendOne(v)) {
        mu_->unlock();
        return;
    }

    enqueueSlow(v);
}

void ChannelImpl::enqueue(void** from, size_t len) noexcept {
    for (size_t i = 0; i < len;) {
        mu_->lock();

        STD_INSIST(!closed_);

        if (sendOne(from[i])) {
            ++i;

            while (i < len && sendOne(from[i])) {
                ++i;
            }

            mu_->unlock();
        } else {
            enqueueSlow(from[i]);
            ++i;
        }
    }
}

bool ChannelImpl::tryEnqueue(void* v) noexcept {
    LockGuard guard(mu_);

    STD_INSIST(!closed_);

    return sendOne(v);
}

__attribute__((noinline)) void ChannelImpl::enqueueSlow(void* v) noexcept {
    Event::Buf buf;
    auto* ev = Event::create(buf, exec_);
    STD_DEFER {
        delete ev;
    };

    Waiter w;

    w.ev = ev;
    w.value = v;
    w.valueSet = true;

    senders_.pushBack(&w);
    ev->wait(makeRunable([this] {
        mu_->unlock();
    }));
}

bool ChannelImpl::recvOne(void** out) noexcept {
    if (unbufferOne(out)) {
        return true;
    }

    if (auto w = (Waiter*)senders_.popFrontOrNull(); w) {
        *out = w->value;
        w->ev->signal();

        return true;
    }

    return false;
}

bool ChannelImpl::dequeue(void** out) noexcept {
    mu_->lock();

    if (recvOne(out)) {
        mu_->unlock();
        return true;
    }

    if (closed_) {
        mu_->unlock();
        return false;
    }

    return dequeueSlow(out);
}

bool ChannelImpl::tryDequeue(void** out) noexcept {
    LockGuard guard(mu_);

    return recvOne(out);
}

__attribute__((noinline)) bool ChannelImpl::dequeueSlow(void** out) noexcept {
    Event::Buf buf;
    auto* ev = Event::create(buf, exec_);
    STD_DEFER {
        delete ev;
    };

    Waiter w;

    w.ev = ev;
    w.value = nullptr;
    w.valueSet = false;

    receivers_.pushBack(&w);
    ev->wait(makeRunable([this] {
        mu_->unlock();
    }));

    if (w.valueSet) {
        *out = w.value;

        return true;
    }

    return false;
}

size_t ChannelImpl::dequeue(void** to, size_t len) noexcept {
    if (len == 0) {
        return 0;
    }

    // block for the first element
    if (!dequeue(to)) {
        return 0;
    }

    size_t n = 1;

    // drain the rest under one lock
    mu_->lock();

    while (n < len && recvOne(to + n)) {
        ++n;
    }

    mu_->unlock();

    return n;
}

void ChannelImpl::close() noexcept {
    LockGuard guard(mu_);

    STD_INSIST(!closed_);
    STD_INSIST(senders_.empty());

    closed_ = true;

    while (auto w = (Waiter*)receivers_.popFrontOrNull()) {
        w->ev->signal();
    }
}

bool BufferedImpl::bufferOne(void* v) noexcept {
    if (!buf_.full()) {
        buf_.push(v);

        return true;
    }

    return false;
}

bool BufferedImpl::unbufferOne(void** out) noexcept {
    if (!buf_.empty()) {
        *out = buf_.pop();

        if (auto w = (Waiter*)senders_.popFrontOrNull(); w) {
            buf_.push(w->value);
            w->ev->signal();
        }

        return true;
    }

    return false;
}

Pow2RingBuf::Pow2RingBuf(void** buf, size_t capa) noexcept
    : buf_(buf)
    , mask_(capa - 1)
    , head_(0)
    , size_(0)
{
}

void Pow2RingBuf::push(void* v) noexcept {
    buf_[(head_ + size_) & mask_] = v;
    ++size_;
}

void* Pow2RingBuf::pop() noexcept {
    void* v = buf_[head_];
    head_ = (head_ + 1) & mask_;
    --size_;
    return v;
}

Channel* Channel::create(ObjPool* pool) {
    return pool->make<ChannelImpl>(Mutex::create(pool));
}

Channel* Channel::create(ObjPool* pool, size_t cap) {
    if (cap == 0) {
        return create(pool);
    }

    size_t rcap = clp2(cap);
    auto storage = (void**)pool->allocate(rcap * sizeof(void*));

    return pool->make<BufferedImpl>(Mutex::create(pool), storage, rcap);
}

Channel* Channel::create(ObjPool* pool, CoroExecutor* exec) {
    return pool->make<ChannelImpl>(exec, Mutex::createSpinLock(pool, exec));
}

Channel* Channel::create(ObjPool* pool, CoroExecutor* exec, size_t cap) {
    if (cap == 0) {
        return create(pool, exec);
    }

    size_t rcap = clp2(cap);
    auto storage = (void**)pool->allocate(rcap * sizeof(void*));

    return pool->make<BufferedImpl>(exec, Mutex::createSpinLock(pool, exec), storage, rcap);
}
./std/thr/cond_var.cpp
#include "coro.h"
#include "mutex.h"
#include "cond_var.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>
#include <std/mem/obj_pool.h>

#include <pthread.h>

using namespace stl;

namespace {
    struct PosixCondVarImpl: public CondVar, public pthread_cond_t {
        PosixCondVarImpl() {
            if (pthread_cond_init(this, nullptr) != 0) {
                Errno().raise(StringBuilder() << StringView(u8"pthread_cond_init failed"));
            }
        }

        ~PosixCondVarImpl() noexcept {
            STD_INSIST(pthread_cond_destroy(this) == 0);
        }

        void wait(Mutex* mutex) noexcept override {
            STD_INSIST(pthread_cond_wait(this, (pthread_mutex_t*)mutex->nativeHandle()) == 0);
        }

        void signal() noexcept override {
            STD_INSIST(pthread_cond_signal(this) == 0);
        }

        void broadcast() noexcept override {
            STD_INSIST(pthread_cond_broadcast(this) == 0);
        }
    };
}

CondVar* CondVar::create(ObjPool* pool) {
    return pool->make<PosixCondVarImpl>();
}

CondVar* CondVar::create(ObjPool* pool, CoroExecutor* exec) {
    return exec->createCondVar(pool);
}
./std/thr/context.cpp
#include "context.h"

#include <std/mem/new.h>
#include <std/dbg/insist.h>
#include <std/thr/runable.h>
#include <std/alg/exchange.h>

using namespace stl;

namespace {
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
    extern "C" {
        void __sanitizer_start_switch_fiber(void** fakeStackSave, const void* bottom, size_t size);
        void __sanitizer_finish_switch_fiber(void* fakeStackSave, const void** bottomOld, size_t* sizeOld);
    }

    struct AsanFiberState {
        void* fakeStack_ = nullptr;
        AsanFiberState* prev_ = nullptr;
        const void* stackBottom_ = nullptr;
        size_t stackSize_ = 0;

        void initStack(void* ptr, size_t size) noexcept {
            stackBottom_ = ptr;
            stackSize_ = size;
        }

        void beforeSwitch(AsanFiberState& target) noexcept {
            __sanitizer_start_switch_fiber(&fakeStack_, target.stackBottom_, target.stackSize_);
            target.prev_ = this;
        }

        void finishSwitch() noexcept {
            __sanitizer_finish_switch_fiber(fakeStack_, &stackBottom_, &stackSize_);
        }

        void afterSwitch() noexcept {
            prev_->finishSwitch();
        }
    };
#else
    struct AsanFiberState {
        void initStack(void*, size_t) noexcept {
        }

        void beforeSwitch(AsanFiberState&) noexcept {
        }

        void afterSwitch() noexcept {
        }
    };
#endif
}

namespace __cxxabiv1 {
    struct __cxa_eh_globals {
        void* caughtExceptions = nullptr;
        unsigned int uncaughtExceptions = 0;
    };

    extern "C" __cxa_eh_globals* __cxa_get_globals() noexcept;
}

namespace {
    struct alignas(max_align_t) ContextBase: public Context, public Newable {
        static void operator delete(void*) noexcept {
        }
    };
}

#if defined(__x86_64__)
namespace {
    struct ContextImpl: public ContextBase, public AsanFiberState {
        u64 rsp = 0;
        __cxxabiv1::__cxa_eh_globals ehg_;
        Runable* entry_ = nullptr;

        ContextImpl() = default;
        ContextImpl(void* stackPtr, size_t stackSize, Runable& entry) noexcept;

        void switchTo(Context& target) noexcept override;

        __attribute__((naked, noinline)) static void swapContext(u64*, u64*);

        [[noreturn]]
        static void trampoline();
    };
}

void ContextImpl::swapContext(u64*, u64*) {
    __asm__(
        "pushq %rbx\n\t"
        "pushq %rbp\n\t"
        "pushq %r12\n\t"
        "pushq %r13\n\t"
        "pushq %r14\n\t"
        "pushq %r15\n\t"
        "movq %rsp, (%rdi)\n\t"
        "movq (%rsi), %rsp\n\t"
        "popq %r15\n\t"
        "popq %r14\n\t"
        "popq %r13\n\t"
        "popq %r12\n\t"
        "popq %rbp\n\t"
        "popq %rbx\n\t"
        "retq\n\t");
}

__attribute__((no_sanitize("address"))) void ContextImpl::trampoline() {
    ContextImpl* self;

    __asm__ volatile("movq %%rbx, %0" : "=r"(self));

    self->afterSwitch();
    self->entry_->run();

    STD_INSIST(false);
    __builtin_unreachable();
}

ContextImpl::ContextImpl(void* stackPtr, size_t stackSize, Runable& entry) noexcept {
    entry_ = &entry;

    auto top = (u64*)(((uintptr_t)stackPtr + stackSize) & ~(uintptr_t)15);

    // after swapContext pops 6 regs and does ret (7 * 8 = 56 bytes),
    // rsp = top - 8 which is 8-mod-16, matching the ABI for function entry
    *--top = 0;
    *--top = (u64)trampoline;
    *--top = (u64)this; // rbx
    *--top = 0;         // rbp
    *--top = 0;         // r12
    *--top = 0;         // r13
    *--top = 0;         // r14
    *--top = 0;         // r15

    rsp = (u64)top;
    initStack(stackPtr, stackSize);
}

void ContextImpl::switchTo(Context& target) noexcept {
    auto& t = (ContextImpl&)target;
    ehg_ = exchange(*__cxxabiv1::__cxa_get_globals(), t.ehg_);
    beforeSwitch(t);
    swapContext(&rsp, &t.rsp);
    afterSwitch();
}
#elif defined(__aarch64__)
namespace {
    struct ContextImpl: public ContextBase, public AsanFiberState {
        u64 sp_ = 0;
        __cxxabiv1::__cxa_eh_globals ehg_;
        Runable* entry_ = nullptr;

        ContextImpl() = default;
        ContextImpl(void* stackPtr, size_t stackSize, Runable& entry) noexcept;

        void switchTo(Context& target) noexcept override;

        __attribute__((naked, noinline)) static void swapContext(u64*, u64*);

        [[noreturn]]
        static void trampoline();
    };
}

void ContextImpl::swapContext(u64*, u64*) {
    __asm__(
        "stp d8,  d9,  [sp, #-16]!\n\t"
        "stp d10, d11, [sp, #-16]!\n\t"
        "stp d12, d13, [sp, #-16]!\n\t"
        "stp d14, d15, [sp, #-16]!\n\t"
        "stp x19, x20, [sp, #-16]!\n\t"
        "stp x21, x22, [sp, #-16]!\n\t"
        "stp x23, x24, [sp, #-16]!\n\t"
        "stp x25, x26, [sp, #-16]!\n\t"
        "stp x27, x28, [sp, #-16]!\n\t"
        "stp x29, x30, [sp, #-16]!\n\t"
        "mov x2, sp\n\t"
        "str x2, [x0]\n\t"
        "ldr x2, [x1]\n\t"
        "mov sp, x2\n\t"
        "ldp x29, x30, [sp], #16\n\t"
        "ldp x27, x28, [sp], #16\n\t"
        "ldp x25, x26, [sp], #16\n\t"
        "ldp x23, x24, [sp], #16\n\t"
        "ldp x21, x22, [sp], #16\n\t"
        "ldp x19, x20, [sp], #16\n\t"
        "ldp d14, d15, [sp], #16\n\t"
        "ldp d12, d13, [sp], #16\n\t"
        "ldp d10, d11, [sp], #16\n\t"
        "ldp d8,  d9,  [sp], #16\n\t"
        "ret\n\t");
}

__attribute__((no_sanitize("address"))) void ContextImpl::trampoline() {
    ContextImpl* self;

    __asm__ volatile("mov %0, x19" : "=r"(self));

    self->afterSwitch();
    self->entry_->run();

    STD_INSIST(false);
    __builtin_unreachable();
}

ContextImpl::ContextImpl(void* stackPtr, size_t stackSize, Runable& entry) noexcept {
    entry_ = &entry;

    auto top = (u64*)(((uintptr_t)stackPtr + stackSize) & ~(uintptr_t)15);

    // *--top fills high-to-low; ldp pops low-to-high
    // last pushed = lowest address = first popped
    // d8, d9 (popped last, filled first)
    *--top = 0;               // d9
    *--top = 0;               // d8
    *--top = 0;               // d11
    *--top = 0;               // d10
    *--top = 0;               // d13
    *--top = 0;               // d12
    *--top = 0;               // d15
    *--top = 0;               // d14
    *--top = 0;               // x20
    *--top = (u64)this;       // x19 — ContextImpl*
    *--top = 0;               // x22
    *--top = 0;               // x21
    *--top = 0;               // x24
    *--top = 0;               // x23
    *--top = 0;               // x26
    *--top = 0;               // x25
    *--top = 0;               // x28
    *--top = 0;               // x27
    *--top = (u64)trampoline; // x30 (lr)
    *--top = 0;               // x29 (fp)

    sp_ = (u64)top;
    initStack(stackPtr, stackSize);
}

void ContextImpl::switchTo(Context& target) noexcept {
    auto& t = (ContextImpl&)target;
    ehg_ = exchange(*__cxxabiv1::__cxa_get_globals(), t.ehg_);
    beforeSwitch(t);
    swapContext(&sp_, &t.sp_);
    afterSwitch();
}
#else
    #include <ucontext.h>

namespace {
    struct ContextImpl: public ContextBase {
        ucontext_t uctx;
        __cxxabiv1::__cxa_eh_globals ehg_;

        ContextImpl() noexcept;
        ContextImpl(void* stackPtr, size_t stackSize, Runable& entry) noexcept;

        void switchTo(Context& target) noexcept override;

        static void trampoline(u32 lo, u32 hi);
    };
}

ContextImpl::ContextImpl() noexcept {
}

ContextImpl::ContextImpl(void* stackPtr, size_t stackSize, Runable& entry) noexcept {
    getcontext(&uctx);

    uctx.uc_stack.ss_sp = stackPtr;
    uctx.uc_stack.ss_size = stackSize;
    uctx.uc_link = nullptr;

    auto p = (uintptr_t)&entry;

    makecontext(&uctx, (void (*)())trampoline, 2, (u32)p, (u32)(p >> 32));
}

void ContextImpl::trampoline(u32 lo, u32 hi) {
    ((Runable*)(uintptr_t)(((uintptr_t)hi << 32) | lo))->run();
}

void ContextImpl::switchTo(Context& target) noexcept {
    auto& t = (ContextImpl&)target;
    ehg_ = exchange(*__cxxabiv1::__cxa_get_globals(), t.ehg_);
    swapcontext(&uctx, &t.uctx);
}
#endif

size_t Context::implSize() noexcept {
    static_assert(sizeof(ContextImpl) % alignof(max_align_t) == 0);

    return sizeof(ContextImpl);
}

Context* Context::create(void* buf) noexcept {
    return new (buf) ContextImpl();
}

Context* Context::create(void* buf, void* stackPtr, size_t stackSize, Runable& entry) noexcept {
    return new (buf) ContextImpl(stackPtr, stackSize, entry);
}

Context::~Context() {
}
./std/thr/coro.cpp
#include "coro.h"
#include "task.h"
#include "pool.h"

#include <std/lib/vector.h>
#include "mutex.h"
#include "guard.h"
#include "thread.h"
#include "context.h"
#include "cond_var.h"
#include "semaphore.h"
#include "event.h"
#include "io_reactor.h"

#include <std/sys/fd.h>
#include <std/sys/crt.h>
#include <std/mem/new.h>
#include <std/rng/pcg.h>
#include <std/lib/list.h>
#include <std/alg/defer.h>
#include <std/alg/range.h>
#include <std/sys/atomic.h>
#include <std/dbg/insist.h>
#include <std/ptr/scoped.h>
#include <std/alg/minmax.h>
#include <std/dbg/assert.h>
#include <std/alg/exchange.h>
#include <std/alg/destruct.h>
#include <std/mem/obj_pool.h>

#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sched.h>
#include <alloca.h>
#include <unistd.h>

using namespace stl;

namespace {
    struct CoroExecutorImpl;

    struct ContImpl: public Task {
        CoroExecutorImpl* exec_;
        ScopedPtr<Context> ctx_;
        Context* workerCtx_;
        Runable* runable_;

        ContImpl(CoroExecutorImpl* exec, void* ctxBuf, SpawnParams params) noexcept;
        virtual ~ContImpl();

        void operator delete(void* p) noexcept {
            freeMemory(p);
        }

        void parkWith(Runable* afterSuspend) noexcept;

        void parkWith(Runable&& afterSuspend) noexcept {
            parkWith(&afterSuspend);
        }

        template <typename F>
        void park(F f) noexcept {
            parkWith(makeRunable(f));
        }

        void run() noexcept override;
        void reSchedule() noexcept;
    };

    struct alignas(max_align_t) HeapContImpl final: public ContImpl {
        void* operator new(size_t, size_t stackSize) {
            return allocateMemory(sizeof(HeapContImpl) + Context::implSize() + stackSize);
        }

        HeapContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept
            : ContImpl(exec, (char*)(this + 1), params.setStackPtr((char*)(this + 1) + Context::implSize()))
        {
        }
    };

    struct alignas(max_align_t) ExtStackContImpl final: public ContImpl {
        void* operator new(size_t sz) {
            return allocateMemory(sz + Context::implSize());
        }

        ExtStackContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept
            : ContImpl(exec, (char*)(this + 1), params)
        {
        }
    };

    ContImpl* makeContImpl(CoroExecutorImpl* exec, SpawnParams params) {
        if (params.stackPtr) {
            return new ExtStackContImpl(exec, params);
        } else {
            return new (params.stackSize) HeapContImpl(exec, params);
        }
    }

    struct JoinPipe {
        ScopedFD r;
        ScopedFD w;

        JoinPipe() {
            createPipeFD(r, w);
            w.setNonBlocking();
        }
    };

    struct CoroExecutorImpl: public CoroExecutor {
        alignas(64) int inflight_ = 0;
        JoinPipe* join_;
        Vector<ContImpl*> conts_;
        IoReactor* io_;
        ThreadPool* pool_;

        CoroExecutorImpl(ObjPool* pool, size_t threads);
        ~CoroExecutorImpl() noexcept;

        void join() noexcept override;
        void spawnRun(SpawnParams params) override;

        ContImpl** contSlot() {
            size_t id;

            if (pool_->workerId(&id)) {
                return &conts_.mutData()[id];
            }

            return nullptr;
        }

        ContImpl* currentCont() {
            return *contSlot();
        }

        void* currentCoroId() const noexcept override {
            size_t id;

            if (((CoroExecutorImpl*)this)->pool_->workerId(&id)) {
                return ((CoroExecutorImpl*)this)->conts_[id];
            }

            return nullptr;
        }

        void yield() noexcept override;

        Event* createEvent(void* buf) override;
        Thread* createThread(ObjPool* pool) override;
        CondVar* createCondVar(ObjPool* pool) override;
        Mutex* createSemaphoreImpl(ObjPool* pool, size_t initial) override;

        IoReactor* io() noexcept override {
            return io_;
        }

        ThreadPool* pool() noexcept override {
            return pool_;
        }

        void parkWith(Runable&&, Task**) noexcept override;
        void offloadRun(ThreadPool* pool, Runable&& work) override;
    };
}

CoroExecutorImpl::CoroExecutorImpl(ObjPool* pool, size_t threads)
    : join_(pool->make<JoinPipe>())
    , io_(IoReactor::create(pool, this, threads))
    , pool_(ThreadPool::workStealing(pool, threads, io_->hooks()))
{
    for (size_t i = 0; i < threads; ++i) {
        conts_.pushBack(nullptr);
    }
}

void CoroExecutorImpl::spawnRun(SpawnParams params) {
    makeContImpl(this, params)->reSchedule();
}

void CoroExecutorImpl::yield() noexcept {
    auto c = currentCont();

    c->park([c] {
        c->reSchedule();
    });
}

CoroExecutorImpl::~CoroExecutorImpl() noexcept {
    join();
}

void CoroExecutorImpl::join() noexcept {
    while (stdAtomicFetch(&inflight_, MemoryOrder::Acquire) > 0) {
        char b;

        join_->r.read(&b, 1);
    }
}

ContImpl::ContImpl(CoroExecutorImpl* exec, void* ctxBuf, SpawnParams params) noexcept
    : exec_(exec)
    , ctx_{Context::create(ctxBuf, params.stackPtr, params.stackSize, *this)}
    , workerCtx_(nullptr)
    , runable_(params.runable)
{
    stdAtomicAddAndFetch(&exec_->inflight_, 1, MemoryOrder::Relaxed);
}

ContImpl::~ContImpl() {
    if (stdAtomicAddAndFetch(&exec_->inflight_, -1, MemoryOrder::Release) == 0) {
        char b = 1;

        exec_->join_->w.write(&b, 1);
    }
}

void ContImpl::reSchedule() noexcept {
    STD_ASSERT(!workerCtx_);
    exec_->pool_->submitTask(this);
}

void ContImpl::parkWith(Runable* afterSuspend) noexcept {
    runable_ = afterSuspend;
    STD_ASSERT(exec_->currentCont() == this);
    ctx_->switchTo(*workerCtx_);
}

void ContImpl::run() noexcept {
    if (workerCtx_) {
        exchange(runable_, nullptr)->run();
        ctx_->switchTo(*workerCtx_);

        return;
    }

    auto slot = exec_->contSlot();

    *slot = this;
    (workerCtx_ = Context::create(alloca(Context::implSize())))->switchTo(*ctx_.ptr);
    delete workerCtx_;
    workerCtx_ = nullptr;
    *slot = nullptr;

    if (auto as = runable_; as) {
        runable_ = nullptr;
        return as->run();
    }

    delete this;
}

SpawnParams::SpawnParams() noexcept
    : stackSize(16 * 1024)
    , stackPtr(nullptr)
    , runable(nullptr)
{
}

void CoroExecutorImpl::parkWith(Runable&& afterSuspend, Task** out) noexcept {
    auto cont = currentCont();
    *out = cont;
    cont->parkWith(&afterSuspend);
}

void CoroExecutorImpl::offloadRun(ThreadPool* pool, Runable&& work) {
    auto cont = currentCont();

    cont->park([pool, cont, &work] {
        pool->submit([cont, &work] {
            work.run();
            cont->reSchedule();
        });
    });
}

Event* CoroExecutorImpl::createEvent(void* buf) {
    struct CoroEventImpl: public Event, public Newable {
        CoroExecutorImpl* exec_;
        ContImpl* waiter_;

        CoroEventImpl(CoroExecutorImpl* exec) noexcept
            : exec_(exec)
            , waiter_(nullptr)
        {
        }

        void wait(Runable& cb) noexcept override {
            (waiter_ = exec_->currentCont())->parkWith(&cb);
        }

        void signal() noexcept override {
            waiter_->reSchedule();
        }

        static void operator delete(void*) noexcept {
        }
    };

    return new (buf) CoroEventImpl(this);
}

CondVar* CoroExecutorImpl::createCondVar(ObjPool* pool) {
    struct CoroCondVarImpl: public CondVar {
        Mutex* queueMutex_;
        IntrusiveList waiters_;

        CoroCondVarImpl(ObjPool* pool, CoroExecutorImpl* exec) noexcept
            : queueMutex_(Mutex::createSpinLock(pool, exec))
        {
        }

        CoroExecutorImpl* exec() noexcept {
            return (CoroExecutorImpl*)queueMutex_->nativeHandle();
        }

        void wait(Mutex* mutex) noexcept override {
            queueMutex_->lock();
            auto cont = exec()->currentCont();
            waiters_.pushBack(cont);
            cont->park([&] {
                queueMutex_->unlock();
                mutex->unlock();
            });
            mutex->lock();
        }

        void signal() noexcept override {
            auto node = LockGuard(queueMutex_).run([&] {
                return (ContImpl*)(Task*)waiters_.popFrontOrNull();
            });

            if (node) {
                node->reSchedule();
            }
        }

        void broadcast() noexcept override {
            IntrusiveList tmp;

            LockGuard(queueMutex_).run([&] {
                tmp.xchg(waiters_);
            });

            while (auto node = (ContImpl*)(Task*)tmp.popFrontOrNull()) {
                node->reSchedule();
            }
        }
    };

    return pool->make<CoroCondVarImpl>(pool, this);
}

Thread* CoroExecutorImpl::createThread(ObjPool* pool) {
    struct CoroThreadImpl: public Thread {
        Semaphore* sem_;

        CoroThreadImpl(Semaphore* sem) noexcept
            : sem_(sem)
        {
        }

        void start(Runable& runable) override {
            ((CoroExecutor*)sem_->nativeHandle())->spawn([this, &runable] {
                runable.run();
                sem_->post();
            });
        }

        void join() noexcept override {
            sem_->wait();
        }

        u64 threadId() const noexcept override {
            return (u64)(size_t)this;
        }
    };

    return pool->make<CoroThreadImpl>(Semaphore::create(pool, 0, this));
}

Mutex* CoroExecutorImpl::createSemaphoreImpl(ObjPool* pool, size_t initial) {
    struct CoroSemaphoreImpl: public Mutex {
        Mutex* lock_;
        IntrusiveList waiters_;
        size_t count_;

        CoroSemaphoreImpl(Mutex* lock, size_t initial) noexcept
            : lock_(lock)
            , count_(initial)
        {
        }

        void post() noexcept override {
            lock_->lock();

            if (auto cont = (ContImpl*)(Task*)waiters_.popFrontOrNull(); cont) {
                lock_->unlock();
                cont->reSchedule();
            } else {
                ++count_;
                lock_->unlock();
            }
        }

        void wait() noexcept override {
            lock_->lock();

            if (count_ > 0) {
                --count_;
                lock_->unlock();
                return;
            }

            auto cont = ((CoroExecutorImpl*)(CoroExecutor*)nativeHandle())->currentCont();
            waiters_.pushBack(cont);
            cont->park([&] {
                lock_->unlock();
            });
        }

        void* nativeHandle() noexcept override {
            return lock_->nativeHandle();
        }

        bool tryWait() noexcept override {
            lock_->lock();

            if (count_ > 0) {
                --count_;
                lock_->unlock();

                return true;
            }

            lock_->unlock();

            return false;
        }
    };

    return pool->make<CoroSemaphoreImpl>(Mutex::createSpinLock(pool, this), initial);
}

Semaphore* CoroExecutor::createSemaphore(ObjPool* pool, size_t initial) {
    return createSemaphoreImpl(pool, initial);
}

Mutex* CoroExecutor::createMutex(ObjPool* pool) {
    return createSemaphoreImpl(pool, 1);
}

CoroExecutor* CoroExecutor::create(ObjPool* pool, size_t threads) {
    return pool->make<CoroExecutorImpl>(pool, threads);
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

SpawnParams& SpawnParams::setStack(void* v, size_t len) noexcept {
    return setStackPtr(v).setStackSize(len);
}

SpawnParams& SpawnParams::setStack(ObjPool* v, size_t len) noexcept {
    return setStack(v->allocate(len), len);
}
./std/thr/event.cpp
#include "event.h"
#include "coro.h"
#include "guard.h"
#include "mutex.h"
#include "cond_var.h"

#include <std/mem/new.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

#ifdef __linux__
    #include <unistd.h>
    #include <limits.h>
    #include <sys/syscall.h>
    #include <linux/futex.h>
#endif

using namespace stl;

Event::~Event() noexcept {
}

namespace {
#ifdef __linux__
    struct alignas(64) FutexEventImpl: public Event, public Newable {
        u32 state_;

        FutexEventImpl() noexcept
            : state_(0)
        {
        }

        void wait(Runable& cb) noexcept override {
            u32 cur = stdAtomicFetch(&state_, MemoryOrder::Acquire);
            cb.run();

            while (cur == 0) {
                syscall(SYS_futex, &state_, FUTEX_WAIT, 0, nullptr, nullptr, 0);
                cur = stdAtomicFetch(&state_, MemoryOrder::Acquire);
            }
        }

        void signal() noexcept override {
            stdAtomicStore(&state_, (u32)1, MemoryOrder::Release);
            syscall(SYS_futex, &state_, FUTEX_WAKE, 1, nullptr, nullptr, 0);
        }

        static void operator delete(void*) noexcept {
        }
    };

    using DefaultEventImpl = FutexEventImpl;
#else
    struct PosixEventImpl: public Event, public Newable {
        ObjPool::Ref pool_;
        Mutex* mu_;
        CondVar* cv_;
        bool signaled_;

        PosixEventImpl() noexcept
            : pool_(ObjPool::fromMemory())
            , mu_(Mutex::create(pool_.mutPtr()))
            , cv_(CondVar::create(pool_.mutPtr()))
            , signaled_(false)
        {
        }

        void wait(Runable& cb) noexcept override {
            mu_->lock();
            cb.run();

            while (!signaled_) {
                cv_->wait(mu_);
            }

            mu_->unlock();
        }

        void signal() noexcept override {
            LockGuard guard(mu_);
            signaled_ = true;
            cv_->signal();
        }

        static void operator delete(void*) noexcept {
        }
    };

    using DefaultEventImpl = PosixEventImpl;
#endif
}

Event* Event::create(Buf& buf) {
    return new (buf.data) DefaultEventImpl();
}

Event* Event::create(Buf& buf, CoroExecutor* exec) {
    if (exec) {
        return exec->createEvent(&buf);
    }

    return new (buf.data) DefaultEventImpl();
}
./std/thr/future.cpp
#include "future.h"
./std/thr/future_iface.cpp
#include "future_iface.h"

using namespace stl;

FutureIface::~FutureIface() noexcept {
}
./std/thr/guard.cpp
#include "guard.h"
#include "mutex.h"

using namespace stl;

LockGuard::LockGuard(Mutex* mutex)
    : mutex_(mutex)
{
    mutex_->lock();
}

LockGuard::~LockGuard() noexcept {
    if (mutex_) {
        mutex_->unlock();
    }
}

UnlockGuard::UnlockGuard(Mutex* mutex)
    : mutex_(mutex)
{
    mutex_->unlock();
}

UnlockGuard::~UnlockGuard() noexcept {
    mutex_->lock();
}
./std/thr/io_reactor.cpp
#include "io_reactor.h"
#include "io_uring.h"
#include "io_classic.h"

#include <stdlib.h>

using namespace stl;

IoReactor* IoReactor::create(ObjPool* pool, CoroExecutor* exec, size_t threads) {
    if (getenv("USE_POLL_POLLER")) {
        return createPollIoReactor(pool, exec, threads);
    }

    if (auto io = createIoUringReactor(pool, exec, threads); io) {
        return io;
    } else {
        return createPollIoReactor(pool, exec, threads);
    }
}
./std/thr/mutex.cpp
#include "mutex.h"
#include "coro.h"

#include <std/str/view.h>
#include <std/mem/obj_pool.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

#include <pthread.h>
#include <sched.h>

using namespace stl;

namespace {
    struct alignas(64) SpinMutexImpl: public Mutex {
        CoroExecutor* exec_;
        char flag_ = 0;

        SpinMutexImpl(CoroExecutor* exec) noexcept
            : exec_(exec)
        {
        }

        void spin() noexcept {
            if (exec_ && exec_->currentCoroId()) {
                exec_->yield();
            } else {
                sched_yield();
            }
        }

        void* nativeHandle() noexcept override {
            return exec_;
        }

        void wait() noexcept override {
            for (;;) {
                if (!__atomic_test_and_set(&flag_, __ATOMIC_ACQUIRE)) {
                    return;
                }

                while (__atomic_load_n(&flag_, __ATOMIC_RELAXED)) {
                    spin();
                }
            }
        }

        void post() noexcept override {
            __atomic_clear(&flag_, __ATOMIC_RELEASE);
        }

        bool tryWait() noexcept override {
            return !__atomic_test_and_set(&flag_, __ATOMIC_ACQUIRE);
        }
    };

    struct PosixMutexImpl: public Mutex, public pthread_mutex_t {
        PosixMutexImpl() {
            if (pthread_mutex_init(this, nullptr) != 0) {
                Errno().raise(StringBuilder() << StringView(u8"pthread_mutex_init failed"));
            }
        }

        ~PosixMutexImpl() noexcept {
            STD_INSIST(pthread_mutex_destroy(this) == 0);
        }

        void wait() noexcept override {
            STD_INSIST(pthread_mutex_lock(this) == 0);
        }

        void post() noexcept override {
            STD_INSIST(pthread_mutex_unlock(this) == 0);
        }

        bool tryWait() noexcept override {
            return pthread_mutex_trylock(this) == 0;
        }

        void* nativeHandle() noexcept override {
            return static_cast<pthread_mutex_t*>(this);
        }
    };
}

Mutex* Mutex::create(ObjPool* pool) {
    return pool->make<PosixMutexImpl>();
}

Mutex* Mutex::create(ObjPool* pool, CoroExecutor* exec) {
    return exec->createMutex(pool);
}

Mutex* Mutex::createSpinLock(ObjPool* pool) {
    return pool->make<SpinMutexImpl>(nullptr);
}

Mutex* Mutex::createSpinLock(ObjPool* pool, CoroExecutor* exec) {
    return pool->make<SpinMutexImpl>(exec);
}
./std/thr/parker.cpp
#include "parker.h"

#include <std/sys/atomic.h>

using namespace stl;

Parker::Parker()
    : sleeping_(false)
{
}

Parker::~Parker() noexcept {
}

int Parker::fd() const noexcept {
    return ev_.fd();
}

void Parker::drain() noexcept {
    ev_.drain();
}

void Parker::unpark() noexcept {
    bool expected = true;

    if (stdAtomicCAS(&sleeping_, &expected, false, MemoryOrder::Acquire, MemoryOrder::Relaxed)) {
        signal();
    }
}

void Parker::signal() noexcept {
    ev_.signal();
}

void Parker::parkEnter() noexcept {
    stdAtomicStore(&sleeping_, true, MemoryOrder::Release);
}

void Parker::parkLeave() noexcept {
    stdAtomicStore(&sleeping_, false, MemoryOrder::Relaxed);
}
./std/thr/poll_fd.cpp
#include "poll_fd.h"

#include <poll.h>

using namespace stl;

#if defined(__linux__)
static_assert(PollFlag::In == POLLIN);
static_assert(PollFlag::Out == POLLOUT);
static_assert(PollFlag::Err == POLLERR);
static_assert(PollFlag::Hup == POLLHUP);

short PollFD::toPollEvents() const noexcept {
    return (short)flags;
}

u32 PollFD::fromPollEvents(short events) noexcept {
    return (u32)events & (PollFlag::In | PollFlag::Out | PollFlag::Err | PollFlag::Hup);
}
#else
short PollFD::toPollEvents() const noexcept {
    short r = 0;

    if (flags & PollFlag::In) {
        r |= POLLIN;
    }

    if (flags & PollFlag::Out) {
        r |= POLLOUT;
    }

    if (flags & PollFlag::Err) {
        r |= POLLERR;
    }

    if (flags & PollFlag::Hup) {
        r |= POLLHUP;
    }

    return r;
}

u32 PollFD::fromPollEvents(short events) noexcept {
    u32 r = 0;

    if (events & POLLIN) {
        r |= PollFlag::In;
    }

    if (events & POLLOUT) {
        r |= PollFlag::Out;
    }

    if (events & POLLERR) {
        r |= PollFlag::Err;
    }

    if (events & POLLHUP) {
        r |= PollFlag::Hup;
    }

    return r;
}
#endif
./std/thr/pollable.cpp
#include "pollable.h"
./std/thr/poller.cpp
#include "poller.h"
#include "poll_fd.h"
#include "pollable.h"

#include <std/sys/fd.h>
#include <std/sys/crt.h>
#include <std/sys/types.h>
#include <std/alg/range.h>
#include <std/sym/i_map.h>
#include <std/lib/vector.h>
#include <std/dbg/insist.h>
#include <std/alg/minmax.h>
#include <std/mem/obj_pool.h>

#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#if defined(__linux__)
    #include <sys/epoll.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
    #include <sys/event.h>
    #include <sys/time.h>
#endif

#if defined(_WIN32)
    #include <winsock2.h>
    #define STD_POLL WSAPoll
#else
    #define STD_POLL poll
#endif

using namespace stl;

#if defined(__linux__)
namespace {
    struct EpollPoller: public WaitablePoller {
        int epfd_;

        EpollPoller() {
            epfd_ = epoll_create1(0);
            STD_INSIST(epfd_ >= 0);
        }

        ~EpollPoller() noexcept {
            ::close(epfd_);
        }

        void arm(PollFD pfd) override {
            epoll_event ev{};

            ev.data.fd = pfd.fd;
            ev.events = pfd.toPollEvents() | EPOLLONESHOT;

            if (epoll_ctl(epfd_, EPOLL_CTL_MOD, pfd.fd, &ev) < 0) {
                STD_INSIST(errno == ENOENT);
                STD_INSIST(epoll_ctl(epfd_, EPOLL_CTL_ADD, pfd.fd, &ev) == 0);
            }
        }

        void disarm(int fd) override {
            epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
        }

        int fd() override {
            return epfd_;
        }

        void waitImpl(VisitorFace& v, u32 timeoutUs) override {
            epoll_event raw[64];

            int n = epoll_wait(epfd_, raw, sizeof(raw) / sizeof(raw[0]), (int)((timeoutUs + 999) / 1000));

            if (n <= 0) {
                return;
            }

            for (auto& e : range(raw, raw + n)) {
                PollFD ev{e.data.fd, PollFD::fromPollEvents(e.events)};

                v.visit(&ev);
            }
        }
    };
}
#endif

#if defined(__APPLE__) || defined(__FreeBSD__)
namespace {
    struct KqueuePoller: public WaitablePoller {
        int kqfd_;

        KqueuePoller() {
            kqfd_ = kqueue();
            STD_INSIST(kqfd_ >= 0);
        }

        ~KqueuePoller() noexcept {
            ::close(kqfd_);
        }

        void arm(PollFD pfd) override {
            struct kevent changes[2];

            int n = 0;

            if (pfd.flags & PollFlag::In) {
                EV_SET(&changes[n++], pfd.fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, 0);
            }

            if (pfd.flags & PollFlag::Out) {
                EV_SET(&changes[n++], pfd.fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, 0);
            }

            if (n > 0) {
                STD_INSIST(kevent(kqfd_, changes, n, nullptr, 0, nullptr) == 0);
            }
        }

        void disarm(int fd) override {
            struct kevent changes[2];

            EV_SET(&changes[0], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
            EV_SET(&changes[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);

            kevent(kqfd_, changes, 2, nullptr, 0, nullptr);
        }

        void waitImpl(VisitorFace& v, u32 timeoutUs) override {
            struct kevent raw[64];

            struct timespec ts;

            ts.tv_sec = timeoutUs / 1000000;
            ts.tv_nsec = (timeoutUs % 1000000) * 1000L;

            int n = kevent(kqfd_, nullptr, 0, raw, sizeof(raw) / sizeof(raw[0]), &ts);

            if (n <= 0) {
                return;
            }

            for (auto& e : range(raw, raw + n)) {
                u32 fl = 0;

                if (e.filter == EVFILT_READ) {
                    fl |= PollFlag::In;
                }

                if (e.filter == EVFILT_WRITE) {
                    fl |= PollFlag::Out;
                }

                if (e.flags & EV_EOF) {
                    fl |= PollFlag::Hup;
                }

                if (e.flags & EV_ERROR) {
                    fl |= PollFlag::Err;
                }

                PollFD ev{(int)e.ident, fl};

                v.visit(&ev);
            }
        }

        int fd() override {
            return kqfd_;
        }
    };
}
#endif

namespace {
    struct PollPoller: public PollerIface {
        IntMap<PollFD> armed_;
        Vector<struct pollfd> fds_; // rebuilt each wait()

        PollPoller(ObjPool* pool)
            : armed_(ObjPool::create(pool))
        {
        }

        void arm(PollFD pfd) override {
            armed_[pfd.fd] = pfd;
        }

        void disarm(int fd) override {
            armed_.erase(fd);
        }

        void waitImpl(VisitorFace& v, u32 timeoutUs) override {
            fds_.clear();

            armed_.visit([&](const PollFD& e) {
                fds_.pushBack({
                    .fd = e.fd,
                    .events = e.toPollEvents(),
                    .revents = 0,
                });
            });

            int n = STD_POLL(fds_.mutData(), (int)fds_.length(), (int)((timeoutUs + 999) / 1000));

            if (n < 0) {
                return;
            }

            for (const auto& pfd : range(fds_)) {
                if (pfd.revents == 0) {
                    continue;
                }

                if (!armed_.erase(pfd.fd)) {
                    continue;
                }

                PollFD ev{pfd.fd, PollFD::fromPollEvents(pfd.revents)};

                v.visit(&ev);
            }
        }
    };

    struct HybridPoller: public PollerIface {
        static constexpr size_t threshold = 100;

        PollPoller* poll_;
        PollerIface* current_;
        PollerIface* native_;

        HybridPoller(ObjPool* pool, PollerIface* native)
            : poll_(pool->make<PollPoller>(pool))
            , current_(poll_)
            , native_(native)
        {
        }

        void migrate() {
            poll_->armed_.visit([&](const PollFD& pfd) {
                native_->arm(pfd);
            });

            current_ = native_;
            poll_ = nullptr;
        }

        void arm(PollFD pfd) override {
            if (poll_) {
                poll_->arm(pfd);

                if (poll_->armed_.size() > threshold) {
                    migrate();
                }
            } else {
                current_->arm(pfd);
            }
        }

        void disarm(int fd) override {
            current_->disarm(fd);
        }

        void waitImpl(VisitorFace& v, u32 timeoutUs) override {
            current_->waitImpl(v, timeoutUs);
        }
    };
}

PollerIface* PollerIface::create(ObjPool* pool) {
    if (getenv("USE_POLL_POLLER")) {
        return pool->make<PollPoller>(pool);
    }

    if (auto native = WaitablePoller::create(pool); native) {
        return pool->make<HybridPoller>(pool, native);
    }

    return pool->make<PollPoller>(pool);
}

WaitablePoller* WaitablePoller::create(ObjPool* pool) {
#if defined(__linux__)
    return pool->make<EpollPoller>();
#elif defined(__APPLE__) || defined(__FreeBSD__)
    return pool->make<KqueuePoller>();
#else
    return nullptr;
#endif
}

namespace {
    struct SlavePoller: public WaitablePoller {
        WaitablePoller* slave_;
        Pollable* reactor_;

        SlavePoller(WaitablePoller* slave, Pollable* reactor)
            : slave_(slave)
            , reactor_(reactor)
        {
        }

        void arm(PollFD pfd) override {
            slave_->arm(pfd);
        }

        void disarm(int fd) override {
            slave_->disarm(fd);
        }

        void waitImpl(VisitorFace& v, u32 timeoutUs) override {
            auto deadlineUs = monotonicNowUs() + (u64)timeoutUs;

            if (!reactor_->poll({slave_->fd(), PollFlag::In}, deadlineUs)) {
                return;
            }

            slave_->waitImpl(v, 0);
        }

        int fd() override {
            return slave_->fd();
        }
    };
}

WaitablePoller* WaitablePoller::create(ObjPool* pool, Pollable* reactor) {
    if (auto native = WaitablePoller::create(pool); native) {
        return pool->make<SlavePoller>(native, reactor);
    }

    return nullptr;
}

namespace {
    struct MultishotPoller: public PollerIface {
        PollerIface* slave_;
        IntMap<PollFD> armed_;

        MultishotPoller(ObjPool* pool, PollerIface* slave)
            : slave_(slave)
            , armed_(ObjPool::create(pool))
        {
        }

        void arm(PollFD pfd) override {
            armed_[pfd.fd] = pfd;
            slave_->arm(pfd);
        }

        void disarm(int fd) override {
            armed_.erase(fd);
            slave_->disarm(fd);
        }

        void waitImpl(VisitorFace& v, u32 timeoutUs) override {
            auto wrapper = makeVisitor([this, &v](void* ptr) {
                auto ev = (PollFD*)ptr;

                v.visit(ev);

                if (auto p = armed_.find(ev->fd); p) {
                    slave_->arm(*p);
                }
            });

            slave_->waitImpl(wrapper, timeoutUs);
        }
    };
}

PollerIface* PollerIface::createMultishot(ObjPool* pool, PollerIface* slave) {
    return pool->make<MultishotPoller>(pool, slave);
}

void PollerIface::waitBase(VisitorFace&& v, u64 deadlineUs) {
    if (auto now = monotonicNowUs(); now >= deadlineUs) {
        waitImpl(v, 0);
    } else {
        waitImpl(v, min(deadlineUs - now, (u64)30000000));
    }
}
./std/thr/pool.cpp
#include "pool.h"
#include "task.h"
#include "mutex.h"
#include "guard.h"
#include "thread.h"
#include "runable.h"
#include "cond_var.h"
#include "wait_queue.h"

#include <std/rng/mix.h>
#include <std/rng/pcg.h>
#include <std/lib/list.h>
#include <std/sym/i_map.h>
#include <std/alg/range.h>
#include <std/alg/minmax.h>
#include <std/lib/vector.h>
#include <std/dbg/assert.h>
#include <std/dbg/insist.h>
#include <std/sys/atomic.h>
#include <std/alg/shuffle.h>
#include <std/alg/exchange.h>
#include <std/mem/obj_pool.h>
#include <std/rng/split_mix_64.h>

#include <stdlib.h>
#include <sched.h>

using namespace stl;

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
        void submitTasks(IntrusiveList& tasks) noexcept override {
            while (auto t = (Task*)tasks.popFrontOrNull()) {
                t->run();
            }
        }

        void join() noexcept override {
        }

        bool workerId(size_t* id) noexcept override {
            *id = 0;

            return true;
        }

        void flushLocal() noexcept override {
        }
    };

    class ThreadPoolImpl: public ThreadPool {
        struct Worker: public Runable {
            ThreadPoolImpl* pool_;
            Thread* thread_;

            Worker(ThreadPoolImpl* p, ObjPool* opool) noexcept
                : pool_(p)
                , thread_(Thread::create(opool, *this))
            {
            }

            auto key() const noexcept {
                return thread_->threadId();
            }

            void run() noexcept override {
                try {
                    pool_->workerLoop();
                } catch (ShutDown* sh) {
                    pool_->submitTask(sh);
                }
            }
        };

        Mutex* mutex_ = nullptr;
        CondVar* condVar_ = nullptr;
        IntrusiveList queue_;
        IntMap<Worker> workers_;
        size_t inflight_ = 0;

        void workerLoop();

    public:
        ThreadPoolImpl(ObjPool* pool, size_t numThreads);
        ~ThreadPoolImpl() noexcept;

        void submitTasks(IntrusiveList& tasks) noexcept override;
        void join() noexcept override;
        bool workerId(size_t* id) noexcept override;
        void flushLocal() noexcept override;
    };
}

ThreadPoolImpl::ThreadPoolImpl(ObjPool* pool, size_t numThreads)
    : mutex_(Mutex::create(pool))
    , condVar_(CondVar::create(pool))
    , workers_(pool)
{
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.insertKeyed(this, pool);
    }
}

void ThreadPoolImpl::submitTasks(IntrusiveList& tasks) noexcept {
    LockGuard lock(mutex_);
    inflight_ += tasks.length();
    queue_.pushBack(tasks);
    condVar_->signal();
}

void ThreadPoolImpl::join() noexcept {
    LockGuard lock(mutex_);

    while (inflight_) {
        UnlockGuard unlock(mutex_);

        sched_yield();
    }
}

ThreadPoolImpl::~ThreadPoolImpl() noexcept {
    join();

    ShutDown task;

    submitTask(&task);

    workers_.visit([](Worker& w) {
        w.thread_->join();
    });
}

bool ThreadPoolImpl::workerId(size_t*) noexcept {
    return false;
}

void ThreadPoolImpl::flushLocal() noexcept {
}

void ThreadPoolImpl::workerLoop() {
    LockGuard lock(mutex_);

    for (;; condVar_->wait(mutex_)) {
        while (auto t = (Task*)queue_.popFrontOrNull()) {
            {
                UnlockGuard unlock(mutex_);

                t->run();
            }

            --inflight_;
        }
    }
}

void ThreadPool::submitTask(Task* task) noexcept {
    IntrusiveList list;
    list.pushBack(task);
    submitTasks(list);
}

ThreadPool* ThreadPool::sync(ObjPool* pool) {
    return pool->make<SyncThreadPool>();
}

ThreadPool* ThreadPool::simple(ObjPool* pool, size_t threads) {
    if (threads == 0) {
        return sync(pool);
    }

    return pool->make<ThreadPoolImpl>(pool, threads);
}

namespace {
    struct WorkStealingThreadPool: public ThreadPool {
        struct Worker: public Runable, public WaitQueue::Item {
            ObjPool* opool_ = nullptr;
            WorkStealingThreadPool* pool_ = nullptr;
            u32 index_;
            PCG32 rng_;
            Vector<Worker*> so_;
            Mutex* mutex_ = nullptr;
            CondVar* condVar_ = nullptr;
            IntrusiveList tasks_;
            IntrusiveList local_;
            Thread* thread_ = nullptr;

            Worker(WorkStealingThreadPool* pool, ObjPool* opool, u32 myIndex, u64 seed);

            auto key() const noexcept {
                return thread_->threadId();
            }

            void flushLocal() noexcept {
                tasks_.pushBack(local_);
            }

            Task* popNoLock() noexcept {
                return (Task*)tasks_.popFrontOrNull();
            }

            PCG32& random() noexcept {
                return rng_;
            }

            void loop();
            void join() noexcept;
            void sleep() noexcept;
            void run() noexcept override;
            void initStealOrder() noexcept;
            void push(Task* task) noexcept;
            void push(IntrusiveList* task) noexcept;
            bool shouldSleep(i32 searching) noexcept;
            void steal(IntrusiveList* stolen) noexcept;
            void trySteal(IntrusiveList* stolen) noexcept;
            void splitHalf(IntrusiveList* stolen) noexcept;
            void pushNoSignal(IntrusiveList& tasks) noexcept;
        };

        IntMap<Worker*> workers_;
        Vector<Worker*> index_;
        WaitQueue* wq;
        ThreadPoolHooks* hooks_ = nullptr;
        alignas(64) i32 taskCount_ = 0;
        alignas(64) i32 searching_ = 0;

        WorkStealingThreadPool(ObjPool* pool, size_t numThreads, ThreadPoolHooks* hooks);
        ~WorkStealingThreadPool() noexcept;

        void join() noexcept override;
        Worker* localWorker() noexcept;
        void flushLocal() noexcept override;
        bool workerId(size_t* id) noexcept override;
        void submitTasks(IntrusiveList& tasks) noexcept override;
    };
}

void WorkStealingThreadPool::Worker::sleep() noexcept {
    pool_->wq->enqueue(this);
    condVar_->wait(mutex_);
}

void WorkStealingThreadPool::Worker::push(Task* task) noexcept {
    LockGuard lock(mutex_);
    tasks_.pushBack(task);
    condVar_->signal();
}

void WorkStealingThreadPool::Worker::pushNoSignal(IntrusiveList& tasks) noexcept {
    LockGuard lock(mutex_);
    tasks_.pushBack(tasks);
}

void WorkStealingThreadPool::Worker::push(IntrusiveList* tasks) noexcept {
    LockGuard lock(mutex_);
    tasks_.pushBack(*tasks);
    condVar_->signal();
}

WorkStealingThreadPool::WorkStealingThreadPool(ObjPool* pool, size_t numThreads, ThreadPoolHooks* hooks)
    : workers_(pool)
    , wq(WaitQueue::construct(pool, numThreads))
    , hooks_(hooks)
{
    PCG32 rng(this);

    for (size_t i = 0; i < numThreads; ++i) {
        auto w = pool->make<Worker>(this, pool, i, rng.nextU64());
        workers_.insert(w->key(), w);
    }

    workers_.visit([this](Worker* w) {
        index_.pushBack(w);
        w->initStealOrder();
    });

    workers_.visit([](Worker* w) {
        w->mutex_->unlock();
    });
}

WorkStealingThreadPool::Worker* WorkStealingThreadPool::localWorker() noexcept {
    static thread_local Worker* curw = nullptr;

    if (curw) {
        return curw;
    } else if (auto w = workers_.find(Thread::currentThreadId()); w) {
        return curw = *w;
    }

    return nullptr;
}

void WorkStealingThreadPool::submitTasks(IntrusiveList& tasks) noexcept {
    auto count = (i32)tasks.length();
    auto tc = stdAtomicAddAndFetch(&taskCount_, count, MemoryOrder::Release);

    if (auto w = localWorker(); w) {
        w->local_.pushBack(tasks);
    } else if (auto w = (Worker*)wq->dequeue(); w) {
        w->push(&tasks);
    } else {
        index_[mix(tasks.mutFront(), &tasks, (void*)(uintptr_t)tc) % index_.length()]->pushNoSignal(tasks);

        if (auto w = (Worker*)wq->dequeue(); w) {
            IntrusiveList tmp;
            w->push(&tmp);
        }
    }
}

bool WorkStealingThreadPool::workerId(size_t* id) noexcept {
    if (auto w = localWorker()) {
        *id = w->index_;

        return true;
    }

    return false;
}

void WorkStealingThreadPool::flushLocal() noexcept {
    if (auto w = localWorker()) {
        if (w->local_.empty()) {
            return;
        }

        Task* stolen = nullptr;
        Worker* target = nullptr;

        {
            LockGuard lock(w->mutex_);

            w->flushLocal();

            if (auto s = (Worker*)wq->dequeue(); s) {
                if (s == w) {
                    w->condVar_->signal();
                } else {
                    target = s;
                    stolen = w->popNoLock();
                }
            }
        }

        if (target) {
            target->push(stolen);
        }
    }
}

void WorkStealingThreadPool::join() noexcept {
    while (wq->sleeping() != workers_.size()) {
        sched_yield();
    }
}

WorkStealingThreadPool::~WorkStealingThreadPool() noexcept {
    join();

    workers_.visit([](Worker* w) {
        w->join();
    });

    STD_INSIST(taskCount_ == 0);
}

WorkStealingThreadPool::Worker::Worker(WorkStealingThreadPool* pool, ObjPool* opool, u32 myIndex, u64 seed)
    : WaitQueue::Item{nullptr, (u8)myIndex}
    , opool_(ObjPool::create(opool))
    , pool_(pool)
    , index_(myIndex)
    , rng_(seed)
    , mutex_(pool->hooks_->createMutex(opool_))
{
    mutex_->lock();
    thread_ = Thread::create(opool_, *this);
}

void WorkStealingThreadPool::Worker::join() noexcept {
    ShutDown sh;
    stdAtomicAddAndFetch(&pool_->taskCount_, 1, MemoryOrder::Relaxed);
    push(&sh);
    thread_->join();
}

void WorkStealingThreadPool::Worker::initStealOrder() noexcept {
    pool_->workers_.visit([this](Worker* w) {
        if (w != this) {
            so_.pushBack(w);
        }
    });

    shuffle(rng_, so_.mutBegin(), so_.mutEnd());
}

bool WorkStealingThreadPool::Worker::shouldSleep(i32 searching) noexcept {
    if (!tasks_.empty()) {
        return false;
    }

    if (searching == 0) {
        if (stdAtomicFetch(&pool_->taskCount_, MemoryOrder::Acquire) > 0) {
            return false;
        }
    }

    return true;
}

void WorkStealingThreadPool::Worker::trySteal(IntrusiveList* stolen) noexcept {
    const u32 n = (u32)so_.length();
    const u32 offset = rng_.uniformUnbiased(n);

    for (u32 i = 0; stolen->empty() && i < n; ++i) {
        so_[(offset + i) % n]->steal(stolen);
    }
}

void WorkStealingThreadPool::Worker::run() noexcept {
    try {
        loop();
    } catch (ShutDown* sh) {
    }
}

void WorkStealingThreadPool::Worker::loop() {
    condVar_ = pool_->hooks_->createCondVar(opool_, index_);

    LockGuard lock(mutex_);

    while (true) {
        while (auto task = popNoLock()) {
            if (tasks_.empty()) {
                // pass
            } else if (auto w = (Worker*)pool_->wq->dequeue(); w) {
                w->push(popNoLock());
            }

            stdAtomicSubAndFetch(&pool_->taskCount_, 1, MemoryOrder::Relaxed);

            UnlockGuard(mutex_).run([task] {
                task->run();
            });

            flushLocal();
        }

        stdAtomicAddAndFetch(&pool_->searching_, 1, MemoryOrder::Relaxed);

        IntrusiveList stolen;

        UnlockGuard(mutex_).run([this, &stolen] {
            trySteal(&stolen);
        });

        local_.pushBack(stolen);
        flushLocal();

        if (shouldSleep(stdAtomicSubAndFetch(&pool_->searching_, 1, MemoryOrder::Release))) {
            sleep();
            flushLocal();
        }
    }
}

void WorkStealingThreadPool::Worker::steal(IntrusiveList* stolen) noexcept {
    if (mutex_->tryLock()) {
        splitHalf(stolen);
        mutex_->unlock();
    }
}

void WorkStealingThreadPool::Worker::splitHalf(IntrusiveList* stolen) noexcept {
    if (tasks_.empty()) {
        return;
    } else if (tasks_.almostEmpty()) {
        tasks_.xchgWithEmptyList(*stolen);
    } else {
        tasks_.cutHalf(*stolen);
    }
}

ThreadPool* ThreadPool::workStealing(ObjPool* pool, size_t threads) {
    struct DefaultThreadPoolHooks: public ThreadPoolHooks {
        Mutex* createMutex(ObjPool* pool) override {
            return Mutex::create(pool);
        }

        CondVar* createCondVar(ObjPool* pool, size_t) override {
            return CondVar::create(pool);
        }
    };

    static auto df = new DefaultThreadPoolHooks();

    return workStealing(pool, threads, df);
}

ThreadPool* ThreadPool::workStealing(ObjPool* pool, size_t threads, ThreadPoolHooks* hooks) {
    if (threads <= 1) {
        return simple(pool, threads);
    }

    return pool->make<WorkStealingThreadPool>(pool, threads, hooks);
}
./std/thr/reactor_poll.cpp
#include "reactor_poll.h"

#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "guard.h"
#include "thread.h"
#include "parker.h"
#include "poller.h"
#include "poll_fd.h"
#include "io_reactor.h"

#include <std/mem/new.h>
#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/lib/node.h>
#include <std/thr/task.h>
#include <std/map/treap.h>
#include <std/sym/i_map.h>
#include <std/alg/range.h>
#include <std/alg/minmax.h>
#include <std/sys/atomic.h>
#include <std/dbg/assert.h>
#include <std/lib/visitor.h>
#include <std/alg/exchange.h>
#include <std/mem/obj_pool.h>
#include <std/map/treap_node.h>

using namespace stl;

namespace {
    struct DeadlineTreap: public Treap {
        bool cmp(void* a, void* b) const noexcept override;
        u64 earliest() const noexcept;
    };

    struct ReactorState;
    struct ReactorPoller;

    struct ReqCommon {
        u64 deadline = 0;
        Task* task = nullptr;
        ReactorState* reactor = nullptr;

        void reset(ReactorState* r, u64 dl) noexcept;
    };

    struct InternalReq: public TreapNode, public IntrusiveNode {
        PollFD pfd = {};
        u32 result = 0;
        ReqCommon* common = nullptr;

        virtual void complete(u32 res, IntrusiveList& ready) noexcept;
    };

    struct InternalMultiReq: public InternalReq {
        void complete(u32 res, IntrusiveList& ready) noexcept override;
    };

    struct FdEntry: public IntrusiveList {
        u32 flags() const noexcept;
    };

    struct ReactorState: public ReactorIface, public Runable {
        CoroExecutor* exec_;
        PollerIface* poller;
        DeadlineTreap timers;
        DeadlineTreap sleepers;
        IntMap<FdEntry> fdMap_;
        Mutex* queueMutex_;
        DeadlineTreap queue_;
        Parker parker_;
        bool stopped_ = false;
        Thread* thread_ = nullptr;

        ReactorState(CoroExecutor* exec, ObjPool* opool);
        ~ReactorState() noexcept;

        void drainQueue();
        void rearmOrDisarm(int fd);
        void run() noexcept override;
        void cancelInternal(InternalReq* req);
        void processEvent(PollFD* ev, IntrusiveList& ready) noexcept;

        u32 poll(PollFD pfd, u64 deadlineUs) override;
        PollerIface* createPoller(ObjPool* pool) override;
    };

    struct ReactorPoller: public PollerIface, public ReqCommon {
        IntMap<InternalMultiReq> fds_;

        ReactorPoller(ObjPool* pool, ReactorState* rs);

        void arm(PollFD pfd) override;
        void disarm(int fd) override;
        void waitImpl(VisitorFace& v, u32 timeoutUs) override;
        void cancelOthers(InternalMultiReq* except);
    };
}

bool DeadlineTreap::cmp(void* a, void* b) const noexcept {
    auto ra = (InternalReq*)a;
    auto rb = (InternalReq*)b;

    if (ra->common->deadline != rb->common->deadline) {
        return ra->common->deadline < rb->common->deadline;
    }

    return ra < rb;
}

u64 DeadlineTreap::earliest() const noexcept {
    auto n = (InternalReq*)min();

    return n ? n->common->deadline : UINT64_MAX;
}

void ReqCommon::reset(ReactorState* r, u64 dl) noexcept {
    reactor = r;
    deadline = dl;
    task = nullptr;
}

void InternalReq::complete(u32 res, IntrusiveList& ready) noexcept {
    result = res;
    ready.pushBack(common->task);
}

void InternalMultiReq::complete(u32 res, IntrusiveList& ready) noexcept {
    result = res;
    ready.pushBack(common->task);
    ((ReactorPoller*)common)->cancelOthers(this);
}

u32 FdEntry::flags() const noexcept {
    u32 f = 0;

    for (auto n = front(), e = end(); n != e; n = n->next) {
        f |= static_cast<const InternalReq*>(n)->pfd.flags;
    }

    return f;
}

ReactorState::ReactorState(CoroExecutor* exec, ObjPool* opool)
    : exec_(exec)
    , poller(PollerIface::create(opool))
    , fdMap_(ObjPool::create(opool))
    , queueMutex_(Mutex::createSpinLock(opool, exec))
{
    poller->arm({parker_.fd(), PollFlag::In});
    thread_ = Thread::create(opool, *this);
}

void ReactorState::rearmOrDisarm(int fd) {
    if (auto entry = fdMap_.find(fd); !entry) {
        poller->disarm(fd);
    } else if (entry->empty()) {
        poller->disarm(fd);
        fdMap_.erase(fd);
    } else {
        poller->arm({fd, entry->flags()});
    }
}

void ReactorState::cancelInternal(InternalReq* req) {
    req->remove();
    timers.remove(req);
    rearmOrDisarm(req->pfd.fd);
}

ReactorState::~ReactorState() noexcept {
    stdAtomicStore(&stopped_, true, MemoryOrder::Release);
    parker_.signal();
    thread_->join();
}

void ReactorState::drainQueue() {
    DeadlineTreap local;

    LockGuard(queueMutex_).run([this, &local] {
        queue_.xchg(local);
    });

    local.visit([&](TreapNode* node) {
        auto req = (InternalReq*)node;

        req->left = nullptr;
        req->right = nullptr;

        if (int fd = req->pfd.fd; fd == -1) {
            sleepers.insert(req);
        } else {
            timers.insert(req);

            auto& entry = fdMap_[fd];

            entry.pushBack(req);
            poller->arm({fd, entry.flags()});
        }
    });
}

void ReactorState::processEvent(PollFD* ev, IntrusiveList& ready) noexcept {
    if (auto entry = fdMap_.find(ev->fd); entry) {
        for (auto n = entry->mutFront(), e = entry->mutEnd(); n != e;) {
            if (auto req = (InternalReq*)exchange(n, n->next); req->pfd.flags & ev->flags) {
                req->remove();
                timers.remove(req);
                req->complete(ev->flags, ready);
            }
        }

        rearmOrDisarm(ev->fd);
    }
}

void ReactorState::run() noexcept {
    while (!stdAtomicFetch(&stopped_, MemoryOrder::Acquire)) {
        IntrusiveList ready;

        parker_.park([&] {
            drainQueue();

            poller->wait([this, &ready](PollFD* ev) {
                if (ev->fd == parker_.fd()) {
                    parker_.drain();
                    poller->arm({parker_.fd(), PollFlag::In});
                } else {
                    processEvent(ev, ready);
                }
            }, min(timers.earliest(), sleepers.earliest()));
        });

        auto now = monotonicNowUs();

        while (auto req = (InternalReq*)timers.min()) {
            if (req->common->deadline > now) {
                break;
            }

            timers.remove(req);
            req->remove();
            rearmOrDisarm(req->pfd.fd);
            req->complete(0, ready);
        }

        while (auto req = (InternalReq*)sleepers.min()) {
            if (req->common->deadline > now) {
                break;
            }

            sleepers.remove(req);
            req->complete(0, ready);
        }

        if (!ready.empty()) {
            exec_->pool()->submitTasks(ready);
        }
    }
}

u32 ReactorState::poll(PollFD pfd, u64 deadlineUs) {
    ReqCommon common;

    common.reset(this, deadlineUs);

    InternalReq req;

    req.pfd = pfd;
    req.common = &common;

    queueMutex_->lock();
    queue_.insert(&req);

    // clang-format off
    exec_->parkWith(makeRunable([this, needsWakeup = (queue_.min() == &req)] {
        queueMutex_->unlock();

        if (needsWakeup) {
            parker_.unpark();
        }
    }), &common.task);
    // clang-format on

    return req.result;
}

PollerIface* ReactorState::createPoller(ObjPool* pool) {
    if (auto p = WaitablePoller::create(pool, this); p) {
        return p;
    }

    return pool->make<ReactorPoller>(pool, this);
}

// ReactorPoller

ReactorPoller::ReactorPoller(ObjPool* pool, ReactorState* rs)
    : fds_(pool)
{
    reactor = rs;
}

void ReactorPoller::arm(PollFD pfd) {
    fds_[pfd.fd].pfd = pfd;
}

void ReactorPoller::disarm(int fd) {
    fds_.erase(fd);
}

void ReactorPoller::waitImpl(VisitorFace& v, u32 timeoutUs) {
    auto rs = reactor;
    auto dl = monotonicNowUs() + (u64)timeoutUs;

    reset(rs, dl);

    fds_.visit([](InternalMultiReq& req) {
        req.result = 0;
        req.left = nullptr;
        req.right = nullptr;
    });

    rs->queueMutex_->lock();

    fds_.visit([this, rs](InternalMultiReq& req) {
        req.common = this;
        rs->queue_.insert(&req);
    });

    // clang-format off
    rs->exec_->parkWith(makeRunable([rs] {
        rs->queueMutex_->unlock();
        rs->parker_.unpark();
    }), &task);
    // clang-format on

    fds_.visit([&v](InternalMultiReq& req) {
        if (auto res = req.result; res) {
            PollFD pfd = {req.pfd.fd, res};

            v.visit(&pfd);
        }
    });
}

void ReactorPoller::cancelOthers(InternalMultiReq* except) {
    fds_.visit([this, except](InternalMultiReq& req) {
        if (&req != except) {
            reactor->cancelInternal(&req);
        }
    });
}

ReactorIface* ReactorIface::create(CoroExecutor* exec, ObjPool* opool) {
    return opool->make<ReactorState>(exec, opool);
}
./std/thr/runable.cpp
#include "runable.h"
./std/thr/semaphore.cpp
#include "semaphore.h"
#include "coro.h"

#include <std/dbg/insist.h>
#include <std/mem/obj_pool.h>

#if defined(__APPLE__)
    #include <dispatch/dispatch.h>
#endif

#if defined(__linux__)
    #include <semaphore.h>
#endif

using namespace stl;

void* Semaphore::nativeHandle() noexcept {
    return nullptr;
}

namespace {
#if defined(__APPLE__)
    struct SemImpl: public Semaphore {
        dispatch_semaphore_t sem_;

        SemImpl(size_t initial) noexcept
            : sem_(dispatch_semaphore_create((long)initial))
        {
        }

        ~SemImpl() noexcept {
            dispatch_release(sem_);
        }

        void post() noexcept override {
            dispatch_semaphore_signal(sem_);
        }

        void wait() noexcept override {
            dispatch_semaphore_wait(sem_, DISPATCH_TIME_FOREVER);
        }

        bool tryWait() noexcept override {
            return dispatch_semaphore_wait(sem_, DISPATCH_TIME_NOW) == 0;
        }
    };
#elif defined(__linux__)
    struct SemImpl: public Semaphore {
        sem_t sem_;

        SemImpl(size_t initial) noexcept {
            STD_INSIST(sem_init(&sem_, 0, initial) == 0);
        }

        ~SemImpl() noexcept {
            STD_INSIST(sem_destroy(&sem_) == 0);
        }

        void post() noexcept override {
            STD_INSIST(sem_post(&sem_) == 0);
        }

        void wait() noexcept override {
            STD_INSIST(sem_wait(&sem_) == 0);
        }

        bool tryWait() noexcept override {
            return sem_trywait(&sem_) == 0;
        }
    };
#endif
}

Semaphore* Semaphore::create(ObjPool* pool, size_t initial) {
    return pool->make<SemImpl>(initial);
}

Semaphore* Semaphore::create(ObjPool* pool, size_t initial, CoroExecutor* exec) {
    return exec->createSemaphore(pool, initial);
}
./std/thr/task.cpp
#include "task.h"
./std/thr/thread.cpp
#include "thread.h"

#include "coro.h"
#include "runable.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>
#include <std/mem/obj_pool.h>

#include <pthread.h>

using namespace stl;

namespace {
    struct PosixThreadImpl: public Thread {
        pthread_t thread_;

        static void* threadFunc(void* arg) {
            return (((Runable*)arg)->run(), nullptr);
        }

        void start(Runable& runable) override {
            if (pthread_create(&thread_, nullptr, threadFunc, &runable)) {
                Errno().raise(StringBuilder() << StringView(u8"pthread_create failed"));
            }
        }

        void join() noexcept override {
            STD_INSIST(pthread_join(thread_, nullptr) == 0);
        }

        u64 threadId() const noexcept override {
            static_assert(sizeof(pthread_t) <= sizeof(u64));
            return (u64)thread_;
        }
    };
}

u64 Thread::currentThreadId() noexcept {
    return (u64)pthread_self();
}

Thread* Thread::create(ObjPool* pool, Runable& runable) {
    auto t = pool->make<PosixThreadImpl>();

    t->start(runable);

    return t;
}

Thread* Thread::create(ObjPool* pool, CoroExecutor* exec, Runable& runable) {
    auto t = exec->createThread(pool);

    t->start(runable);

    return t;
}
./std/thr/wait_group.cpp
#include "wait_group.h"

#include "coro.h"
#include "mutex.h"
#include "guard.h"
#include "cond_var.h"

#include <std/sys/types.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    struct WaitGroupImpl: public WaitGroup {
        Mutex* mutex;
        CondVar* cv;
        size_t counter;

        WaitGroupImpl(Mutex* m, CondVar* c, size_t init)
            : mutex(m)
            , cv(c)
            , counter(init)
        {
        }

        void add(size_t n) noexcept override {
            LockGuard lock(mutex);
            counter += n;
        }

        void done() noexcept override {
            LockGuard lock(mutex);

            if (--counter == 0) {
                cv->broadcast();
            }
        }

        void wait() noexcept override {
            LockGuard lock(mutex);

            while (counter > 0) {
                cv->wait(mutex);
            }
        }
    };
}

WaitGroup* WaitGroup::create(ObjPool* pool, size_t init) {
    return pool->make<WaitGroupImpl>(Mutex::create(pool), CondVar::create(pool), init);
}

WaitGroup* WaitGroup::create(ObjPool* pool, size_t init, CoroExecutor* exec) {
    return pool->make<WaitGroupImpl>(Mutex::create(pool, exec), CondVar::create(pool, exec), init);
}
./std/thr/wait_queue.cpp
#include "wait_queue.h"
#include "mutex.h"
#include "guard.h"

#include <std/sys/types.h>
#include <std/dbg/assert.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

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

        size_t sleeping() const noexcept override {
            T val = stdAtomicFetch(&bits_, MemoryOrder::Acquire);

            if constexpr (sizeof(T) == 8) {
                return (size_t)__builtin_popcountll(val);
            } else {
                return (size_t)__builtin_popcount(val);
            }
        }
    };

#if defined(__x86_64__)
    struct alignas(64) Bitmask128Impl: public WaitQueue {
        static constexpr u8 N = 128;

        struct alignas(16) Bits {
            u64 lo;
            u64 hi;
        };

        Bits bits_ = {0, 0};
        Item* items_[N] = {};

        void enqueue(Item* item) noexcept override {
            u8 idx = item->index;
            STD_ASSERT(idx < N);

            items_[idx] = item;

            Bits old;

            __atomic_load(&bits_, &old, __ATOMIC_RELAXED);

            Bits desired;

            do {
                desired = old;

                if (idx < 64) {
                    desired.lo |= u64(1) << idx;
                } else {
                    desired.hi |= u64(1) << (idx - 64);
                }
            } while (!__atomic_compare_exchange(&bits_, &old, &desired, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED));
        }

        Item* dequeue() noexcept override {
            Bits old;

            __atomic_load(&bits_, &old, __ATOMIC_ACQUIRE);

            for (;;) {
                if (!old.lo && !old.hi) {
                    return nullptr;
                }

                int idx;
                Bits desired = old;

                if (old.lo) {
                    idx = __builtin_ctzll(old.lo);
                    desired.lo &= ~(u64(1) << idx);
                } else {
                    idx = 64 + __builtin_ctzll(old.hi);
                    desired.hi &= ~(u64(1) << (idx - 64));
                }

                if (__atomic_compare_exchange(&bits_, &old, &desired, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)) {
                    return items_[idx];
                }
            }
        }

        size_t sleeping() const noexcept override {
            Bits b;

            __atomic_load(&bits_, &b, __ATOMIC_ACQUIRE);

            return (size_t)__builtin_popcountll(b.lo) + (size_t)__builtin_popcountll(b.hi);
        }
    };

    // No ABA here: each Item is a Worker that re-enqueues itself only after being
    // dequeued and completing a full condvar sleep/wake cycle.
    // Uses CMPXCHG16B to atomically update head pointer and count together.
    struct alignas(64) PointerImpl: public WaitQueue {
        struct alignas(16) State {
            Item* head;
            size_t count;
        };

        State state_ = {nullptr, 0};

        void enqueue(Item* item) noexcept override {
            State old;

            __atomic_load(&state_, &old, __ATOMIC_RELAXED);

            State desired;

            do {
                item->next = old.head;
                desired = {item, old.count + 1};
            } while (!__atomic_compare_exchange(&state_, &old, &desired, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED));
        }

        Item* dequeue() noexcept override {
            State old;

            __atomic_load(&state_, &old, __ATOMIC_ACQUIRE);

            for (;;) {
                if (!old.head) {
                    return nullptr;
                }

                State desired = {old.head->next, old.count - 1};

                if (__atomic_compare_exchange(&state_, &old, &desired, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)) {
                    return old.head;
                }
            }
        }

        size_t sleeping() const noexcept override {
            State s;

            __atomic_load(&state_, &s, __ATOMIC_ACQUIRE);

            return s.count;
        }
    };
#elif __SIZEOF_POINTER__ == 8
    // No ABA here: each Item is a Worker that re-enqueues itself only after being
    // dequeued and completing a full condvar sleep/wake cycle. The tag would need
    // to wrap 65536 times while a competing CAS spins — impossible in practice.
    struct alignas(64) PointerImpl: public WaitQueue {
        // upper 16 bits: tag, lower 48 bits: pointer
        u64 head_ = 0;
        size_t count_ = 0;

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

            stdAtomicAddAndFetch(&count_, 1, MemoryOrder::Release);
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
                    stdAtomicSubAndFetch(&count_, 1, MemoryOrder::Release);

                    return ptr;
                }
            }
        }

        size_t sleeping() const noexcept override {
            return stdAtomicFetch(&count_, MemoryOrder::Acquire);
        }
    };
#endif

    struct MutexImpl: public WaitQueue {
        Mutex* mutex_;
        Item* head = nullptr;
        size_t count_ = 0;

        MutexImpl(Mutex* m)
            : mutex_(m)
        {
        }

        void enqueue(Item* item) noexcept override {
            LockGuard lock(mutex_);

            item->next = head;
            head = item;

            stdAtomicAddAndFetch(&count_, 1, MemoryOrder::Release);
        }

        Item* dequeue() noexcept override {
            if (!stdAtomicFetch(&count_, MemoryOrder::Acquire)) {
                return nullptr;
            }

            LockGuard lock(mutex_);

            Item* item = head;

            if (!item) {
                return nullptr;
            }

            head = item->next;

            stdAtomicSubAndFetch(&count_, 1, MemoryOrder::Release);

            return item;
        }

        size_t sleeping() const noexcept override {
            return stdAtomicFetch(&count_, MemoryOrder::Acquire);
        }
    };
}

WaitQueue* WaitQueue::construct(ObjPool* pool, size_t maxWaiters) {
    if (maxWaiters <= 32) {
        return pool->make<BitmaskImpl<u32>>();
    }

#if __SIZEOF_POINTER__ == 8
    if (maxWaiters <= 64) {
        return pool->make<BitmaskImpl<u64>>();
    }
#endif

#if defined(__x86_64__)
    if (maxWaiters <= 128) {
        return pool->make<Bitmask128Impl>();
    }
#endif

#if __SIZEOF_POINTER__ == 8
    return pool->make<PointerImpl>();
#endif

    return pool->make<MutexImpl>(Mutex::create(pool));
}
./std/thr/io_classic.cpp
#include "io_classic.h"

#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "poll_fd.h"
#include "cond_var.h"
#include "io_reactor.h"
#include "reactor_poll.h"

#include <std/rng/mix.h>
#include <std/lib/vector.h>
#include <std/mem/obj_pool.h>
#include <std/rng/split_mix_64.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/socket.h>

#ifndef MSG_NOSIGNAL
    #define MSG_NOSIGNAL 0
#endif

#ifndef SOCK_NONBLOCK
    #define SOCK_NONBLOCK 0
#endif

#ifndef SOCK_CLOEXEC
    #define SOCK_CLOEXEC 0
#endif

using namespace stl;

namespace {
    struct PollIoReactor: public IoReactor, public ThreadPoolHooks {
        Vector<ReactorIface*> reactors_;
        CoroExecutor* exec_;
        ThreadPool* offload_;

        PollIoReactor(ObjPool* pool, CoroExecutor* exec, size_t reactors);

        ReactorIface* reactor(int fd) noexcept {
            return reactors_[splitMix64(fd) % reactors_.length()];
        }

        ThreadPoolHooks* hooks() override {
            return this;
        }

        Mutex* createMutex(ObjPool* pool) override;
        CondVar* createCondVar(ObjPool* pool, size_t) override;

        PollerIface* createPoller(ObjPool* pool) override;

        int recv(int fd, size_t* nRead, void* buf, size_t len, u64 deadlineUs) override;
        int recvfrom(int fd, size_t* nRead, void* buf, size_t len, sockaddr* addr, u32* addrLen, u64 deadlineUs) override;
        int send(int fd, size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) override;
        int sendto(int fd, size_t* nWritten, const void* buf, size_t len, const sockaddr* addr, u32 addrLen, u64 deadlineUs) override;
        int read(int fd, size_t* nRead, void* buf, size_t len, u64 deadlineUs) override;
        int write(int fd, size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) override;
        int writev(int fd, size_t* nWritten, iovec* iov, size_t iovcnt, u64 deadlineUs) override;
        int accept(int fd, int* newFd, sockaddr* addr, u32* addrLen, u64 deadlineUs) override;
        int connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) override;

        int pread(int fd, size_t* nRead, void* buf, size_t len, off_t offset) override;
        int pwrite(int fd, size_t* nWritten, const void* buf, size_t len, off_t offset) override;
        int fsync(int fd) override;
        int fdatasync(int fd) override;

        u32 poll(PollFD pfd, u64 deadlineUs) override;
        void sleep(u64 deadlineUs) override;
    };
}

PollIoReactor::PollIoReactor(ObjPool* pool, CoroExecutor* exec, size_t reactors)
    : exec_(exec)
    , offload_(ThreadPool::simple(pool, reactors))
{
    for (size_t i = 0; i < reactors; ++i) {
        reactors_.pushBack(ReactorIface::create(exec, pool));
    }
}

int PollIoReactor::recv(int fd, size_t* nRead, void* buf, size_t len, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::recv(fd, buf, len, 0);

        if (n >= 0) {
            *nRead = (size_t)n;
            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor(fd)->poll({fd, PollFlag::In}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::recvfrom(int fd, size_t* nRead, void* buf, size_t len, sockaddr* addr, u32* addrLen, u64 deadlineUs) {
    for (;;) {
        socklen_t slen = addrLen ? (socklen_t)*addrLen : 0;
        ssize_t n = ::recvfrom(fd, buf, len, 0, addr, addrLen ? &slen : nullptr);

        if (n >= 0) {
            *nRead = (size_t)n;

            if (addrLen) {
                *addrLen = (u32)slen;
            }

            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor(fd)->poll({fd, PollFlag::In}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::send(int fd, size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::send(fd, buf, len, MSG_NOSIGNAL);

        if (n >= 0) {
            *nWritten = (size_t)n;
            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor(fd)->poll({fd, PollFlag::Out}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::sendto(int fd, size_t* nWritten, const void* buf, size_t len, const sockaddr* addr, u32 addrLen, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::sendto(fd, buf, len, MSG_NOSIGNAL, addr, addrLen);

        if (n >= 0) {
            *nWritten = (size_t)n;
            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor(fd)->poll({fd, PollFlag::Out}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::read(int fd, size_t* nRead, void* buf, size_t len, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::read(fd, buf, len);

        if (n >= 0) {
            *nRead = (size_t)n;
            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor(fd)->poll({fd, PollFlag::In}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::write(int fd, size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::write(fd, buf, len);

        if (n >= 0) {
            *nWritten = (size_t)n;
            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor(fd)->poll({fd, PollFlag::Out}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::writev(int fd, size_t* nWritten, iovec* iov, size_t iovcnt, u64 deadlineUs) {
    for (;;) {
        ssize_t n = ::writev(fd, iov, iovcnt);

        if (n >= 0) {
            *nWritten = (size_t)n;
            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor(fd)->poll({fd, PollFlag::Out}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::accept(int fd, int* newFd, sockaddr* addr, u32* addrLen, u64 deadlineUs) {
    for (;;) {
        socklen_t slen = addrLen ? (socklen_t)*addrLen : 0;

#ifdef __linux__
        int r = ::accept4(fd, addr, addrLen ? &slen : nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
        int r = ::accept(fd, addr, addrLen ? &slen : nullptr);
#endif

        if (r >= 0) {
            if (addrLen) {
                *addrLen = (u32)slen;
            }

            *newFd = r;

            return 0;
        }

        if (errno != EAGAIN) {
            return errno;
        }

        if (!reactor(fd)->poll({fd, PollFlag::In}, deadlineUs)) {
            return EAGAIN;
        }
    }
}

int PollIoReactor::connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) {
    if (int r = ::connect(fd, addr, addrLen); r == 0) {
        return 0;
    } else if (errno != EINPROGRESS) {
        return errno;
    }

    reactor(fd)->poll({fd, PollFlag::Out}, deadlineUs);

    int err = 0;
    socklen_t len = sizeof(err);

    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
        return errno;
    }

    return err;
}

int PollIoReactor::pread(int fd, size_t* nRead, void* buf, size_t len, off_t offset) {
    int result = 0;

    exec_->offload(offload_, [&] {
        ssize_t n = ::pread(fd, buf, len, offset);

        if (n < 0) {
            result = errno;
        } else {
            *nRead = (size_t)n;
        }
    });

    return result;
}

int PollIoReactor::pwrite(int fd, size_t* nWritten, const void* buf, size_t len, off_t offset) {
    int result = 0;

    exec_->offload(offload_, [&] {
        ssize_t n = ::pwrite(fd, buf, len, offset);

        if (n < 0) {
            result = errno;
        } else {
            *nWritten = (size_t)n;
        }
    });

    return result;
}

int PollIoReactor::fsync(int fd) {
    int result = 0;

    exec_->offload(offload_, [&] {
        if (::fsync(fd) < 0) {
            result = errno;
        }
    });

    return result;
}

int PollIoReactor::fdatasync(int fd) {
    int result = 0;

    // clang-format off
    exec_->offload(offload_, [&] {
#if defined(__APPLE__)
        if (::fcntl(fd, F_FULLFSYNC) < 0) {
            result = errno;
        }
#else
        if (::fdatasync(fd) < 0) {
            result = errno;
        }
#endif
    });
    // clang-format on

    return result;
}

u32 PollIoReactor::poll(PollFD pfd, u64 deadlineUs) {
    return reactor(pfd.fd)->poll(pfd, deadlineUs);
}

void PollIoReactor::sleep(u64 deadlineUs) {
    poll({-1, 0}, deadlineUs);
}

PollerIface* PollIoReactor::createPoller(ObjPool* pool) {
    return reactors_[mix(&pool, pool) % reactors_.length()]->createPoller(pool);
}

Mutex* PollIoReactor::createMutex(ObjPool* pool) {
    return Mutex::create(pool);
}

CondVar* PollIoReactor::createCondVar(ObjPool* pool, size_t) {
    return CondVar::create(pool);
}

IoReactor* stl::createPollIoReactor(ObjPool* pool, CoroExecutor* exec, size_t reactors) {
    return pool->make<PollIoReactor>(pool, exec, reactors);
}
./std/thr/io_uring.cpp
#include "io_uring.h"

#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "guard.h"
#include "poller.h"
#include "poll_fd.h"
#include "cond_var.h"
#include "cond_var.h"
#include "io_reactor.h"

#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/lib/vector.h>
#include <std/lib/visitor.h>
#include <std/mem/obj_pool.h>

#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>

#if __has_include(<liburing.h>)
    #include <liburing.h>
#endif

using namespace stl;

#if __has_include(<liburing.h>)
namespace {
    constexpr u64 WAKEUP_COOKIE = 0xCAFE;

    struct Ring;

    struct UringReqBase {
        virtual void complete(int result, IntrusiveList& ready) noexcept = 0;
    };

    static int toErrno(int res) noexcept {
        if (res >= 0) {
            return 0;
        }

        if (res == -ECANCELED) {
            return EAGAIN;
        }

        return -res;
    }

    struct UringReq: public UringReqBase {
        Task* task;
        int res;

        void complete(int result, IntrusiveList& ready) noexcept override;

        int error() noexcept;

        template <typename T>
        int readInto(T* out) noexcept;
    };

    struct TimeoutReq: public UringReq {
        __kernel_timespec ts;
    };

    static __kernel_timespec usToTimespec(u64 us) noexcept {
        __kernel_timespec ts;

        ts.tv_sec = us / 1000000;
        ts.tv_nsec = (us % 1000000) * 1000;

        return ts;
    }

    struct Ring: public io_uring {
        Ring();
        Ring(u32 flags);

        virtual ~Ring() noexcept;

        io_uring_sqe* getSqe() noexcept;
        void getSqe(io_uring_sqe*& a, io_uring_sqe*& b) noexcept;

        void enable() noexcept;
        void sendMsg(int targetFd) noexcept;

        virtual void wakeUp(int targetFd) noexcept;
    };

    struct ExternalRing: public Ring {
        Mutex* mutex_;

        ExternalRing(ObjPool* pool);

        void wakeUp(int targetFd) noexcept override;
    };

    struct UringReactorImpl;

    struct UringCondVarImpl: public CondVar {
        Ring* ring_;
        UringReactorImpl* reactor_;

        UringCondVarImpl(Ring* ring, UringReactorImpl* reactor) noexcept;

        bool cycle() noexcept;

        void signal() noexcept override;
        void broadcast() noexcept override;
        void wait(Mutex* mutex) noexcept override;
    };

    struct UringReactorImpl: public IoReactor, public ThreadPoolHooks {
        Vector<Ring*> rings_;
        Ring* ext_;
        CoroExecutor* exec_;

        Ring* currentRing() noexcept;

        template <typename F>
        UringReq submit(F prep, u64 deadlineUs) noexcept;

        template <typename Req, typename F>
        void submitReq(Req& req, F prep) noexcept;

        template <typename Req, typename F>
        void submitReq(Req& req, F prep, u64 deadlineUs) noexcept;

        UringReactorImpl(ObjPool* pool, CoroExecutor* exec, size_t threads);

        ThreadPoolHooks* hooks() override;
        Mutex* createMutex(ObjPool* pool) override;
        CondVar* createCondVar(ObjPool* pool, size_t index) override;

        int recv(int, size_t*, void*, size_t, u64) override;
        int recvfrom(int, size_t*, void*, size_t, sockaddr*, u32*, u64) override;
        int send(int, size_t*, const void*, size_t, u64) override;
        int sendto(int, size_t*, const void*, size_t, const sockaddr*, u32, u64) override;
        int read(int, size_t*, void*, size_t, u64) override;
        int write(int, size_t*, const void*, size_t, u64) override;
        int writev(int, size_t*, iovec*, size_t, u64) override;
        int accept(int, int*, sockaddr*, u32*, u64) override;
        int connect(int, const sockaddr*, u32, u64) override;
        int pread(int, size_t*, void*, size_t, off_t) override;
        int pwrite(int, size_t*, const void*, size_t, off_t) override;
        int fsync(int) override;
        int fdatasync(int) override;
        u32 poll(PollFD, u64) override;
        PollerIface* createPoller(ObjPool*) override;
        void sleep(u64) override;
    };
}

void UringReq::complete(int result, IntrusiveList& ready) noexcept {
    res = result;
    ready.pushBack(task);
}

int UringReq::error() noexcept {
    return toErrno(res);
}

template <typename T>
int UringReq::readInto(T* out) noexcept {
    if (auto e = toErrno(res)) {
        return e;
    }

    *out = res;

    return 0;
}

Ring::Ring()
    : Ring(IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN | IORING_SETUP_R_DISABLED)
{
}

Ring::Ring(u32 flags) {
    struct io_uring_params params = {};

    params.flags = flags;

    if (io_uring_queue_init_params(256, this, &params) < 0) {
        throw 1;
    }
}

Ring::~Ring() noexcept {
    io_uring_queue_exit(this);
}

io_uring_sqe* Ring::getSqe() noexcept {
    auto sqe = io_uring_get_sqe(this);

    if (!sqe) {
        io_uring_submit(this);
        sqe = io_uring_get_sqe(this);
    }

    return sqe;
}

void Ring::getSqe(io_uring_sqe*& a, io_uring_sqe*& b) noexcept {
    a = io_uring_get_sqe(this);
    b = io_uring_get_sqe(this);

    if (!b) {
        io_uring_submit(this);
        a = io_uring_get_sqe(this);
        b = io_uring_get_sqe(this);
    }
}

void Ring::enable() noexcept {
    io_uring_enable_rings(this);
}

void Ring::sendMsg(int targetFd) noexcept {
    auto sqe = getSqe();

    io_uring_prep_msg_ring(sqe, targetFd, 0, WAKEUP_COOKIE, 0);
    sqe->flags |= IOSQE_CQE_SKIP_SUCCESS;
    io_uring_submit(this);
}

void Ring::wakeUp(int targetFd) noexcept {
    sendMsg(targetFd);
}

ExternalRing::ExternalRing(ObjPool* pool)
    : Ring(0)
    , mutex_(Mutex::create(pool))
{
}

void ExternalRing::wakeUp(int targetFd) noexcept {
    mutex_->lock();
    sendMsg(targetFd);
    mutex_->unlock();
}

UringCondVarImpl::UringCondVarImpl(Ring* ring, UringReactorImpl* reactor) noexcept
    : ring_(ring)
    , reactor_(reactor)
{
    ring_->enable();
}

bool UringCondVarImpl::cycle() noexcept {
    io_uring_submit_and_wait(ring_, 1);

    bool signaled = false;
    struct io_uring_cqe* cqe;
    IntrusiveList ready;

    while (io_uring_peek_cqe(ring_, &cqe) == 0) {
        auto ud = cqe->user_data;
        auto res = cqe->res;

        io_uring_cqe_seen(ring_, cqe);

        if (ud == WAKEUP_COOKIE) {
            signaled = true;
        } else {
            ((UringReqBase*)ud)->complete(res, ready);
        }
    }

    if (!ready.empty()) {
        reactor_->exec_->pool()->submitTasks(ready);
        reactor_->exec_->pool()->flushLocal();
    }

    return signaled;
}

void UringCondVarImpl::wait(Mutex* mutex) noexcept {
    UnlockGuard unlock(mutex);

    while (!cycle()) {
    }
}

void UringCondVarImpl::signal() noexcept {
    reactor_->currentRing()->wakeUp(ring_->ring_fd);
}

void UringCondVarImpl::broadcast() noexcept {
    signal();
}

UringReactorImpl::UringReactorImpl(ObjPool* pool, CoroExecutor* exec, size_t threads)
    : ext_(pool->make<ExternalRing>(pool))
    , exec_(exec)
{
    for (size_t i = 0; i < threads; ++i) {
        rings_.pushBack(pool->make<Ring>());
    }
}

ThreadPoolHooks* UringReactorImpl::hooks() {
    return this;
}

Ring* UringReactorImpl::currentRing() noexcept {
    size_t id;

    if (exec_->pool()->workerId(&id)) {
        return rings_[id];
    }

    return ext_;
}

template <typename F>
UringReq UringReactorImpl::submit(F prep, u64 deadlineUs) noexcept {
    UringReq req;

    if (deadlineUs == UINT64_MAX) {
        submitReq(req, prep);
    } else {
        submitReq(req, prep, deadlineUs);
    }

    return req;
}

template <typename Req, typename F>
void UringReactorImpl::submitReq(Req& req, F prep) noexcept {
    auto ring = currentRing();

    exec_->parkWith(makeRunable([&] {
        auto sqe = ring->getSqe();

        prep(sqe);

        io_uring_sqe_set_data(sqe, static_cast<UringReqBase*>(&req));
    }), &req.task);
}

template <typename Req, typename F>
void UringReactorImpl::submitReq(Req& req, F prep, u64 deadlineUs) noexcept {
    auto ring = currentRing();
    auto now = monotonicNowUs();
    auto ts = usToTimespec(deadlineUs - now);

    exec_->parkWith(makeRunable([&] {
        io_uring_sqe* sqe;
        io_uring_sqe* tsqe;

        ring->getSqe(sqe, tsqe);

        prep(sqe);

        io_uring_sqe_set_data(sqe, static_cast<UringReqBase*>(&req));
        sqe->flags |= IOSQE_IO_LINK;

        io_uring_prep_link_timeout(tsqe, &ts, 0);
        io_uring_sqe_set_data64(tsqe, WAKEUP_COOKIE);
        tsqe->flags |= IOSQE_CQE_SKIP_SUCCESS;
    }), &req.task);
}

Mutex* UringReactorImpl::createMutex(ObjPool* pool) {
    return Mutex::createSpinLock(pool);
}

CondVar* UringReactorImpl::createCondVar(ObjPool* pool, size_t index) {
    return pool->make<UringCondVarImpl>(rings_[index], this);
}

int UringReactorImpl::recv(int fd, size_t* nRead, void* buf, size_t len, u64 deadlineUs) {
    return submit([&](auto sqe) {
        io_uring_prep_recv(sqe, fd, buf, len, 0);
    }, deadlineUs).readInto(nRead);
}

int UringReactorImpl::recvfrom(int fd, size_t* nRead, void* buf, size_t len, sockaddr* addr, u32* addrLen, u64 deadlineUs) {
    struct msghdr msg = {};
    struct iovec iov = {buf, len};

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_name = addr;
    msg.msg_namelen = addrLen ? *addrLen : 0;

    auto res = submit([&](auto sqe) {
        io_uring_prep_recvmsg(sqe, fd, &msg, 0);
    }, deadlineUs).readInto(nRead);

    if (!res && addrLen) {
        *addrLen = msg.msg_namelen;
    }

    return res;
}

int UringReactorImpl::send(int fd, size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) {
    return submit([&](auto sqe) {
        io_uring_prep_send(sqe, fd, buf, len, MSG_NOSIGNAL);
    }, deadlineUs).readInto(nWritten);
}

int UringReactorImpl::sendto(int fd, size_t* nWritten, const void* buf, size_t len, const sockaddr* addr, u32 addrLen, u64 deadlineUs) {
    struct msghdr msg = {};
    struct iovec iov = {const_cast<void*>(buf), len};

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_name = const_cast<sockaddr*>(addr);
    msg.msg_namelen = addrLen;

    return submit([&](auto sqe) {
        io_uring_prep_sendmsg(sqe, fd, &msg, MSG_NOSIGNAL);
    }, deadlineUs).readInto(nWritten);
}

int UringReactorImpl::read(int fd, size_t* nRead, void* buf, size_t len, u64 deadlineUs) {
    return submit([&](auto sqe) {
        // offset = -1 tells io_uring "no offset, stream-style read",
        // which is the right semantics for non-seekable fds (TUN,
        // pipes, sockets used as streams).
        io_uring_prep_read(sqe, fd, buf, len, (u64)-1);
    }, deadlineUs).readInto(nRead);
}

int UringReactorImpl::write(int fd, size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) {
    return submit([&](auto sqe) {
        io_uring_prep_write(sqe, fd, buf, len, (u64)-1);
    }, deadlineUs).readInto(nWritten);
}

int UringReactorImpl::writev(int fd, size_t* nWritten, iovec* iov, size_t iovcnt, u64 deadlineUs) {
    return submit([&](auto sqe) {
        io_uring_prep_writev(sqe, fd, iov, iovcnt, 0);
    }, deadlineUs).readInto(nWritten);
}

int UringReactorImpl::accept(int fd, int* newFd, sockaddr* addr, u32* addrLen, u64 deadlineUs) {
    return submit([&](auto sqe) {
        io_uring_prep_accept(sqe, fd, addr, (socklen_t*)addrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    }, deadlineUs).readInto(newFd);
}

int UringReactorImpl::connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) {
    return submit([&](auto sqe) {
        io_uring_prep_connect(sqe, fd, addr, addrLen);
    }, deadlineUs).error();
}

int UringReactorImpl::pread(int fd, size_t* nRead, void* buf, size_t len, off_t offset) {
    return submit([&](auto sqe) {
        io_uring_prep_read(sqe, fd, buf, len, offset);
    }, UINT64_MAX).readInto(nRead);
}

int UringReactorImpl::pwrite(int fd, size_t* nWritten, const void* buf, size_t len, off_t offset) {
    return submit([&](auto sqe) {
        io_uring_prep_write(sqe, fd, buf, len, offset);
    }, UINT64_MAX).readInto(nWritten);
}

int UringReactorImpl::fsync(int fd) {
    return submit([&](auto sqe) {
        io_uring_prep_fsync(sqe, fd, 0);
    }, UINT64_MAX).error();
}

int UringReactorImpl::fdatasync(int fd) {
    return submit([&](auto sqe) {
        io_uring_prep_fsync(sqe, fd, IORING_FSYNC_DATASYNC);
    }, UINT64_MAX).error();
}

u32 UringReactorImpl::poll(PollFD pfd, u64 deadlineUs) {
    auto req = submit([&](auto sqe) {
        io_uring_prep_poll_add(sqe, pfd.fd, pfd.toPollEvents());
    }, deadlineUs);

    if (req.res <= 0) {
        return 0;
    }

    return PollFD::fromPollEvents(req.res);
}

PollerIface* UringReactorImpl::createPoller(ObjPool* pool) {
    return WaitablePoller::create(pool, this);
}

void UringReactorImpl::sleep(u64 deadlineUs) {
    TimeoutReq req;

    req.ts = usToTimespec(deadlineUs);

    submitReq(req, [&](auto sqe) {
        io_uring_prep_timeout(sqe, &req.ts, 0, IORING_TIMEOUT_ABS);
    });
}

// factory

IoReactor* stl::createIoUringReactor(ObjPool* pool, CoroExecutor* exec, size_t threads) {
    try {
        return pool->make<UringReactorImpl>(pool, exec, threads);
    } catch (int) {
        return nullptr;
    }
}
#else
IoReactor* stl::createIoUringReactor(ObjPool*, CoroExecutor*, size_t) {
    return nullptr;
}
#endif
./std/tst/args.cpp
#include "args.h"

#include <std/str/view.h>

using namespace stl;

void TestArgs::parse(StringView arg) {
    StringView key(arg.data() + 2, arg.length() - 2);
    StringView value(u8"1");

    if (auto eq = key.memChr('='); eq) {
        value = StringView(eq + 1, key.end());
        key = StringView(key.begin(), eq);
    }

    (*this)[key] = value;
}

TestArgs::TestArgs(ObjPool* pool, int argc, char** argv)
    : HashMap(pool)
{
    for (int i = 1; i < argc; ++i) {
        StringView arg(argv[i]);

        if (arg.startsWith(u8"--") && arg.length() > 2) {
            parse(arg);
        }
    }
}
./std/tst/ctx.cpp
#include "ctx.h"

#if __has_include(<execinfo.h>)
    #include <execinfo.h>
#endif

using namespace stl;

void Ctx::printTB() const {
#if __has_include(<execinfo.h>)
    void* frames[64];
    int n = backtrace(frames, 64);

    backtrace_symbols_fd(frames, n, 2);
#endif
}
./std/tst/ut.cpp
#include "ut.h"
#include "ctx.h"
#include "args.h"

#include <std/ios/sys.h>
#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/thr/pool.h>
#include <std/thr/guard.h>
#include <std/thr/mutex.h>
#include <std/sys/throw.h>
#include <std/dbg/color.h>
#include <std/dbg/panic.h>
#include <std/alg/range.h>
#include <std/map/treap.h>
#include <std/alg/qsort.h>
#include <std/sys/atomic.h>
#include <std/lib/vector.h>
#include <std/alg/minmax.h>
#include <std/str/builder.h>
#include <std/mem/obj_pool.h>

#include <stdio.h>
#include <stdlib.h>

using namespace stl;

namespace {
    struct Exc {
    };

    struct BufferedExecContext: public ExecContext {
        mutable StringBuilder buf_;
        const TestArgs* opts = nullptr;

        ZeroCopyOutput& output() const noexcept override {
            return buf_;
        }

        const TestArgs& args() const noexcept override {
            return *opts;
        }
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
        ObjPool::Ref pool_ = ObjPool::fromMemory();
        Vector<StringView> includes;
        Vector<StringView> excludes;
        TestArgs opts{pool_.mutPtr()};

        GetOpt(Ctx& ctx) noexcept;

        void help() const noexcept;

        bool matchesFilter(StringView testName) const noexcept;
        bool matchesFilterStrong(StringView testName) const noexcept;
        bool matchesExclude(StringView testName) const noexcept;

        size_t threads() const noexcept {
            if (auto sv = opts.find(StringView(u8"threads")); sv) {
                return (size_t)sv->stou();
            }

            return 0;
        }
    };

    struct TestTiming {
        u64 timeUs;
        TestFunc* test;
    };

    struct Tests: public Treap {
        Ctx* ctx = 0;
        GetOpt* opt = 0;
        size_t ok = 0;
        size_t err = 0;
        size_t skip = 0;
        size_t mute = 0;
        Vector<TestTiming> timings;
        Mutex* timingsMu = nullptr;

        bool cmp(void* l, void* r) const noexcept override {
            return compare(*(const TestFunc*)(l), *(const TestFunc*)(r));
        }

        static bool compare(const TestFunc& l, const TestFunc& r) noexcept {
            return l.suite() < r.suite() || (l.suite() == r.suite() && l.name() < r.name());
        }

        void execute();
        void handlePanic2();
        void run(Ctx& ctx_) noexcept;

        static Tests& instance() noexcept;

        void handlePanic1() noexcept {
            // outbuf->flush();
        }

        static void panicHandler1() noexcept {
            instance().handlePanic1();
        }

        static void panicHandler2() {
            instance().handlePanic2();
        }
    };
}

void Tests::run(Ctx& ctx_) noexcept {
    ctx = &ctx_;
    opt = new GetOpt(ctx_);
    execute();
}

void Tests::execute() {
    setPanicHandler1(panicHandler1);
    setPanicHandler2(panicHandler2);

    auto opool = ObjPool::fromMemory();
    auto pool = ThreadPool::simple(opool.mutPtr(), opt->threads());

    size_t topN = 0;

    if (auto sv = opt->opts.find(StringView(u8"top")); sv) {
        topN = (size_t)sv->stou();
    }

    auto mu = Mutex::create(opool.mutPtr());

    if (topN) {
        timingsMu = mu;
    }

    StringBuilder sb;

    visit([&](void* el) {
        auto test = (TestFunc*)el;

        sb.reset();
        sb << *test;

        if (test->name().startsWith(u8"_") && !opt->matchesFilterStrong(StringView(sb))) {
            ++mute;
        } else if (!opt->matchesFilter(StringView(sb))) {
            ++skip;
        } else {
            pool->submit([&, test] {
                BufferedExecContext bctx;

                bctx.opts = &opt->opts;

                u64 t0 = monotonicNowUs();

                if (::execute(test, bctx)) {
                    stdAtomicAddAndFetch(&ok, 1, MemoryOrder::Relaxed);
                } else {
                    stdAtomicAddAndFetch(&err, 1, MemoryOrder::Relaxed);
                }

                if (timingsMu) {
                    u64 dt = monotonicNowUs() - t0;
                    LockGuard guard(timingsMu);
                    timings.pushBack(TestTiming{dt, test});
                }

                stdoutStream().write(bctx.buf_.data(), bctx.buf_.length());
            });
        }
    });

    pool->join();

    auto&& outb = sysO;

    if (!timings.empty()) {
        quickSort(timings.mutBegin(), timings.mutEnd(), [](const TestTiming& a, const TestTiming& b) {
            return a.timeUs > b.timeUs;
        });

        outb << endL
             << StringView(u8"Slowest tests:")
             << endL;

        size_t n = ::min(topN, timings.length());

        for (size_t i = 0; i < n; ++i) {
            auto& t = timings[i];

            outb << StringView(u8"  ")
                 << t.timeUs
                 << StringView(u8" us ")
                 << *t.test
                 << endL;
        }

        outb << endL;
    }

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

void Tests::handlePanic2() {
    ctx->printTB();
    fflush(stdout);
    fflush(stderr);
    throw Exc();
}

Tests& Tests::instance() noexcept {
    static auto res = new Tests();

    return *res;
}

GetOpt::GetOpt(Ctx& ctx) noexcept {
    for (int i = 1; i < ctx.argc; ++i) {
        StringView arg(ctx.argv[i]);

        if (arg.startsWith(u8"--") && arg.length() > 2) {
            opts.parse(arg);
        } else if (arg.startsWith(u8"-") && arg.length() > 1) {
            excludes.pushBack(StringView(arg.data() + 1, arg.length() - 1));
        } else {
            includes.pushBack(arg);
        }
    }

    help();
}

void GetOpt::help() const noexcept {
    if (!opts.find(StringView(u8"help"))) {
        return;
    }

    auto out = sysE;

    out << StringView(u8"Usage: test-binary [FILTER...] [--OPTION[=VALUE]]") << endL
        << endL
        << StringView(u8"Filters:") << endL
        << StringView(u8"  Suite::Test    run tests whose full name starts with the prefix") << endL
        << StringView(u8"  -Suite::Test   exclude tests matching the prefix") << endL
        << StringView(u8"  (tests prefixed with _ are muted unless explicitly included)") << endL
        << endL
        << StringView(u8"Options:") << endL
        << StringView(u8"  --help         print this help") << endL
        << StringView(u8"  --threads=N    run tests in parallel using N threads") << endL
        << StringView(u8"  --top=N        show N slowest tests") << endL
        << StringView(u8"  --OPT          equivalent to --OPT=1") << endL
        << StringView(u8"  --OPT=VALUE    set option OPT to VALUE") << endL
        << flsH;

    exit(0);
}

bool GetOpt::matchesFilter(StringView testName) const noexcept {
    if (matchesExclude(testName)) {
        return false;
    }

    if (includes.empty()) {
        return true;
    }

    return matchesFilterStrong(testName);
}

bool GetOpt::matchesFilterStrong(StringView testName) const noexcept {
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

bool GetOpt::matchesExclude(StringView testName) const noexcept {
    for (auto prefix : range(excludes)) {
        if (testName.startsWith(prefix)) {
            return true;
        }
    }

    return false;
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
./std/typ/intrin.cpp
#include "intrin.h"
./std/typ/support.cpp
#include "support.h"
./tst/channel_pipeline.cpp
#include <std/thr/coro.h>
#include <std/lib/vector.h>
#include <std/thr/channel.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    void dutchRudder(int nStages, int nMessages) {
        auto opool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(opool.mutPtr(), 8);

        Vector<Channel*> chArr;

        for (int i = 0; i <= nStages; ++i) {
            chArr.pushBack(Channel::create(opool.mutPtr(), exec, (size_t)5));
        }

        for (int i = 0; i < nStages; ++i) {
            exec->spawn([in = chArr[i], out = chArr[i + 1]] {
                void* batch[8];

                for (;;) {
                    size_t n = in->dequeue(batch, 8);

                    if (!n) {
                        break;
                    }

                    out->enqueue(batch, n);
                }

                out->close();
            });
        }

        i64 sum = 0;

        exec->spawn([ch = chArr[nStages], &sum] {
            void* batch[8];

            for (;;) {
                size_t n = ch->dequeue(batch, 8);

                if (!n) {
                    break;
                }

                for (size_t j = 0; j < n; ++j) {
                    sum += (i64)(uintptr_t)batch[j];
                }
            }
        });

        exec->spawn([ch = chArr[0], nMessages] {
            for (int i = 1; i <= nMessages; ++i) {
                ch->enqueue((void*)(uintptr_t)i);
            }
            ch->close();
        });

        exec->join();
    }
}

int main() {
    dutchRudder(2000, 200000);
}
./tst/coro_pipe.cpp
#include <std/sys/fd.h>
#include <std/thr/coro.h>
#include <std/lib/vector.h>
#include <std/thr/poll_fd.h>
#include <std/mem/obj_pool.h>
#include <std/thr/io_reactor.h>

#include <errno.h>
#include <unistd.h>

using namespace stl;

int main() {
    auto opool = ObjPool::fromMemory();
    auto exec = CoroExecutor::create(opool.mutPtr(), 8);
    const int N = 200;
    const size_t TOTAL = 500 * 1024 * 1024;

    struct Pipe {
        ScopedFD r, w;
        Pipe() {
            createPipeFD(r, w);
            r.setNonBlocking();
            w.setNonBlocking();
        }
        ~Pipe() noexcept {
        }
    };

    Vector<Pipe*> pipes;

    for (int i = 0; i < N; i++) {
        auto p = opool->make<Pipe>();
        pipes.pushBack(p);

        int rfd = p->r.get();
        int wfd = p->w.get();

        CoroExecutor* ex = exec;

        exec->spawnRun(SpawnParams().setStackSize(64 * 1024).setRunable([ex, rfd] {
            char buf[16 * 1024];
            size_t received = 0;

            while (received < TOTAL) {
                ssize_t n = ::read(rfd, buf, sizeof(buf));

                if (n > 0) {
                    received += (size_t)n;
                } else if (errno == EAGAIN) {
                    ex->io()->poll({rfd, PollFlag::In}, UINT64_MAX);
                }
            }
        }));

        exec->spawnRun(SpawnParams().setStackSize(64 * 1024).setRunable([ex, wfd] {
            char buf[16 * 1024] = {};
            size_t sent = 0;

            while (sent < TOTAL) {
                size_t rem = TOTAL - sent;
                ssize_t n = ::write(wfd, buf, rem < sizeof(buf) ? rem : sizeof(buf));

                if (n > 0) {
                    sent += (size_t)n;
                } else if (errno == EAGAIN) {
                    ex->io()->poll({wfd, PollFlag::Out}, UINT64_MAX);
                }
            }
        }));
    }

    exec->join();
}
./tst/coro_sw.cpp
#include <std/thr/coro.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    void doW(int work) {
        for (volatile int i = 0; i < work; i = i + 1) {
        }
    }
}

int main() {
    const int depth = 22;
    const int work = 999;

    auto pool = ObjPool::fromMemory();
    auto exec = CoroExecutor::create(pool.mutPtr(), 16);

    int counter2 = 0;

    auto run = [&](auto& self, int d) -> void {
        stdAtomicAddAndFetch(&counter2, 1, MemoryOrder::Relaxed);

        doW(work);

        if (d > 0) {
            exec->spawnRun(SpawnParams().setStackSize(2000).setRunable([&, d] {
                self(self, d - 1);
            }));
        }

        exec->yield();

        if (d > 0) {
            exec->spawnRun(SpawnParams().setStackSize(2000).setRunable([&, d] {
                self(self, d - 1);
            }));
        }

        doW(work);
    };

    exec->spawn([&] {
        run(run, depth);
    });

    exec->join();
}
./tst/dns_stress.cpp
#include <std/tst/args.h>
#include <std/thr/coro.h>
#include <std/thr/semaphore.h>
#include <std/dns/iface.h>
#include <std/dns/config.h>
#include <std/lib/vector.h>
#include <std/mem/obj_pool.h>

#include <stdio.h>
#include <string.h>

using namespace stl;

int main(int argc, char** argv) {
    auto pool = ObjPool::fromMemory();
    TestArgs a(pool.mutPtr(), argc, argv);
    size_t threads = 8;

    if (auto sv = a.find(u8"coro-threads"); sv) {
        threads = sv->stou();
    }

    DnsConfig dnsCfg;

    if (auto sv = a.find(u8"dns-family"); sv) {
        dnsCfg.family = sv->stou();
    }

    if (auto sv = a.find(u8"dns-timeout"); sv) {
        dnsCfg.timeout = sv->stou();
    }

    if (auto sv = a.find(u8"dns-tries"); sv) {
        dnsCfg.tries = sv->stou();
    }

    if (auto sv = a.find(u8"dns-udp-max-queries"); sv) {
        dnsCfg.udpMaxQueries = sv->stou();
    }

    if (a.find(u8"dns-tcp")) {
        dnsCfg.tcp = true;
    }

    if (auto sv = a.find(u8"dns-server"); sv) {
        dnsCfg.server = *sv;
    }

    auto exec = CoroExecutor::create(pool.mutPtr(), threads);

    auto& sem = *Semaphore::create(pool.mutPtr(), threads * 20, exec);

    Vector<DnsResolver*> resolvers;

    for (size_t j = 0; j < threads; ++j) {
        resolvers.pushBack(DnsResolver::create(pool.mutPtr(), exec, nullptr, dnsCfg));
    }

    for (int i = 0; i < 100000; ++i) {
        exec->spawn([&, i] {
            auto rpool = ObjPool::fromMemory();
            char buf[64];

            snprintf(buf, sizeof(buf), "host%d.test.invalid", i);

            sem.wait();
            resolvers[i % threads]->resolve(rpool.mutPtr(), StringView((const u8*)buf, strlen(buf)));
            sem.post();
        });
    }

    exec->join();
}
./tst/http_load.cpp
#include <std/map/map.h>
#include <std/ios/out.h>
#include <std/ios/sys.h>
#include <std/sys/crt.h>
#include <std/tst/args.h>
#include <std/thr/coro.h>
#include <std/thr/async.h>
#include <std/dbg/color.h>
#include <std/alg/defer.h>
#include <std/dbg/insist.h>
#include <std/lib/buffer.h>
#include <std/ios/in_buf.h>
#include <std/dns/iface.h>
#include <std/dns/config.h>
#include <std/dns/result.h>
#include <std/dns/record.h>
#include <std/str/builder.h>
#include <std/thr/channel.h>
#include <std/mem/obj_pool.h>
#include <std/thr/wait_group.h>
#include <std/ios/stream_tcp.h>
#include <std/net/tcp_socket.h>
#include <std/net/http_client.h>
#include <std/net/http_reason.h>

#include <sys/socket.h>
#include <netinet/in.h>

using namespace stl;

namespace {
    struct ReqRecord {
        u32 status;
        u32 elapsedUs;
    };

    struct ReqBatch {
        static constexpr u32 CAP = 255;

        ReqRecord records[CAP];
        u64 len;

        ReqBatch()
            : len(0)
        {
        }
    };
}

int main(int argc, char** argv) {
    auto pool = ObjPool::fromMemory();
    TestArgs a(pool.mutPtr(), argc, argv);

    auto urlArg = a.find(StringView("url"));

    if (!urlArg) {
        sysE << StringView(u8"usage: http_load --url=http://host:port/path [--coros=100] [--threads=4] [--duration=10] [--payload=0]") << endL;
        return 1;
    }

    StringView url = *urlArg;

    if (url.startsWith(StringView("http://"))) {
        url = StringView(url.data() + 7, url.length() - 7);
    }

    StringView hostPort, path;

    if (!url.split('/', hostPort, path)) {
        hostPort = url;
    }

    StringView host, portStr;
    u16 port = 80;

    if (hostPort.split(':', host, portStr)) {
        port = (u16)portStr.stou();
    } else {
        host = hostPort;
    }

    u32 numCoros = 100;
    u32 numThreads = 4;
    u32 durationSec = 10;
    u32 payloadLen = 0;

    if (auto v = a.find(StringView("coros"))) {
        numCoros = (u32)v->stou();
    }

    if (auto v = a.find(StringView("threads"))) {
        numThreads = (u32)v->stou();
    }

    if (auto v = a.find(StringView("duration"))) {
        durationSec = (u32)v->stou();
    }

    if (auto v = a.find(StringView("payload"))) {
        payloadLen = (u32)v->stou();
    }

    auto exec = CoroExecutor::create(pool.mutPtr(), numThreads);

    auto resolver = DnsResolver::create(pool.mutPtr(), exec, nullptr, DnsConfig());

    auto dns = async(exec, [&] {
                   return resolver->resolve(pool.mutPtr(), host);
               }).wait();

    if (!dns->ok() || !dns->record) {
        sysE << StringView(u8"dns resolve failed: ") << dns << endL;
        return 1;
    }

    auto rec = dns->record;

    if (rec->family == AF_INET) {
        ((sockaddr_in*)rec->addr)->sin_port = htons(port);
    } else {
        ((sockaddr_in6*)rec->addr)->sin6_port = htons(port);
    }

    Buffer payloadBuf;

    for (u32 i = 0; i < payloadLen; ++i) {
        u8 x = 'X';
        payloadBuf.append(&x, 1);
    }

    StringView payloadVal(payloadBuf);

    StringView fullPath;

    if (path.empty()) {
        fullPath = StringView("/");
    } else {
        StringBuilder pathBuf;

        pathBuf << StringView(u8"/") << path;

        fullPath = pool.mutPtr()->intern(StringView(pathBuf));
    }

    auto G = Color::bright(AnsiColor::Green);
    auto C = Color::bright(AnsiColor::Cyan);
    auto W = Color::bright(AnsiColor::White);
    auto R = Color::reset();

    sysE << endL;
    sysE << W << StringView(u8"host:     ") << G << host << R << endL;
    sysE << W << StringView(u8"resolved: ") << G << rec << R << endL;
    sysE << W << StringView(u8"port:     ") << G << (u64)port << R << endL;
    sysE << W << StringView(u8"path:     ") << G << fullPath << R << endL;
    sysE << W << StringView(u8"threads:  ") << G << (u64)numThreads << R << endL;
    sysE << W << StringView(u8"coros:    ") << G << (u64)numCoros << R << endL;
    sysE << W << StringView(u8"duration: ") << G << (u64)durationSec << StringView(u8"s") << R << endL;
    sysE << W << StringView(u8"payload:  ") << G << (u64)payloadLen << R << endL;

    auto& ch = *Channel::create(pool.mutPtr(), exec, numCoros * 4);
    auto& wg = *WaitGroup::create(pool.mutPtr(), 0, exec);

    u64 startUs = monotonicNowUs();
    u64 deadlineUs = startUs + (u64)durationSec * 1000000;

    u64 totalReqs = 0;
    Map<u64, u64> statuses(pool.mutPtr());
    u64 hist[64] = {};

    exec->spawn([&] {
        void* v = nullptr;

        while (ch.dequeue(&v)) {
            auto batch = (ReqBatch*)v;

            for (u32 j = 0; j < batch->len; ++j) {
                ++statuses[(u64)batch->records[j].status];
                ++totalReqs;

                u32 dt = batch->records[j].elapsedUs;
                u32 bucket = 0;

                if (dt > 0) {
                    bucket = 31 - __builtin_clz(dt);
                }

                ++hist[bucket];
            }

            delete batch;
        }
    });

    for (u32 i = 0; i < numCoros; ++i) {
        wg.inc();

        exec->spawn([&] {
            STD_DEFER {
                wg.done();
            };

            u32 addrLen = (rec->family == AF_INET) ? (u32)sizeof(sockaddr_in) : (u32)sizeof(sockaddr_in6);

            int cfd;

            STD_INSIST(TcpSocket::connectInf(&cfd, exec, rec->addr, addrLen) == 0);

            TcpSocket sock(cfd, exec);

            STD_DEFER {
                sock.close();
            };

            TcpStream stream(sock);
            InBuf in(stream);

            Buffer body;
            auto batch = new ReqBatch();

            while (monotonicNowUs() < deadlineUs) {
                auto rpool = ObjPool::fromMemory();
                auto req = HttpClientRequest::create(rpool.mutPtr(), &in, &stream);

                req->setPath(fullPath);
                req->addHeader(StringView("Host"), host);
                req->addHeader(StringView("Connection"), StringView("keep-alive"));

                if (payloadLen > 0) {
                    req->addHeader(StringView("X-Payload"), payloadVal);
                }

                u64 t0 = monotonicNowUs();

                req->endHeaders();

                auto resp = req->response();
                u32 st = resp->status();

                if (st == 0) {
                    break;
                }

                resp->body()->readAll(body);

                u64 t1 = monotonicNowUs();

                batch->records[batch->len].status = st;
                batch->records[batch->len].elapsedUs = t1 - t0;
                ++batch->len;

                if (batch->len == ReqBatch::CAP) {
                    ch.enqueue(batch);
                    batch = new ReqBatch();
                }
            }

            if (batch->len > 0) {
                ch.enqueue(batch);
            } else {
                delete batch;
            }
        });
    }

    exec->spawn([&] {
        wg.wait();
        ch.close();
    });

    exec->join();

    u64 elapsedUs = monotonicNowUs() - startUs;
    double elapsedSec = (double)elapsedUs / 1000000.0;
    double rps = (double)totalReqs / elapsedSec;

    sysE << endL;
    sysE << W << StringView(u8"requests: ") << G << totalReqs << endL;
    sysE << W << StringView(u8"elapsed:  ") << G << elapsedSec << StringView(u8"s") << endL;
    sysE << W << StringView(u8"rps:      ") << G << rps << endL;
    sysE << R << endL;

    sysE << W << StringView(u8"status codes:") << R << endL;

    statuses.visit([&](u64 code, u64& count) {
        auto sc = (code >= 200 && code < 300)
                      ? Color::bright(AnsiColor::Green)
                  : (code >= 400)
                      ? Color::bright(AnsiColor::Red)
                      : Color::bright(AnsiColor::Yellow);

        sysE << StringView(u8"  ") << sc << code
             << StringView(u8" ") << reasonPhrase((u32)code)
             << R << StringView(u8": ") << W << count
             << R << endL;
    });

    sysE << endL;
    sysE << W << StringView(u8"latency histogram (us):") << R << endL;

    u32 lastNonZero = 0;

    for (u32 i = 0; i < 64; ++i) {
        if (hist[i] > 0) {
            lastNonZero = i;
        }
    }

    for (u32 i = 0; i <= lastNonZero; ++i) {
        u64 lo = (i == 0) ? 0 : ((u64)1 << i);
        u64 hi = ((u64)1 << (i + 1)) - 1;

        auto bc = (hist[i] > 0)
                      ? Color::bright(AnsiColor::Cyan)
                      : Color::dark(AnsiColor::White);

        sysE << C << StringView(u8"  [")
             << lo << StringView(u8" .. ") << hi
             << StringView(u8"] ") << bc << hist[i]
             << R << endL;
    }
}
./tst/http_serve_files.cpp
#include <std/sys/fd.h>
#include <std/tst/args.h>
#include <std/thr/coro.h>
#include <std/lib/buffer.h>
#include <std/net/http_srv.h>
#include <std/mem/obj_pool.h>
#include <std/thr/wait_group.h>
#include <std/ios/in_fd_coro.h>
#include <std/net/ssl_socket.h>

#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace stl;

namespace {
    const char testCert[] =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBfzCCASWgAwIBAgIUUQjugcnFUjBED4h+YaPFRr7pMlowCgYIKoZIzj0EAwIw\n"
        "FDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MDMyMzE0MjI1MFoYDzIxMjYwMjI3\n"
        "MTQyMjUwWjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwWTATBgcqhkjOPQIBBggqhkjO\n"
        "PQMBBwNCAARfsZNKls055081/xImV4diaUrCimYW2k0m7Rhq/B5xBjWP5ETfBy3X\n"
        "aoUXGA02bImZaTSTLd43TWTCB5IbRkngo1MwUTAdBgNVHQ4EFgQURCfdIwDIDcdE\n"
        "hrObYyq2RJsofgswHwYDVR0jBBgwFoAURCfdIwDIDcdEhrObYyq2RJsofgswDwYD\n"
        "VR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNIADBFAiEAiISrnWrLf3ZHEnIfmPcM\n"
        "XMJDSo3hPE486n7a40YQQw8CIEICeaFmiGyD2MvrpAe+S6k80bGFOJeWDOzwJmP7\n"
        "nZq5\n"
        "-----END CERTIFICATE-----";

    const char testKey[] =
        "-----BEGIN PRIVATE KEY-----\n"
        "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgw0VlkoyRhsiGGmYa\n"
        "R4bbSCCAbt0MrRex/LkuoYW1/myhRANCAARfsZNKls055081/xImV4diaUrCimYW\n"
        "2k0m7Rhq/B5xBjWP5ETfBy3XaoUXGA02bImZaTSTLd43TWTCB5IbRkng\n"
        "-----END PRIVATE KEY-----";

    sockaddr_in makeAddr(u16 port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        return addr;
    }
}

int main(int argc, char** argv) {
    auto pool = ObjPool::fromMemory();
    TestArgs a(pool.mutPtr(), argc, argv);
    auto exec = CoroExecutor::create(pool.mutPtr(), 8);
    auto sslCtx = SslCtx::create(pool.mutPtr(), StringView(testCert), StringView(testKey));

    struct Handler: HttpServe {
        CoroExecutor* exec;
        SslCtx* sslCtx;

        SslCtx* ssl() override {
            return sslCtx;
        }

        void serve(HttpServerResponse& resp) override {
            auto req = resp.request();

            ScopedFD fd(::open(Buffer(req->path()).cStr(), O_RDONLY));

            if (fd.get() < 0) {
                resp.setStatus(404);
                resp.addHeader(StringView("Content-Length"), StringView("0"));
                resp.endHeaders();

                return;
            }

            resp.endHeaders();

            CoroFDInput(fd, exec).sendTo(*resp.out());
            resp.out()->finish();
        }
    } handler;

    handler.exec = exec;
    handler.sslCtx = sslCtx;

    u16 port = 18080;

    if (auto v = a.find(StringView("port"))) {
        port = (u16)v->stou();
    }

    auto addr = makeAddr(port);

    auto& wg = *WaitGroup::create(pool.mutPtr(), 0, exec);

    serve(
        pool.mutPtr(),
        {
            .handler = &handler,
            .exec = exec,
            .wg = &wg,
            .addr = (const sockaddr*)&addr,
            .addrLen = sizeof(addr),
        });

    exec->spawn([&] {
        wg.wait();
    });

    exec->join();
}
./tst/http_serve_ok.cpp
#include <std/ios/sys.h>
#include <std/tst/args.h>
#include <std/thr/coro.h>
#include <std/thr/async.h>
#include <std/dns/iface.h>
#include <std/dns/config.h>
#include <std/dns/result.h>
#include <std/dns/record.h>
#include <std/mem/obj_pool.h>
#include <std/net/http_srv.h>
#include <std/net/ssl_socket.h>
#include <std/thr/wait_group.h>

#include <netinet/in.h>

using namespace stl;

namespace {
    const char testCert[] =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBfzCCASWgAwIBAgIUUQjugcnFUjBED4h+YaPFRr7pMlowCgYIKoZIzj0EAwIw\n"
        "FDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MDMyMzE0MjI1MFoYDzIxMjYwMjI3\n"
        "MTQyMjUwWjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwWTATBgcqhkjOPQIBBggqhkjO\n"
        "PQMBBwNCAARfsZNKls055081/xImV4diaUrCimYW2k0m7Rhq/B5xBjWP5ETfBy3X\n"
        "aoUXGA02bImZaTSTLd43TWTCB5IbRkngo1MwUTAdBgNVHQ4EFgQURCfdIwDIDcdE\n"
        "hrObYyq2RJsofgswHwYDVR0jBBgwFoAURCfdIwDIDcdEhrObYyq2RJsofgswDwYD\n"
        "VR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNIADBFAiEAiISrnWrLf3ZHEnIfmPcM\n"
        "XMJDSo3hPE486n7a40YQQw8CIEICeaFmiGyD2MvrpAe+S6k80bGFOJeWDOzwJmP7\n"
        "nZq5\n"
        "-----END CERTIFICATE-----";

    const char testKey[] =
        "-----BEGIN PRIVATE KEY-----\n"
        "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgw0VlkoyRhsiGGmYa\n"
        "R4bbSCCAbt0MrRex/LkuoYW1/myhRANCAARfsZNKls055081/xImV4diaUrCimYW\n"
        "2k0m7Rhq/B5xBjWP5ETfBy3XaoUXGA02bImZaTSTLd43TWTCB5IbRkng\n"
        "-----END PRIVATE KEY-----";
}

int main(int argc, char** argv) {
    auto pool = ObjPool::fromMemory();
    TestArgs a(pool.mutPtr(), argc, argv);
    auto exec = CoroExecutor::create(pool.mutPtr(), 8);
    auto sslCtx = SslCtx::create(pool.mutPtr(), StringView(testCert), StringView(testKey));

    struct Handler: HttpServe {
        SslCtx* sslCtx;

        SslCtx* ssl() override {
            return sslCtx;
        }

        void serve(HttpServerResponse& resp) override {
            resp.setStatus(200);
            resp.addHeader(StringView("Content-Length"), StringView("0"));
            resp.addHeader(StringView("Payload"), StringView("12345678901234567890123456789012345"));
            resp.endHeaders();
            resp.out()->finish();
        }
    } handler;

    handler.sslCtx = sslCtx;

    u16 port = 18080;

    if (auto v = a.find(StringView("port"))) {
        port = (u16)v->stou();
    }

    auto resolver = DnsResolver::create(pool.mutPtr(), exec, nullptr, DnsConfig());

    auto dns = async(exec, [&] {
                   return resolver->resolve(pool.mutPtr(), StringView("localhost"));
               }).wait();

    if (!dns->ok() || !dns->record) {
        sysE << StringView(u8"dns resolve failed: ") << dns << endL;
        return 1;
    }

    sysE << StringView(u8"resolved: ") << dns->record << endL;

    auto& wg = *WaitGroup::create(pool.mutPtr(), 0, exec);

    for (auto rec = dns->record; rec; rec = rec->next) {
        if (rec->family == AF_INET) {
            ((sockaddr_in*)rec->addr)->sin_port = htons(port);
        } else {
            ((sockaddr_in6*)rec->addr)->sin6_port = htons(port);
        }

        u32 addrLen = (rec->family == AF_INET) ? (u32)sizeof(sockaddr_in) : (u32)sizeof(sockaddr_in6);

        sysE << StringView(u8"serving on ") << *rec << StringView(u8":") << (u64)port << endL;

        serve(
            pool.mutPtr(),
            {
                .handler = &handler,
                .exec = exec,
                .wg = &wg,
                .addr = rec->addr,
                .addrLen = addrLen,
            });
    }

    exec->spawn([&] {
        wg.wait();
    });

    exec->join();
}
./tst/io_uring_msg.cpp
#include <std/sys/crt.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#if __has_include(<liburing.h>)
#include <liburing.h>

static constexpr unsigned long long COOKIE = 0xCAFE;

struct Ring {
    struct io_uring ring;

    void initSimple() {
        if (io_uring_queue_init(64, &ring, 0) < 0) {
            printf("simple init failed\n");
            exit(1);
        }
    }

    void initDeferred() {
        struct io_uring_params params = {};

        params.flags = IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN | IORING_SETUP_R_DISABLED;

        if (io_uring_queue_init_params(64, &ring, &params) < 0) {
            printf("deferred init failed\n");
            exit(1);
        }
    }

    void enable() {
        io_uring_enable_rings(&ring);
    }

    void sendMsg(int targetFd) {
        auto sqe = io_uring_get_sqe(&ring);

        io_uring_prep_msg_ring(sqe, targetFd, 0, COOKIE, 0);

        int r = io_uring_submit(&ring);

        printf("sendMsg: from fd=%d to fd=%d, submit r=%d\n", ring.ring_fd, targetFd, r);
    }

    void waitForCookie() {
        for (;;) {
            io_uring_submit_and_wait(&ring, 1);

            struct io_uring_cqe* cqe;

            while (io_uring_peek_cqe(&ring, &cqe) == 0) {
                auto ud = cqe->user_data;
                auto res = cqe->res;

                io_uring_cqe_seen(&ring, cqe);

                printf("  cqe: res=%d ud=%llu\n", res, (unsigned long long)ud);

                if (ud == COOKIE) {
                    return;
                }
            }
        }
    }

    void destroy() {
        io_uring_queue_exit(&ring);
    }
};

static Ring ext;
static Ring worker;

static void* workerThread(void*) {
    worker.enable();

    printf("worker: enabled ring fd=%d, waiting...\n", worker.ring.ring_fd);

    worker.waitForCookie();

    printf("worker: got cookie!\n");

    return nullptr;
}

int main() {
    ext.initSimple();
    worker.initDeferred();

    printf("ext fd=%d, worker fd=%d\n", ext.ring.ring_fd, worker.ring.ring_fd);

    printf("\n--- Test: ext -> deferred worker (R_DISABLED + enable from worker thread) ---\n");

    pthread_t th;

    pthread_create(&th, nullptr, workerThread, nullptr);
    usleep(50000);

    ext.sendMsg(worker.ring.ring_fd);
    pthread_join(th, nullptr);

    printf("\nOK\n");

    ext.destroy();
    worker.destroy();

    return 0;
}
#else
int main() {
}
#endif
./tst/perf_hash.cpp
#include <std/sym/i_map.h>
#include <std/mem/obj_pool.h>

using namespace stl;

int main() {
    auto pool = ObjPool::fromMemory();
    IntMap<int> m(pool.mutPtr());

    for (size_t i = 0; i < 200000000; ++i) {
        m[i] = i;
    }
}
./tst/perf_malloc.cpp
#include <stdlib.h>
#include <std/sys/types.h>

int main() {
    volatile size_t res = 0;

    for (size_t i = 0; i < 100000000; ++i) {
        auto p = malloc(256);
        res += (size_t)p;
        free(p);
    }
}
./tst/perf_map.cpp
#include <std/map/map.h>
#include <std/mem/obj_pool.h>

using namespace stl;

int main() {
    auto pool = ObjPool::fromMemory();
    Map<int, int> m(pool.mutPtr());

    for (size_t i = 0; i < 20000000; ++i) {
        m[i] = i;
    }
}
./tst/perf_objpool.cpp
#include <std/mem/obj_pool.h>

using namespace stl;

int main() {
    volatile size_t res = 0;

    for (size_t i = 0; i < 100000000; ++i) {
        auto p = ObjPool::fromMemoryRaw();
        res += (size_t)p;
        delete p;
    }
}
./tst/pool_ss.cpp
#include <std/thr/pool.h>
#include <std/thr/mutex.h>
#include <std/thr/guard.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>
#include <std/thr/cond_var.h>

using namespace stl;

namespace {
    void doW(int work) {
        for (volatile int i = 0; i < work; i = i + 1) {
        }
    }

    struct StressState {
        ThreadPool* pool;
        int work;
        int counter;
        ObjPool::Ref opool;
        Mutex* mutex;
        CondVar* condVar;

        StressState(ThreadPool* p, int w)
            : pool(p)
            , work(w)
            , counter(1)
            , opool(ObjPool::fromMemory())
            , mutex(Mutex::create(opool.mutPtr()))
            , condVar(CondVar::create(opool.mutPtr()))
        {
        }
    };

    struct StressTask {
        StressState* state_;
        int depth_;

        StressTask(StressState* s, int d)
            : state_(s)
            , depth_(d)
        {
        }

        void doWork() noexcept {
            doW(state_->work);
        }

        void schedule() {
            stdAtomicAddAndFetch(&state_->counter, 1, MemoryOrder::Relaxed);
            auto t = new StressTask(state_, depth_ - 1);
            state_->pool->submit([t] {
                t->run();
            });
        }

        void run() noexcept {
            doWork();
            if (depth_ > 0) {
                schedule();
            }
            doWork();
            if (depth_ > 0) {
                schedule();
            }
            doWork();

            auto state = state_;
            delete this;

            if (stdAtomicSubAndFetch(&state->counter, 1, MemoryOrder::Release) == 0) {
                LockGuard lock(state->mutex);
                state->condVar->signal();
            }
        }
    };
}

int main() {
    auto opool = ObjPool::fromMemory();
    auto pool = ThreadPool::simple(opool.mutPtr(), 16);
    StressState state(pool, 250);

    auto task = new StressTask(&state, 25);
    pool->submit([task] {
        task->run();
    });

    {
        LockGuard lock(state.mutex);

        while (stdAtomicFetch(&state.counter, MemoryOrder::Acquire) > 0) {
            state.condVar->wait(state.mutex);
        }
    }

    pool->join();
}
./tst/pool_sw.cpp
#include <std/thr/pool.h>
#include <std/thr/mutex.h>
#include <std/thr/guard.h>
#include <std/sys/atomic.h>
#include <std/thr/cond_var.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    void doW(int work) {
        for (volatile int i = 0; i < work; i = i + 1) {
        }
    }

    struct StressState {
        ThreadPool* pool;
        int work;
        int counter;
        ObjPool::Ref opool;
        Mutex* mutex;
        CondVar* condVar;

        StressState(ThreadPool* p, int w)
            : pool(p)
            , work(w)
            , counter(1)
            , opool(ObjPool::fromMemory())
            , mutex(Mutex::create(opool.mutPtr()))
            , condVar(CondVar::create(opool.mutPtr()))
        {
        }
    };

    struct StressTask {
        StressState* state_;
        int depth_;

        StressTask(StressState* s, int d)
            : state_(s)
            , depth_(d)
        {
        }

        void doWork() noexcept {
            doW(state_->work);
        }

        void schedule() {
            stdAtomicAddAndFetch(&state_->counter, 1, MemoryOrder::Relaxed);
            auto t = new StressTask(state_, depth_ - 1);
            state_->pool->submit([t] {
                t->run();
            });
        }

        void run() noexcept {
            doWork();
            if (depth_ > 0) {
                schedule();
            }
            doWork();
            if (depth_ > 0) {
                schedule();
            }
            doWork();

            auto state = state_;
            delete this;

            if (stdAtomicSubAndFetch(&state->counter, 1, MemoryOrder::Release) == 0) {
                LockGuard lock(state->mutex);
                state->condVar->signal();
            }
        }
    };
}

int main() {
    auto opool = ObjPool::fromMemory();
    auto pool = ThreadPool::workStealing(opool.mutPtr(), 16);
    StressState state(pool, 250);

    auto task = new StressTask(&state, 25);
    pool->submit([task] {
        task->run();
    });
    pool->join();
}
./tst/test.cpp
#include <std/tst/ut.h>
#include <std/tst/ctx.h>

#if __has_include(<cpptrace/cpptrace.hpp>)
    #undef noexcept
    #include <cpptrace/cpptrace.hpp>
#endif

using namespace stl;

namespace {
    struct MyCtx: public Ctx {
        MyCtx(int c, char** v) {
            argc = c;
            argv = v;
        }

#if __has_include(<cpptrace/cpptrace.hpp>)
        void printTB() const override {
            cpptrace::generate_trace().print();
        }
#endif
    };
}

int main(int argc, char** argv) {
    MyCtx(argc, argv).run();
}
./all.cpp
