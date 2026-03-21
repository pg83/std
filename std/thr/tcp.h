#pragma once

#include "coro.h"

#include <std/sys/types.h>

#include <sys/socket.h>

namespace stl {
    struct TcpSocket {
        int fd;
        CoroExecutor* exec;

        TcpSocket() noexcept;
        TcpSocket(CoroExecutor* exec) noexcept;
        TcpSocket(int fd, CoroExecutor* exec) noexcept;

        int socket(int domain, int type, int protocol);
        void close();
        void shutdown(int how);
        int bind(const sockaddr* addr, u32 addrLen);
        int listen(int backlog);

        int connect(const sockaddr* addr, u32 addrLen, u64 deadlineUs);
        int connectTout(const sockaddr* addr, u32 addrLen, u64 timeoutUs);
        int connectInf(const sockaddr* addr, u32 addrLen);

        int accept(TcpSocket& out, sockaddr* addr, u32* addrLen, u64 deadlineUs);
        int acceptTout(TcpSocket& out, sockaddr* addr, u32* addrLen, u64 timeoutUs);
        int acceptInf(TcpSocket& out, sockaddr* addr, u32* addrLen);

        size_t read(void* buf, size_t len, u64 deadlineUs);
        size_t readTout(void* buf, size_t len, u64 timeoutUs);
        size_t readInf(void* buf, size_t len);

        size_t write(const void* buf, size_t len, u64 deadlineUs);
        size_t writeTout(const void* buf, size_t len, u64 timeoutUs);
        size_t writeInf(const void* buf, size_t len);
    };
}
