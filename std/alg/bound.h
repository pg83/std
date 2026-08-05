#pragma once

namespace stl {
    // First position in the sorted range [b, e) whose element is not less
    // than value; e when every element is smaller.
    template <typename I, typename T>
    I lowerBound(I b, I e, const T& value) {
        while (b != e) {
            I m = b + (e - b) / 2;

            if (*m < value) {
                b = m + 1;
            } else {
                e = m;
            }
        }

        return b;
    }

    // First position in the sorted range [b, e) whose element is greater
    // than value; e when no element is.
    template <typename I, typename T>
    I upperBound(I b, I e, const T& value) {
        while (b != e) {
            I m = b + (e - b) / 2;

            if (value < *m) {
                e = m;
            } else {
                b = m + 1;
            }
        }

        return b;
    }

    // True when the sorted range [b, e) contains value.
    template <typename I, typename T>
    bool binaryContains(I b, I e, const T& value) {
        I at = lowerBound(b, e, value);

        return at != e && !(value < *at);
    }
}
