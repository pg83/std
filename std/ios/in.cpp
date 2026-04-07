#include "in.h"
#include "in_fd.h"
#include "in_buf.h"
#include "in_mem.h"
#include "in_zero.h"
#include "in_fd_coro.h"

#include <std/mem/obj_pool.h>

using namespace stl;

Input* stl::createFDInput(ObjPool* pool, FD& fd) {
    return pool->make<FDInput>(fd);
}

Input* stl::createCoroFDInput(ObjPool* pool, FD& fd, CoroExecutor* exec) {
    return pool->make<CoroFDInput>(fd, exec);
}

ZeroCopyInput* stl::createMemoryInput(ObjPool* pool, const void* data, size_t len) {
    return pool->make<MemoryInput>(data, len);
}

ZeroCopyInput* stl::createInBuf(ObjPool* pool, Input& in) {
    return pool->make<InBuf>(in);
}

ZeroCopyInput* stl::createInBuf(ObjPool* pool, Input& in, size_t chunkSize) {
    return pool->make<InBuf>(in, chunkSize);
}

ZeroCopyInput* stl::createZeroInput(ObjPool* pool) {
    return pool->make<ZeroInput>();
}
