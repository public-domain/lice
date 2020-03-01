// numeric constants

int main(void) {
    expecti(0x1,  1);
    expecti(0xf,  15);
    expecti(0xF,  15);

    expecti(3L,   3);
    expecti(3LL,  3);
    expecti(3UL,  3);
    expecti(3LU,  3);
    expecti(3ULL, 3);
    expecti(3LU,  3);
    expecti(3LLU, 3);
    expecti(3l,   3);
    expecti(3ll,  3);
    expecti(3ul,  3);
    expecti(3lu,  3);
    expecti(3ull, 3);
    expecti(3lu,  3);
    expecti(3llu, 3);

    expectf(1.0f, 1.0);
    expectf(1.2f, 1.2);
    expectf(1.0f, 1.0f);
    expectf(1.2f, 1.2f);

    expectd(3.14159265,     3.14159265);
    expectd(2e2,            200.0);
    expectd(1.55e1,         15.5);
    expectd(0x0.DE488631p8, 0xDE.488631);

    expectl(0xFL,   15L);
    expectl(0xFULL, 15ULL);

    expecti(0b1011, 11);
    expecti(0xe0,   224);
    expecti(0xE0,   224);

    expecti(sizeof(0xe0), 4); // should be integer type

    return 0;
}
