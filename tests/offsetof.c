// offsetof

#include <stddef.h>

struct test {
    int   a; // 0
    int   b; // 4
    int   c; // 8
    char  d; // 12
    short e; // 14
    int   f; // 16
};

int main(void) {
    expecti(offsetof(struct test, a), 0);
    expecti(offsetof(struct test, b), 4);
    expecti(offsetof(struct test, c), 8);
    expecti(offsetof(struct test, d), 12);
    expecti(offsetof(struct test, e), 14);
    expecti(offsetof(struct test, f), 16);

    // it should also be a valid integer constant expression
    int a[offsetof(struct test, b)];
    expecti(sizeof(a), sizeof(int) * 4);

    return 0;
}
