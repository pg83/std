#pragma once

#include <std/sys/types.h>

struct iovec;
struct sockaddr;

namespace stl {
    class ObjPool;
    struct ScopedFD;
    struct CoroExecutor;

    struct TcpSocket {
        int fd;
        CoroExecutor* exec;

        TcpSocket() noexcept;
        TcpSocket(CoroExecutor* exec) noexcept;
        TcpSocket(int fd, CoroExecutor* exec) noexcept;

        void close();
        void shutdown(int how);

        int listen(int backlog);
        int bind(const sockaddr* addr, u32 addrLen);
        int socket(int domain, int type, int protocol);

        int setReuseAddr(bool on);
        int setNoDelay(bool on);

        int connectInf(const sockaddr* addr, u32 addrLen);
        int connect(const sockaddr* addr, u32 addrLen, u64 deadlineUs);
        int connectTout(const sockaddr* addr, u32 addrLen, u64 timeoutUs);

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

        static TcpSocket* create(ObjPool* pool, CoroExecutor* exec);
        static TcpSocket* create(ObjPool* pool, int fd, CoroExecutor* exec);
    };
}
