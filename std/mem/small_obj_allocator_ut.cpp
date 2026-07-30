#include "obj_pool.h"
#include "small_obj_allocator.h"

#include <std/tst/ut.h>

using namespace stl;

namespace {
    struct Tracked {
        Tracked(i32 value, i32* destroyed);
        ~Tracked() noexcept;

        i32 value;
        i32* destroyed;
    };

    struct MaxSizeObject {
        u8 data[smallObjMaxSize];
    };

    Tracked::Tracked(i32 value_, i32* destroyed_)
        : value(value_)
        , destroyed(destroyed_)
    {
    }

    Tracked::~Tracked() noexcept {
        ++*destroyed;
    }
}

STD_TEST_SUITE(SmallObjAllocator) {
    STD_TEST(ReusesReleasedSlot) {
        auto pool = ObjPool::fromMemory();
        SmallObjAllocator* allocator = SmallObjAllocator::create(pool.mutPtr());

        void* first = allocator->allocate(37);
        allocator->deallocate(first, 37);
        void* second = allocator->allocate(37);

        STD_INSIST(second == first);
        allocator->deallocate(second, 37);
    }

    STD_TEST(SharesSlotsWithinSizeClass) {
        auto pool = ObjPool::fromMemory();
        SmallObjAllocator* allocator = SmallObjAllocator::create(pool.mutPtr());

        void* first = allocator->allocate(17);
        allocator->deallocate(first, 17);
        void* second = allocator->allocate(32);

        STD_INSIST(second == first);
        allocator->deallocate(second, 32);
    }

    STD_TEST(KeepsSizeClassesSeparate) {
        auto pool = ObjPool::fromMemory();
        SmallObjAllocator* allocator = SmallObjAllocator::create(pool.mutPtr());

        void* small = allocator->allocate(16);
        void* large = allocator->allocate(17);
        allocator->deallocate(small, 16);
        allocator->deallocate(large, 17);

        void* reusedSmall = allocator->allocate(16);
        void* reusedLarge = allocator->allocate(17);

        STD_INSIST(reusedSmall == small);
        STD_INSIST(reusedLarge == large);
        allocator->deallocate(reusedSmall, 16);
        allocator->deallocate(reusedLarge, 17);
    }

    STD_TEST(SupportsEverySizeClassBoundary) {
        auto pool = ObjPool::fromMemory();
        SmallObjAllocator* allocator = SmallObjAllocator::create(pool.mutPtr());
        constexpr size_t sizes[] = {1, 16, 17, 32, 33, 64, 65, 128, 129, 256, 257, 512, 513, 1024, 1025, smallObjMaxSize};

        for (size_t size : sizes) {
            void* first = allocator->allocate(size);
            allocator->deallocate(first, size);
            void* second = allocator->allocate(size);
            STD_INSIST(second == first);
            allocator->deallocate(second, size);
        }
    }

    STD_TEST(MakeAndReleaseManageObjectLifetime) {
        auto pool = ObjPool::fromMemory();
        SmallObjAllocator* allocator = SmallObjAllocator::create(pool.mutPtr());
        i32 destroyed = 0;

        Tracked* object = allocator->make<Tracked>(42, &destroyed);

        STD_INSIST(object->value == 42);
        STD_INSIST(destroyed == 0);
        allocator->release(object);
        STD_INSIST(destroyed == 1);
    }

    STD_TEST(MakeSupportsMaximumSize) {
        auto pool = ObjPool::fromMemory();
        SmallObjAllocator* allocator = SmallObjAllocator::create(pool.mutPtr());

        MaxSizeObject* object = allocator->make<MaxSizeObject>();
        object->data[0] = 12;
        object->data[smallObjMaxSize - 1] = 34;

        STD_INSIST(object->data[0] == 12);
        STD_INSIST(object->data[smallObjMaxSize - 1] == 34);
        allocator->release(object);
    }
}
