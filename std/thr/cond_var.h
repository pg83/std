#pragma once

namespace stl {
    class Mutex;

    class CondVar {
        struct Impl;
        Impl* impl;

    public:
        CondVar();
        ~CondVar();

        void wait(Mutex& mutex);
        void signal();
        void broadcast();
    };
}
