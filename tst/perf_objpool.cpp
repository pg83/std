#include <std/mem/obj_pool.h>

using namespace stl;

int main() {
    volatile size_t res = 0;

    for (size_t i = 0; i < 100000000; ++i) {
        auto p = ObjPool::fromMemoryRaw();
        res += (size_t)p;
        delete p;
    }
}
