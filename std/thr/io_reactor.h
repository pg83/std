#pragma once

#include <std/sys/types.h>

struct iovec;
struct sockaddr;

namespace stl {
    class ObjPool;

    struct PollFD;
    struct PollGroup;
    struct ThreadPool;
    struct VisitorFace;
    struct CoroExecutor;

    struct IoReactor {
        virtual int recv(int fd, void* buf, size_t len, size_t* nRead, u64 deadlineUs) = 0;
        virtual int send(int fd, const void* buf, size_t len, size_t* nWritten, u64 deadlineUs) = 0;
        virtual int writev(int fd, iovec* iov, size_t iovcnt, size_t* nWritten, u64 deadlineUs) = 0;
        virtual int accept(int fd, sockaddr* addr, u32* addrLen, int* newFd, u64 deadlineUs) = 0;
        virtual int connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) = 0;

        virtual int pread(int fd, void* buf, size_t len, off_t offset, size_t* nRead) = 0;
        virtual int pwrite(int fd, const void* buf, size_t len, off_t offset, size_t* nWritten) = 0;
        virtual int fsync(int fd) = 0;
        virtual int fdatasync(int fd) = 0;

        virtual u32 poll(PollFD pfd, u64 deadlineUs) = 0;
        virtual void poll(PollGroup* g, VisitorFace& visitor, u64 deadlineUs) = 0;

        static IoReactor* createPoll(ObjPool* pool, CoroExecutor* exec, ThreadPool* mainPool, ThreadPool* offload);
    };
}
