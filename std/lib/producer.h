#pragma once

namespace stl {
    struct ProducerIface {
        virtual void* run() = 0;
        virtual void del(void*) = 0;
        virtual ~ProducerIface() noexcept;
    };

    template <typename F>
    struct Producer: public ProducerIface {
        F fn;

        Producer(F f)
            : fn(f)
        {
        }

        void* run() override {
            return new decltype(fn())(fn());
        }

        void del(void* p) override {
            delete (decltype(fn())*)p;
        }
    };

    template <typename F>
    ProducerIface* makeProducer(F f) {
        return new Producer<F>(f);
    }
}
