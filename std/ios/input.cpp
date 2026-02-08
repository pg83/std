#include "input.h"
#include "copy.h"

#include <std/lib/buffer.h>
#include <std/alg/advance.h>
#include <std/str/builder.h>

using namespace Std;

Input::~Input() noexcept {
}

void Input::read(void* data, size_t len) {
    while (len) {
        auto p = readP(data, len);

        len -= p;
        data = advancePtr(data, p);
    }
}

void Input::readAll(Buffer& res) {
    StringBuilder sb;
    sb.xchg(res);
    zeroCopy(*this, sb);
    sb.xchg(res);
}
