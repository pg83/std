#include <std/sys/fd.h>
#include <std/tst/args.h>
#include <std/thr/coro.h>
#include <std/lib/buffer.h>
#include <std/net/http_srv.h>
#include <std/mem/obj_pool.h>
#include <std/thr/wait_group.h>
#include <std/ios/in_fd_coro.h>
#include <std/net/ssl_socket.h>

#include <fcntl.h>
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
        CoroExecutor* exec;
        SslCtx* sslCtx;

        SslCtx* ssl() override {
            return sslCtx;
        }

        void serve(HttpServerResponse& resp) override {
            auto req = resp.request();

            ScopedFD fd(::open(Buffer(req->path()).cStr(), O_RDONLY));

            if (fd.get() < 0) {
                resp.setStatus(404);
                resp.addHeader(StringView("Content-Length"), StringView("0"));
                resp.endHeaders();

                return;
            }

            resp.endHeaders();

            CoroFDInput(fd, exec).sendTo(*resp.out());
            resp.out()->finish();
        }
    } handler;

    handler.exec = exec;
    handler.sslCtx = sslCtx;

    u16 port = 18080;

    if (auto v = a.find(StringView("port"))) {
        port = (u16)v->stou();
    }

    auto addr = makeAddr(port);

    WaitGroup wg(0, exec);

    serve(
        pool.mutPtr(),
        {
            .handler = &handler,
            .exec = exec,
            .wg = &wg,
            .addr = (const sockaddr*)&addr,
            .addrLen = sizeof(addr),
        });

    exec->spawn([&] {
        wg.wait();
    });

    exec->join();
}
