#include "dns_iface.h"

#include <std/tst/ut.h>
#include <std/tst/args.h>
#include <std/thr/coro.h>
#include <std/thr/async.h>
#include <std/dbg/insist.h>
#include <std/lib/vector.h>
#include <std/mem/obj_pool.h>
#include <std/thr/coro_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    STD_TEST(_ResolveStress) {
        auto pool = ObjPool::fromMemory();
        auto& a = _ctx.args();
        auto cfg = CoroConfig(8);

        if (auto sv = a.find(u8"coro-threads"); sv) {
            cfg.setThreads(sv->stou());
        }

        if (auto sv = a.find(u8"coro-reactors"); sv) {
            cfg.setReactors(sv->stou());
        }

        if (auto sv = a.find(u8"coro-offload-threads"); sv) {
            cfg.setOffloadThreads(sv->stou());
        }

        if (auto sv = a.find(u8"dns-resolvers"); sv) {
            cfg.setDnsResolvers(sv->stou());
        }

        if (auto sv = a.find(u8"dns-max-queries"); sv) {
            cfg.setMaxDnsQueries(sv->stou());
        }

        if (auto sv = a.find(u8"dns-family"); sv) {
            cfg.setDnsFamily(sv->stou());
        }

        if (auto sv = a.find(u8"dns-timeout"); sv) {
            cfg.setDnsTimeout(sv->stou());
        }

        if (auto sv = a.find(u8"dns-tries"); sv) {
            cfg.setDnsTries(sv->stou());
        }

        if (auto sv = a.find(u8"dns-udp-max-queries"); sv) {
            cfg.setDnsUdpMaxQueries(sv->stou());
        }

        if (a.find(u8"dns-tcp")) {
            cfg.setDnsTcp(true);
        }

        if (auto sv = a.find(u8"dns-server"); sv) {
            cfg.setDnsServer(*sv);
        }

        auto exec = CoroExecutor::create(pool.mutPtr(), cfg);

        for (int i = 0; i < 100000; ++i) {
            exec->spawn([&, i] {
                auto rpool = ObjPool::fromMemory();
                char buf[64];

                snprintf(buf, sizeof(buf), "host%d.test.invalid", i);
                exec->resolve(rpool.mutPtr(), StringView((const u8*)buf, strlen(buf)));
            });
        }

        exec->join();
    }

    STD_TEST(ResolveInvalidName) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto resolver = DnsResolver::create(pool.mutPtr(), exec);

        auto f = async(exec, [&, rpool = pool->create(pool.mutPtr())] {
            return resolver->resolve(rpool, u8"bad name");
        });

        auto result = f.wait();

        STD_INSIST(result->ok());
        STD_INSIST(!result->record);
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
