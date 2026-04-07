#include "in.h"

#include <std/tst/ut.h>
#include <std/sys/fd.h>
#include <std/dbg/insist.h>
#include <std/sys/mem_fd.h>
#include <std/mem/obj_pool.h>

using namespace stl;

STD_TEST_SUITE(InFactory) {
    STD_TEST(createFDInput) {
        auto pool = ObjPool::fromMemory();
        ScopedFD sfd(memFD("test"));

        auto* in = createFDInput(pool.mutPtr(), sfd);

        STD_INSIST(in != nullptr);
    }

    STD_TEST(createMemoryInput) {
        auto pool = ObjPool::fromMemory();
        const char data[] = "hello";

        auto* in = createMemoryInput(pool.mutPtr(), data, sizeof(data) - 1);

        STD_INSIST(in != nullptr);
    }

    STD_TEST(createInBuf) {
        auto pool = ObjPool::fromMemory();
        ScopedFD sfd(memFD("test"));
        auto* fd_in = createFDInput(pool.mutPtr(), sfd);

        auto* buf = createInBuf(pool.mutPtr(), *fd_in);

        STD_INSIST(buf != nullptr);
    }

    STD_TEST(createInBufWithChunkSize) {
        auto pool = ObjPool::fromMemory();
        ScopedFD sfd(memFD("test"));
        auto* fd_in = createFDInput(pool.mutPtr(), sfd);

        auto* buf = createInBuf(pool.mutPtr(), *fd_in, 4096);

        STD_INSIST(buf != nullptr);
    }
}
