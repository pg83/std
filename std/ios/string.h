#include "output.h"

namespace Std {
    class DynString;

    class StringOutput: public Output {
        DynString* str_;

    public:
        virtual ~StringOutput();

        inline StringOutput(DynString* str) noexcept
            : str_(str)
        {
        }

    private:
        void writeImpl(const void* ptr, size_t len) override;
    };
}
