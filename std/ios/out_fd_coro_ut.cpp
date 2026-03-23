#include "out_fd_coro.h"

#include <std/tst/ut.h>
#include <std/sys/fd.h>
#include <std/thr/coro.h>
#include <std/dbg/insist.h>

#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

using namespace stl;

STD_TEST_SUITE(CoroFDOutput) {
    STD_TEST(WriteData) {
        auto exec = CoroExecutor::create(4);
        ScopedFD sfd(memfd_create("test", 0));
        STD_INSIST(sfd.get() >= 0);

        const char* data = "hello coro";

        exec->spawn([&] {
            CoroFDOutput out(sfd, exec.mutPtr());
            out.write(data, strlen(data));
            STD_INSIST(out.offset == (off_t)strlen(data));
        });

        exec->join();

        lseek(sfd.get(), 0, SEEK_SET);
        char buf[32] = {};
        sfd.read(buf, sizeof(buf));
        STD_INSIST(memcmp(buf, data, strlen(data)) == 0);
    }

    STD_TEST(WriteAdvancesOffset) {
        auto exec = CoroExecutor::create(4);
        ScopedFD sfd(memfd_create("test", 0));
        STD_INSIST(sfd.get() >= 0);

        exec->spawn([&] {
            CoroFDOutput out(sfd, exec.mutPtr());
            out.write("abc", 3);
            STD_INSIST(out.offset == 3);
            out.write("def", 3);
            STD_INSIST(out.offset == 6);
        });

        exec->join();

        lseek(sfd.get(), 0, SEEK_SET);
        char buf[8] = {};
        sfd.read(buf, 6);
        STD_INSIST(memcmp(buf, "abcdef", 6) == 0);
    }
}
