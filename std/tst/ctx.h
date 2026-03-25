#pragma once

namespace stl {
    struct Ctx {
        int argc;
        char** argv;

        virtual void printTB() const;

        void run();
    };
}
