#include "output.h"

namespace Std {
    class DynString;

    class StringOutput: public ZeroCopyOutput {
        DynString* str_;

    public:
        virtual ~StringOutput();

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
