#include "input.h"
#include "copy.h"

#include <std/lib/buffer.h>
#include <std/str/builder.h>

using namespace Std;

Input::~Input() noexcept {
}

void Input::read(void* data, size_t len) {
    u8* b = (u8*)data;
    u8* e = b + len;

    while (b < e) {
        b += readP(b, e - b);
    }
}

void Input::readAll(Buffer& res) {
    StringBuilder sb;
    sb.xchg(res);
    zeroCopy(*this, sb);
    sb.xchg(res);
}
