#include "in_fd_coro.h"

#include <std/tst/ut.h>
#include <std/sys/fd.h>
#include <std/thr/coro.h>
#include <std/dbg/insist.h>
#include <std/sys/mem_fd.h>
#include <std/mem/obj_pool.h>
#include <std/thr/coro_config.h>

#include <string.h>

using namespace stl;

STD_TEST_SUITE(CoroFDInput) {
    STD_TEST(ReadData) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        ScopedFD sfd(memFD("test"));
        STD_INSIST(sfd.get() >= 0);

        const char* data = "hello coro";
        sfd.write(data, strlen(data));

        exec->spawn([&] {
            CoroFDInput in(sfd, exec);

            char buf[32] = {};
            auto n = in.read(buf, sizeof(buf));

            STD_INSIST(n == strlen(data));
            STD_INSIST(memcmp(buf, data, n) == 0);
            STD_INSIST(in.offset == (off_t)n);
        });

        exec->join();
    }

    STD_TEST(ReadEof) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        ScopedFD sfd(memFD("test"));
        STD_INSIST(sfd.get() >= 0);

        exec->spawn([&] {
            CoroFDInput in(sfd, exec);

            char buf[32];
            auto n = in.read(buf, sizeof(buf));

            STD_INSIST(n == 0);
        });

        exec->join();
    }

    STD_TEST(ReadAdvancesOffset) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        ScopedFD sfd(memFD("test"));
        STD_INSIST(sfd.get() >= 0);

        sfd.write("abcdef", 6);

        exec->spawn([&] {
            CoroFDInput in(sfd, exec);

            char buf[3];
            STD_INSIST(in.read(buf, 3) == 3);
            STD_INSIST(in.offset == 3);
            STD_INSIST(in.read(buf, 3) == 3);
            STD_INSIST(in.offset == 6);
            STD_INSIST(memcmp(buf, "def", 3) == 0);
        });

        exec->join();
    }
}
