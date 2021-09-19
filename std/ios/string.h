#include "output.h"

namespace Std {
    struct DynString;

    class StringOutput: public ZeroCopyOutput {
        DynString* str_;

    public:
        ~StringOutput() override;

        inline StringOutput(DynString& str) noexcept
            : str_(&str)
        {
        }

    private:
        void writeImpl(const void* ptr, size_t len) override;
        size_t imbueImpl(void** ptr) noexcept override;
        void* imbueImpl(size_t len) override;
        void bumpImpl(const void* ptr) noexcept override;
    };
}
