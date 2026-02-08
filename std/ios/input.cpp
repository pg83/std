#include "input.h"

#include <std/alg/advance.h>

using namespace Std;

void Input::read(void* data, size_t len) {
    while (len) {
        auto p = readP(data, len);

        len -= p;
        data = advancePtr(data, p);
    }
}
