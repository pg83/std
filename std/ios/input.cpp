#include "input.h"
#include "copy.h"

#include <std/alg/defer.h>
#include <std/lib/buffer.h>
#include <std/str/builder.h>

using namespace Std;

Input::~Input() noexcept {
}

void Input::readAll(Buffer& res) {
    StringBuilder sb;

    sb.xchg(res);

    STD_DEFER {
        sb.xchg(res);
    };

    copy(*this, sb);
}

ZeroCopyInput* Input::zeroCopy() noexcept {
    return nullptr;
}
