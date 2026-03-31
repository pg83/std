#include "dns.h"

#include <std/tst/ut.h>
#include <std/thr/coro.h>
#include <std/thr/async.h>
#include <std/dbg/insist.h>
#include <std/mem/obj_pool.h>

#include <arpa/inet.h>

using namespace stl;

STD_TEST_SUITE(DnsResolver) {
    STD_TEST(ResolveLocalhost) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto resolver = DnsResolver::create(pool.mutPtr(), exec);

        auto f = async(exec, [&, rpool = pool->create(pool.mutPtr())] {
            return resolver->resolve(rpool, u8"localhost");
        });

        auto result = f.wait();

        STD_INSIST(result->ok());
        STD_INSIST(result->record);
    }

    STD_TEST(ResolveParallel) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto resolver = DnsResolver::create(pool.mutPtr(), exec);

        auto f1 = async(exec, [&, rpool = pool->create(pool.mutPtr())] {
            return resolver->resolve(rpool, u8"localhost");
        });

        auto f2 = async(exec, [&, rpool = pool->create(pool.mutPtr())] {
            return resolver->resolve(rpool, u8"localhost");
        });

        auto r1 = f1.wait();
        auto r2 = f2.wait();

        STD_INSIST(r1->ok());
        STD_INSIST(r1->record);
        STD_INSIST(r2->ok());
        STD_INSIST(r2->record);
    }

    STD_TEST(ResolveBadName) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto resolver = DnsResolver::create(pool.mutPtr(), exec);

        auto f = async(exec, [&, rpool = pool->create(pool.mutPtr())] {
            return resolver->resolve(rpool, u8"this.name.does.not.exist.invalid");
        });

        auto result = f.wait();

        STD_INSIST(result->ok());
        STD_INSIST(!result->record);
    }
}
