// bitwise operators

int main(void) {
    expecti(1 | 2, 3);
    expecti(2 | 5, 7);
    expecti(2 | 7, 7);

    expecti(1 & 2, 0);
    expecti(2 & 7, 2);

    expecti(~0, -1);
    expecti(~2, -3);
    expecti(~-1, 0);

    expecti(15 ^ 5, 10);

    expecti(1  << 4, 16);
    expecti(3  << 4, 48);
    expecti(15 >> 3, 1);
    expecti(8  >> 2, 2);

    return 0;
}
