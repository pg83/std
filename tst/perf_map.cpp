#include <std/map/map.h>
#include <std/mem/obj_pool.h>

using namespace stl;

int main() {
    auto pool = ObjPool::fromMemory();
    Map<int, int> m(pool.mutPtr());

    for (size_t i = 0; i < 20000000; ++i) {
        m[i] = i;
    }
}
