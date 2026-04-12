#pragma once

#include "pollable.h"

struct iovec;
struct sockaddr;

namespace stl {
    class ObjPool;

    struct PollerIface;
    struct CondVarIface;
    struct CoroExecutor;
    struct ThreadPoolHooks;

    struct IoReactor: public Pollable {
        virtual int connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) = 0;
        virtual int recv(int fd, size_t* nRead, void* buf, size_t len, u64 deadlineUs) = 0;
        virtual int recvfrom(int fd, size_t* nRead, void* buf, size_t len, sockaddr* addr, u32* addrLen, u64 deadlineUs) = 0;
        virtual int accept(int fd, int* newFd, sockaddr* addr, u32* addrLen, u64 deadlineUs) = 0;
        virtual int send(int fd, size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) = 0;
        virtual int sendto(int fd, size_t* nWritten, const void* buf, size_t len, const sockaddr* addr, u32 addrLen, u64 deadlineUs) = 0;
        virtual int writev(int fd, size_t* nWritten, iovec* iov, size_t iovcnt, u64 deadlineUs) = 0;

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
