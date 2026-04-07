#include "out.h"
#include "out_fd.h"
#include "out_buf.h"
#include "out_mem.h"
#include "out_fd_coro.h"

#include <std/mem/obj_pool.h>

using namespace stl;

Output* stl::createFDOutput(ObjPool* pool, FD& fd) {
    return pool->make<FDOutput>(fd);
}

Output* stl::createFDRegular(ObjPool* pool, FD& fd) {
    return pool->make<FDRegular>(fd);
}

Output* stl::createFDCharacter(ObjPool* pool, FD& fd) {
    return pool->make<FDCharacter>(fd);
}

Output* stl::createFDPipe(ObjPool* pool, FD& fd) {
    return pool->make<FDPipe>(fd);
}

Output* stl::createCoroFDOutput(ObjPool* pool, FD& fd, CoroExecutor* exec) {
    return pool->make<CoroFDOutput>(fd, exec);
}

ZeroCopyOutput* stl::createMemoryOutput(ObjPool* pool, void* ptr) {
    return pool->make<MemoryOutput>(ptr);
}

ZeroCopyOutput* stl::createOutBuf(ObjPool* pool, Output& out) {
    return pool->make<OutBuf>(out);
}

ZeroCopyOutput* stl::createOutBuf(ObjPool* pool, Output& out, size_t chunkSize) {
    return pool->make<OutBuf>(out, chunkSize);
}
