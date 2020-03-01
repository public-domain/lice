// alignof

#include <alignof.h>

int main(void) {
    expecti(__alignof_is_defined, 1);

    expecti(alignof(char), 1);
    expecti(alignof(int),  4);
    expecti(alignof(long), 8);

    struct test {
        char a;
        int  b;
    };

    expecti(alignof(struct test), 8);

    expecti(_Alignof(char), 1);
    expecti(__alignof__(char), 1);
    expecti(_Alignof(struct test), 8);
    expecti(__alignof__(struct test), 8);

    struct complex {
        struct {
            char a;
            short b;
            int c;
        } d;
        char e;
        union {
            int f;
            struct {
                int g;
                int h;
            } i;
        } j;
    };

    expecti(alignof(struct complex), 16);

    return 0;
}
