#include <std/ios/sys.h>
#include <std/tst/args.h>
#include <std/thr/coro.h>
#include <std/thr/async.h>
#include <std/dns/result.h>
#include <std/dns/record.h>
#include <std/mem/obj_pool.h>
#include <std/net/http_srv.h>
#include <std/net/ssl_socket.h>
#include <std/thr/wait_group.h>
#include <std/thr/coro_config.h>

#include <netinet/in.h>

using namespace stl;

namespace {
    const char testCert[] =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBfzCCASWgAwIBAgIUUQjugcnFUjBED4h+YaPFRr7pMlowCgYIKoZIzj0EAwIw\n"
        "FDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MDMyMzE0MjI1MFoYDzIxMjYwMjI3\n"
        "MTQyMjUwWjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwWTATBgcqhkjOPQIBBggqhkjO\n"
        "PQMBBwNCAARfsZNKls055081/xImV4diaUrCimYW2k0m7Rhq/B5xBjWP5ETfBy3X\n"
        "aoUXGA02bImZaTSTLd43TWTCB5IbRkngo1MwUTAdBgNVHQ4EFgQURCfdIwDIDcdE\n"
        "hrObYyq2RJsofgswHwYDVR0jBBgwFoAURCfdIwDIDcdEhrObYyq2RJsofgswDwYD\n"
        "VR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNIADBFAiEAiISrnWrLf3ZHEnIfmPcM\n"
        "XMJDSo3hPE486n7a40YQQw8CIEICeaFmiGyD2MvrpAe+S6k80bGFOJeWDOzwJmP7\n"
        "nZq5\n"
        "-----END CERTIFICATE-----";

    const char testKey[] =
        "-----BEGIN PRIVATE KEY-----\n"
        "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgw0VlkoyRhsiGGmYa\n"
        "R4bbSCCAbt0MrRex/LkuoYW1/myhRANCAARfsZNKls055081/xImV4diaUrCimYW\n"
        "2k0m7Rhq/B5xBjWP5ETfBy3XaoUXGA02bImZaTSTLd43TWTCB5IbRkng\n"
        "-----END PRIVATE KEY-----";
}

int main(int argc, char** argv) {
    auto pool = ObjPool::fromMemory();
    TestArgs a(pool.mutPtr(), argc, argv);
    auto exec = CoroExecutor::create(pool.mutPtr(), 32);
    auto sslCtx = SslCtx::create(pool.mutPtr(), StringView(testCert), StringView(testKey));

    struct Handler: HttpServe {
        SslCtx* sslCtx;

        SslCtx* ssl() override {
            return sslCtx;
        }

        void serve(HttpServerResponse& resp) override {
            resp.setStatus(200);
            resp.addHeader(StringView("Content-Length"), StringView("0"));
            resp.addHeader(StringView("Payload"), StringView("12345678901234567890123456789012345"));
            resp.endHeaders();
            resp.out()->finish();
        }
    } handler;

    handler.sslCtx = sslCtx;

    u16 port = 18080;

    if (auto v = a.find(StringView("port"))) {
        port = (u16)v->stou();
    }

    auto dns = async(exec, [&] {
                   return exec->resolve(pool.mutPtr(), StringView("localhost"));
               }).wait();

    if (!dns->ok() || !dns->record) {
        sysE << StringView(u8"dns resolve failed: ") << dns << endL;
        return 1;
    }

    sysE << StringView(u8"resolved: ") << dns->record << endL;

    WaitGroup wg(exec);

    for (auto rec = dns->record; rec; rec = rec->next) {
        if (rec->family == AF_INET) {
            ((sockaddr_in*)rec->addr)->sin_port = htons(port);
        } else {
            ((sockaddr_in6*)rec->addr)->sin6_port = htons(port);
        }

        u32 addrLen = (rec->family == AF_INET) ? (u32)sizeof(sockaddr_in) : (u32)sizeof(sockaddr_in6);

        sysE << StringView(u8"serving on ") << rec << StringView(u8":") << (u64)port << endL;

        serve(
            pool.mutPtr(),
            {
                .handler = &handler,
                .exec = exec,
                .wg = &wg,
                .addr = rec->addr,
                .addrLen = addrLen,
            });
    }

    exec->spawn([&] {
        wg.wait();
    });

    exec->join();
}
