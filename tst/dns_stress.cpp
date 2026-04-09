#include <std/tst/args.h>
#include <std/thr/coro.h>
#include <std/mem/obj_pool.h>
#include <std/thr/coro_config.h>

#include <stdio.h>
#include <string.h>

using namespace stl;

int main(int argc, char** argv) {
    auto pool = ObjPool::fromMemory();
    TestArgs a(pool.mutPtr(), argc, argv);
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
