#include "output.h"

#include <std/str/view.h>
#include <std/alg/range.h>
#include <std/alg/exchange.h>

using namespace Std;

Output::~Output() noexcept {
}

void Output::flushImpl() {
}

void Output::finishImpl() {
}

void Output::writeVImpl(const StringView* parts, size_t count) {
    for (const auto& it : range(parts, parts + count)) {
        write(it.data(), it.length());
    }
}

void Output::write(const void* data, size_t len) {
    const u8* b = (u8*)data;
    const u8* e = b + len;

    const auto part = hint();

    while (true) {
        if (const auto left = e - b; left > part) {
            writeImpl(exchange(b, b + part), part);
        } else {
            return writeImpl(b, left);
        }
    }
}
