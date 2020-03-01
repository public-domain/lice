// enumerations

enum {
    A,
    B,
    C
};

enum {
    D,
    E,
    F
};

int main(void) {
    expecti(A, 0);
    expecti(B, 1);
    expecti(C, 2);
    expecti(D, 0);
    expecti(E, 1);
    expecti(F, 2);

    enum { G } enum1;
    enum { H };

    expecti(G, 0);
    expecti(H, 0);

    enum named {
        I,
        J,
        K
    };

    enum named enum2 = I;
    expecti(enum2, 0);

    return 0;
}
