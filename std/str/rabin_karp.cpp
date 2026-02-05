#include "rabin_karp.h"

using namespace Std;

namespace {
    template <u64 M>
    struct Hash {
        static const u64 B = 31;

        u64 h = 0;
        u64 p = 1;
        u64 t = 0;

        void add(u8 c) {
            h = (h * B + c) % M;
        }

        void addt(u8 c) {
            t = (t * B + c) % M;
        }

        void mulp() {
            p = (p * B) % M;
        }

        void roll(u8 out, u8 in) {
            t = (t + M - (out * p) % M) % M;
            t = (t * B + in) % M;
        }

        bool eq() const {
            return h == t;
        }
    };

    class RabinKarp {
        Hash<1000000007> h1;
        Hash<1000000009> h2;
        Hash<998244353>  h3;
        Hash<1000000021> h4;

        size_t len;

    public:
        RabinKarp(const u8* s, size_t n)
            : len(n)
        {
            for (size_t i = 0; i < n; i++) {
                h1.add(s[i]);
                h2.add(s[i]);
                h3.add(s[i]);
                h4.add(s[i]);

                if (i + 1 < n) {
                    h1.mulp();
                    h2.mulp();
                    h3.mulp();
                    h4.mulp();
                }
            }
        }

        bool find(const u8* txt, size_t tn, size_t* res) {
            if (len > tn) {
                return false;
            }

            for (size_t i = 0; i < len; i++) {
                h1.addt(txt[i]);
                h2.addt(txt[i]);
                h3.addt(txt[i]);
                h4.addt(txt[i]);
            }

            if (h1.eq() && h2.eq() && h3.eq() && h4.eq()) {
                return 0;
            }

            for (size_t i = len; i < tn; i++) {
                h1.roll(txt[i - len], txt[i]);
                h2.roll(txt[i - len], txt[i]);
                h3.roll(txt[i - len], txt[i]);
                h4.roll(txt[i - len], txt[i]);

                if (h1.eq() && h2.eq() && h3.eq() && h4.eq()) {
                    return (*res = i - len + 1, true);
                }
            }

            return false;
        }
    };
}

bool Std::findRK(const u8* str, size_t strLen, const u8* substr, size_t substrLen, size_t* pos) noexcept {
    return RabinKarp(substr, substrLen).find(str, strLen, pos);
}
