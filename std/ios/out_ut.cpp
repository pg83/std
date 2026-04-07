#include "out.h"

#include <std/tst/ut.h>
#include <std/sys/fd.h>
#include <std/dbg/insist.h>
#include <std/sys/mem_fd.h>
#include <std/mem/obj_pool.h>

using namespace stl;

STD_TEST_SUITE(OutFactory) {
    STD_TEST(createFDOutput) {
        auto pool = ObjPool::fromMemory();
        ScopedFD sfd(memFD("test"));

        auto* out = createFDOutput(pool.mutPtr(), sfd);

        STD_INSIST(out != nullptr);
    }

    STD_TEST(createFDRegular) {
        auto pool = ObjPool::fromMemory();
        ScopedFD sfd(memFD("test"));

        auto* out = createFDRegular(pool.mutPtr(), sfd);

        STD_INSIST(out != nullptr);
    }

    STD_TEST(createFDPipe) {
        auto pool = ObjPool::fromMemory();
        ScopedFD in_fd, out_fd;
        createPipeFD(in_fd, out_fd);

        auto* out = createFDPipe(pool.mutPtr(), out_fd);

        STD_INSIST(out != nullptr);
    }

    STD_TEST(createMemoryOutput) {
        auto pool = ObjPool::fromMemory();
        char buf[64];

        auto* out = createMemoryOutput(pool.mutPtr(), buf);

        STD_INSIST(out != nullptr);
    }

    STD_TEST(createOutBuf) {
        auto pool = ObjPool::fromMemory();
        ScopedFD sfd(memFD("test"));
        auto* fd_out = createFDOutput(pool.mutPtr(), sfd);

        auto* buf = createOutBuf(pool.mutPtr(), *fd_out);

        STD_INSIST(buf != nullptr);
    }

    STD_TEST(createOutBufWithChunkSize) {
        auto pool = ObjPool::fromMemory();
        ScopedFD sfd(memFD("test"));
        auto* fd_out = createFDOutput(pool.mutPtr(), sfd);

        auto* buf = createOutBuf(pool.mutPtr(), *fd_out, 4096);

        STD_INSIST(buf != nullptr);
    }
}
