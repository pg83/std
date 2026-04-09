#pragma once

#include <std/sys/types.h>

struct iovec;
struct sockaddr;

namespace stl {
    struct PollFD;
    struct PollGroup;
    struct VisitorFace;

    struct IoReactor {
        virtual int recv(int fd, size_t* nRead, void* buf, size_t len, u64 deadlineUs) = 0;
        virtual int send(int fd, size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) = 0;
        virtual int writev(int fd, size_t* nWritten, iovec* iov, size_t iovcnt, u64 deadlineUs) = 0;
        virtual int accept(int fd, int* newFd, sockaddr* addr, u32* addrLen, u64 deadlineUs) = 0;
        virtual int connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) = 0;

        virtual int pread(int fd, size_t* nRead, void* buf, size_t len, off_t offset) = 0;
        virtual int pwrite(int fd, size_t* nWritten, const void* buf, size_t len, off_t offset) = 0;
        virtual int fsync(int fd) = 0;
        virtual int fdatasync(int fd) = 0;

        virtual u32 poll(PollFD pfd, u64 deadlineUs) = 0;
        virtual void poll(PollGroup* g, VisitorFace& visitor, u64 deadlineUs) = 0;

        void poll(PollGroup* g, VisitorFace&& visitor, u64 deadlineUs);
    };
}
