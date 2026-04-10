#include <std/tst/args.h>
#include <std/thr/coro.h>
#include <std/dns/iface.h>
#include <std/dns/config.h>
#include <std/mem/obj_pool.h>

#include <stdio.h>
#include <string.h>

using namespace stl;

int main(int argc, char** argv) {
    auto pool = ObjPool::fromMemory();
    TestArgs a(pool.mutPtr(), argc, argv);
    size_t threads = 8;

    if (auto sv = a.find(u8"coro-threads"); sv) {
        threads = sv->stou();
    }

    DnsConfig dnsCfg;

    if (auto sv = a.find(u8"dns-family"); sv) {
        dnsCfg.family = sv->stou();
    }

    if (auto sv = a.find(u8"dns-timeout"); sv) {
        dnsCfg.timeout = sv->stou();
    }

    if (auto sv = a.find(u8"dns-tries"); sv) {
        dnsCfg.tries = sv->stou();
    }

    if (auto sv = a.find(u8"dns-udp-max-queries"); sv) {
        dnsCfg.udpMaxQueries = sv->stou();
    }

    if (a.find(u8"dns-tcp")) {
        dnsCfg.tcp = true;
    }

    if (auto sv = a.find(u8"dns-server"); sv) {
        dnsCfg.server = *sv;
    }

    auto exec = CoroExecutor::create(pool.mutPtr(), threads);
    auto resolver = DnsResolver::create(pool.mutPtr(), exec, nullptr, dnsCfg);

    for (int i = 0; i < 100000; ++i) {
        exec->spawn([&, i] {
            auto rpool = ObjPool::fromMemory();
            char buf[64];

            snprintf(buf, sizeof(buf), "host%d.test.invalid", i);
            resolver->resolve(rpool.mutPtr(), StringView((const u8*)buf, strlen(buf)));
        });
    }

    exec->join();
}
