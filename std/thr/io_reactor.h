#pragma once

#include <std/sys/types.h>

struct sockaddr;

namespace stl {
    class ObjPool;

    struct PollFD;
    struct PollGroup;
    struct ThreadPool;
    struct VisitorFace;
    struct CoroExecutor;

    struct IoReactor {
        virtual ssize_t recv(int fd, void* buf, size_t len, u64 deadlineUs) = 0;
        virtual ssize_t send(int fd, const void* buf, size_t len, u64 deadlineUs) = 0;
        virtual int accept(int fd, sockaddr* addr, u32* addrLen, u64 deadlineUs) = 0;
        virtual int connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) = 0;

        virtual ssize_t pread(int fd, void* buf, size_t len, off_t offset) = 0;
        virtual ssize_t pwrite(int fd, const void* buf, size_t len, off_t offset) = 0;
        virtual int fsync(int fd) = 0;
        virtual int fdatasync(int fd) = 0;

        virtual u32 poll(PollFD pfd, u64 deadlineUs) = 0;
        virtual void poll(PollGroup* g, VisitorFace& visitor, u64 deadlineUs) = 0;

        static IoReactor* createPoll(ObjPool* pool, CoroExecutor* exec, ThreadPool* mainPool, ThreadPool* offload);
    };
}
