// arithmetic

int main(void) {
    int i = 0 - 1;

    expecti(0, 0);
    expecti(1, 1);

    expecti(1 + 2, 3);
    expecti(2 - 1, 1);

    expecti(1 + 2 + 3 + 4, 10);
    expecti(1 + 2 * 3 + 4, 11);
    expecti(1 * 2 + 3 * 4, 14);

    expecti(4 / 2 + 6 / 3, 4);
    expecti(24 / 2 / 3 ,   4);

    expecti(24 % 7, 3);
    expecti(24 % 3, 0);

    expecti('a' + 1, 98);
    expecti('b' + 1, 99);
    expecti('a' + 2, 'b' + 1);
    expecti('b' + 1, 'c');

    expecti(i,   0 - 1);
    expecti(i,  -1);
    expecti(i+1, 0);

    expecti(1, +1);

    i = 16;
    expecti(i++, 16);
    expecti(i,   17);
    expecti(i--, 17);
    expecti(i,   16);
    expecti(++i, 17);
    expecti(i,   17);
    expecti(--i, 16);
    expecti(i,   16);

    expecti(!1, 0);
    expecti(!0, 1);

    expecti((1 + 2) ? 128 : 256,   128);
    expecti((1 - 1) ? 128 : 256,   256);
    expecti((1 - 1) ? 64  : 256/2, 128);
    expecti((1 - 0) ? 256 : 64/2,  256);


    expecti((1 + 2) ?: 1024,       3);

    return 0;
}
