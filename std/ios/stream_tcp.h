#pragma once

#include "input.h"
#include "output.h"

namespace stl {
    struct TcpSocket;

    class TcpStream: public Input, public Output {
        size_t readImpl(void* data, size_t len) override;
        size_t writeImpl(const void* data, size_t len) override;
        size_t writeVImpl(iovec* parts, size_t count) override;

    public:
        TcpSocket* sock;

        TcpStream(TcpSocket& sock) noexcept;
        ~TcpStream() noexcept override;
    };
}
