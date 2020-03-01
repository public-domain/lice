// bool

#include <stdbool.h>

int main(void) {
    expecti(__bool_true_false_are_defined, 1);

    _Bool a = 0;
    _Bool b = 1;
    _Bool c = 2;
    _Bool d = 3;

    expecti(a, 0);
    expecti(b, 1);
    expecti(c, 1);
    expecti(d, 1);

    a = 3;
    b = 2;
    c = 1;
    d = 0;

    expecti(a, 1);
    expecti(b, 1);
    expecti(c, 1);
    expecti(d, 0);

    bool a1 = false;
    a1 = !a1;
    a1 = (a1) ? true : false;
    expecti(a1, true);
}
