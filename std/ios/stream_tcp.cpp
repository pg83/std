#include "stream_tcp.h"

#include <std/thr/tcp.h>
#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>

using namespace stl;

TcpStream::TcpStream(TcpSocket& sock) noexcept
    : sock(&sock)
{
}

TcpStream::~TcpStream() noexcept {
}

size_t TcpStream::readImpl(void* data, size_t len) {
    size_t n = 0;

    if (int r = sock->readInf(&n, data, len); r < 0) {
        Errno(-r).raise(StringBuilder() << StringView(u8"tcp read() failed"));
    }

    return n;
}

size_t TcpStream::writeImpl(const void* data, size_t len) {
    size_t n = 0;

    if (int r = sock->writeInf(&n, data, len); r < 0) {
        Errno(-r).raise(StringBuilder() << StringView(u8"tcp write() failed"));
    }

    return n;
}
