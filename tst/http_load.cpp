#include <std/map/map.h>
#include <std/ios/out.h>
#include <std/ios/sys.h>
#include <std/sys/crt.h>
#include <std/tst/args.h>
#include <std/thr/coro.h>
#include <std/thr/async.h>
#include <std/dbg/color.h>
#include <std/alg/defer.h>
#include <std/dbg/insist.h>
#include <std/lib/buffer.h>
#include <std/ios/in_buf.h>
#include <std/dns/iface.h>
#include <std/dns/config.h>
#include <std/dns/result.h>
#include <std/dns/record.h>
#include <std/str/builder.h>
#include <std/thr/channel.h>
#include <std/mem/obj_pool.h>
#include <std/thr/wait_group.h>
#include <std/ios/stream_tcp.h>
#include <std/net/tcp_socket.h>
#include <std/net/http_client.h>
#include <std/net/http_reason.h>

#include <sys/socket.h>
#include <netinet/in.h>

using namespace stl;

namespace {
    struct ReqRecord {
        u32 status;
        u32 elapsedUs;
    };

    struct ReqBatch {
        static constexpr u32 CAP = 255;

        ReqRecord records[CAP];
        u64 len;

        ReqBatch()
            : len(0)
        {
        }
    };
}

int main(int argc, char** argv) {
    auto pool = ObjPool::fromMemory();
    TestArgs a(pool.mutPtr(), argc, argv);

    auto urlArg = a.find(StringView("url"));

    if (!urlArg) {
        sysE << StringView(u8"usage: http_load --url=http://host:port/path [--coros=100] [--threads=4] [--duration=10] [--payload=0]") << endL;
        return 1;
    }

    StringView url = *urlArg;

    if (url.startsWith(StringView("http://"))) {
        url = StringView(url.data() + 7, url.length() - 7);
    }

    StringView hostPort, path;

    if (!url.split('/', hostPort, path)) {
        hostPort = url;
    }

    StringView host, portStr;
    u16 port = 80;

    if (hostPort.split(':', host, portStr)) {
        port = (u16)portStr.stou();
    } else {
        host = hostPort;
    }

    u32 numCoros = 100;
    u32 numThreads = 4;
    u32 durationSec = 10;
    u32 payloadLen = 0;

    if (auto v = a.find(StringView("coros"))) {
        numCoros = (u32)v->stou();
    }

    if (auto v = a.find(StringView("threads"))) {
        numThreads = (u32)v->stou();
    }

    if (auto v = a.find(StringView("duration"))) {
        durationSec = (u32)v->stou();
    }

    if (auto v = a.find(StringView("payload"))) {
        payloadLen = (u32)v->stou();
    }

    auto exec = CoroExecutor::create(pool.mutPtr(), numThreads);

    auto resolver = DnsResolver::create(pool.mutPtr(), exec, nullptr, DnsConfig());

    auto dns = async(exec, [&] {
                   return resolver->resolve(pool.mutPtr(), host);
               }).wait();

    if (!dns->ok() || !dns->record) {
        sysE << StringView(u8"dns resolve failed: ") << dns << endL;
        return 1;
    }

    auto rec = dns->record;

    if (rec->family == AF_INET) {
        ((sockaddr_in*)rec->addr)->sin_port = htons(port);
    } else {
        ((sockaddr_in6*)rec->addr)->sin6_port = htons(port);
    }

    Buffer payloadBuf;

    for (u32 i = 0; i < payloadLen; ++i) {
        u8 x = 'X';
        payloadBuf.append(&x, 1);
    }

    StringView payloadVal(payloadBuf);

    StringView fullPath;

    if (path.empty()) {
        fullPath = StringView("/");
    } else {
        StringBuilder pathBuf;

        pathBuf << StringView(u8"/") << path;

        fullPath = pool.mutPtr()->intern(StringView(pathBuf));
    }

    auto G = Color::bright(AnsiColor::Green);
    auto C = Color::bright(AnsiColor::Cyan);
    auto W = Color::bright(AnsiColor::White);
    auto R = Color::reset();

    sysE << endL;
    sysE << W << StringView(u8"host:     ") << G << host << R << endL;
    sysE << W << StringView(u8"resolved: ") << G << rec << R << endL;
    sysE << W << StringView(u8"port:     ") << G << (u64)port << R << endL;
    sysE << W << StringView(u8"path:     ") << G << fullPath << R << endL;
    sysE << W << StringView(u8"threads:  ") << G << (u64)numThreads << R << endL;
    sysE << W << StringView(u8"coros:    ") << G << (u64)numCoros << R << endL;
    sysE << W << StringView(u8"duration: ") << G << (u64)durationSec << StringView(u8"s") << R << endL;
    sysE << W << StringView(u8"payload:  ") << G << (u64)payloadLen << R << endL;

    Channel ch(exec, numCoros * 4);
    auto& wg = *WaitGroup::create(pool.mutPtr(), 0, exec);

    u64 startUs = monotonicNowUs();
    u64 deadlineUs = startUs + (u64)durationSec * 1000000;

    u64 totalReqs = 0;
    Map<u64, u64> statuses(pool.mutPtr());
    u64 hist[64] = {};

    exec->spawn([&] {
        void* v = nullptr;

        while (ch.dequeue(&v)) {
            auto batch = (ReqBatch*)v;

            for (u32 j = 0; j < batch->len; ++j) {
                ++statuses[(u64)batch->records[j].status];
                ++totalReqs;

                u32 dt = batch->records[j].elapsedUs;
                u32 bucket = 0;

                if (dt > 0) {
                    bucket = 31 - __builtin_clz(dt);
                }

                ++hist[bucket];
            }

            delete batch;
        }
    });

    for (u32 i = 0; i < numCoros; ++i) {
        wg.inc();

        exec->spawn([&] {
            STD_DEFER {
                wg.done();
            };

            u32 addrLen = (rec->family == AF_INET) ? (u32)sizeof(sockaddr_in) : (u32)sizeof(sockaddr_in6);

            int cfd;

            STD_INSIST(TcpSocket::connectInf(&cfd, exec, rec->addr, addrLen) == 0);

            TcpSocket sock(cfd, exec);

            STD_DEFER {
                sock.close();
            };

            TcpStream stream(sock);
            InBuf in(stream);

            Buffer body;
            auto batch = new ReqBatch();

            while (monotonicNowUs() < deadlineUs) {
                auto rpool = ObjPool::fromMemory();
                auto req = HttpClientRequest::create(rpool.mutPtr(), &in, &stream);

                req->setPath(fullPath);
                req->addHeader(StringView("Host"), host);
                req->addHeader(StringView("Connection"), StringView("keep-alive"));

                if (payloadLen > 0) {
                    req->addHeader(StringView("X-Payload"), payloadVal);
                }

                u64 t0 = monotonicNowUs();

                req->endHeaders();

                auto resp = req->response();
                u32 st = resp->status();

                if (st == 0) {
                    break;
                }

                resp->body()->readAll(body);

                u64 t1 = monotonicNowUs();

                batch->records[batch->len].status = st;
                batch->records[batch->len].elapsedUs = t1 - t0;
                ++batch->len;

                if (batch->len == ReqBatch::CAP) {
                    ch.enqueue(batch);
                    batch = new ReqBatch();
                }
            }

            if (batch->len > 0) {
                ch.enqueue(batch);
            } else {
                delete batch;
            }
        });
    }

    exec->spawn([&] {
        wg.wait();
        ch.close();
    });

    exec->join();

    u64 elapsedUs = monotonicNowUs() - startUs;
    double elapsedSec = (double)elapsedUs / 1000000.0;
    double rps = (double)totalReqs / elapsedSec;

    sysE << endL;
    sysE << W << StringView(u8"requests: ") << G << totalReqs << endL;
    sysE << W << StringView(u8"elapsed:  ") << G << elapsedSec << StringView(u8"s") << endL;
    sysE << W << StringView(u8"rps:      ") << G << rps << endL;
    sysE << R << endL;

    sysE << W << StringView(u8"status codes:") << R << endL;

    statuses.visit([&](u64 code, u64& count) {
        auto sc = (code >= 200 && code < 300)
                      ? Color::bright(AnsiColor::Green)
                  : (code >= 400)
                      ? Color::bright(AnsiColor::Red)
                      : Color::bright(AnsiColor::Yellow);

        sysE << StringView(u8"  ") << sc << code
             << StringView(u8" ") << reasonPhrase((u32)code)
             << R << StringView(u8": ") << W << count
             << R << endL;
    });

    sysE << endL;
    sysE << W << StringView(u8"latency histogram (us):") << R << endL;

    u32 lastNonZero = 0;

    for (u32 i = 0; i < 64; ++i) {
        if (hist[i] > 0) {
            lastNonZero = i;
        }
    }

    for (u32 i = 0; i <= lastNonZero; ++i) {
        u64 lo = (i == 0) ? 0 : ((u64)1 << i);
        u64 hi = ((u64)1 << (i + 1)) - 1;

        auto bc = (hist[i] > 0)
                      ? Color::bright(AnsiColor::Cyan)
                      : Color::dark(AnsiColor::White);

        sysE << C << StringView(u8"  [")
             << lo << StringView(u8" .. ") << hi
             << StringView(u8"] ") << bc << hist[i]
             << R << endL;
    }
}
