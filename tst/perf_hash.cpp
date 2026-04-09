#include <std/sym/i_map.h>
#include <std/mem/obj_pool.h>

using namespace stl;

int main() {
    auto pool = ObjPool::fromMemory();
    IntMap<int> m(pool.mutPtr());

    for (size_t i = 0; i < 200000000; ++i) {
        m[i] = i;
    }
}
