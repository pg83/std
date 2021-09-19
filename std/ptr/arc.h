#pragma once

namespace Std {
    class ARC {
        int counter_;

    public:
        ARC() noexcept;

        int ref() noexcept;
        int refCount() const noexcept;
        int unref() noexcept;
    };
}
