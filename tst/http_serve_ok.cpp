#include <std/tst/args.h>
#include <std/thr/coro.h>
#include <std/mem/obj_pool.h>
#include <std/net/http_srv.h>
#include <std/net/ssl_socket.h>
#include <std/thr/wait_group.h>
#include <std/thr/coro_config.h>

#include <arpa/inet.h>
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

    sockaddr_in makeAddr(u16 port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        return addr;
    }
}

int main(int argc, char** argv) {
    auto pool = ObjPool::fromMemory();
    TestArgs a(pool.mutPtr(), argc, argv);
    auto exec = CoroExecutor::create(pool.mutPtr(), 8);
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

    auto addr = makeAddr(port);

    WaitGroup wg(exec);

    serve(
        pool.mutPtr(),
        {
            .handler = &handler,
            .exec = exec,
            .wg = &wg,
            .addr = (const sockaddr*)&addr,
            .addrLen = sizeof(addr),
        }
    );

    exec->spawn([&] {
        wg.wait();
    });

    exec->join();
}
