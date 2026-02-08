#include "defer.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(Defer) {
    STD_TEST(BasicDefer) {
        bool executed = false;

        {
            STD_DEFER {
                executed = true;
            };
        }

        STD_INSIST(executed);
    }

    STD_TEST(MultipleDefers) {
        int counter = 0;

        {
            STD_DEFER {
                counter = counter * 10 + 1;
            };

            STD_DEFER {
                counter = counter * 10 + 2;
            };

            STD_DEFER {
                counter = counter * 10 + 3;
            };
        }

        STD_INSIST(counter == 321);
    }

    STD_TEST(DeferWithCapture) {
        int value = 42;
        bool captured = false;

        {
            STD_DEFER {
                captured = (value == 42);
            };
        }

        STD_INSIST(captured);
    }

    STD_TEST(DeferWithMutableCapture) {
        int value = 10;

        {
            STD_DEFER {
                value = 20;
            };
        }

        STD_INSIST(value == 20);
    }

    STD_TEST(DeferExecutesOnException) {
        bool executed = false;

        try {
            STD_DEFER {
                executed = true;
            };

            throw 42;
        } catch (...) {
        }

        STD_INSIST(executed);
    }

    STD_TEST(DeferWithComplexStatements) {
        int sum = 0;

        {
            STD_DEFER {
                for (int i = 0; i < 5; ++i) {
                    sum += i;
                }
            };
        }

        STD_INSIST(sum == 10);
    }

    STD_TEST(NestedScopes) {
        int outer = 0;
        int inner = 0;

        {
            STD_DEFER {
                outer = 1;
            };

            {
                STD_DEFER {
                    inner = 2;
                };
            }

            STD_INSIST(inner == 2);
            STD_INSIST(outer == 0);
        }

        STD_INSIST(outer == 1);
    }

    STD_TEST(DeferWithPointerOperations) {
        int* ptr = new int(100);
        bool deleted = false;

        {
            STD_DEFER {
                delete ptr;
                deleted = true;
            };
        }

        STD_INSIST(deleted);
    }

    STD_TEST(DeferModifyingLocalVariable) {
        int x = 5;

        {
            STD_DEFER {
                x *= 2;
            };

            x += 3;
        }

        STD_INSIST(x == 16);
    }

    STD_TEST(DeferWithReturnValue) {
        auto test = []() -> int {
            int result = 0;

            {
                STD_DEFER {
                    result = 42;
                };

                return result;
            }
        };

        STD_INSIST(test() == 0);
    }

    STD_TEST(DeferExecutesBeforeReturn) {
        auto test = []() -> int {
            int value = 10;

            {
                STD_DEFER {
                    value = 20;
                };
            }

            return value;
        };

        STD_INSIST(test() == 20);
    }

    STD_TEST(MultipleVariablesCapture) {
        int a = 1;
        int b = 2;
        int c = 3;
        int sum = 0;

        {
            STD_DEFER {
                sum = a + b + c;
            };
        }

        STD_INSIST(sum == 6);
    }

    STD_TEST(DeferInLoop) {
        int counter = 0;

        for (int i = 0; i < 3; ++i) {
            STD_DEFER {
                counter++;
            };
        }

        STD_INSIST(counter == 3);
    }

    STD_TEST(DeferWithConditional) {
        bool flag = false;
        int value = 0;

        {
            STD_DEFER {
                if (flag) {
                    value = 100;
                } else {
                    value = 200;
                }
            };

            flag = true;
        }

        STD_INSIST(value == 100);
    }

    STD_TEST(DeferMutatesBeforeUse) {
        int x = 0;

        {
            x = 10;

            STD_DEFER {
                x *= 2;
            };

            x += 5;
        }

        STD_INSIST(x == 30);
    }
}
