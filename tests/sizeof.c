// sizeof operator

int main(void) {
    expecti(sizeof(char),   1);
    expecti(sizeof(short),  2);
    expecti(sizeof(int),    4);
    expecti(sizeof(long),   8);
    expecti(sizeof(char*),  8);
    expecti(sizeof(short*), 8);
    expecti(sizeof(int*),   8);
    expecti(sizeof(long*),  8);

    expecti(sizeof(unsigned char),  1);
    expecti(sizeof(unsigned short), 2);
    expecti(sizeof(unsigned int),   4);
    expecti(sizeof(unsigned long),  8);

    expecti(sizeof 1,    4);
    expecti(sizeof 1L,   8);
    expecti(sizeof 1.0f, 4);
    expecti(sizeof 1.0,  8);
    expecti(sizeof 'a',  4);
    expecti(sizeof('b'), 4);

    expecti(sizeof(char[1]),      1);
    expecti(sizeof(char[2]),      2);
    expecti(sizeof(char[3]),      3);
    expecti(sizeof(char[1][10]),  10);
    expecti(sizeof(char[10][1]),  10);
    expecti(sizeof(char[10][10]), 100);
    expecti(sizeof(int[4][2]),    32);
    expecti(sizeof(int[2][4]),    32);

    char  a[] = { 1, 2, 3 };
    char  b[] = "abc";
    char *c[5];
    char *(*d)[3];

    expecti(sizeof(a),    3);
    expecti(sizeof(b),    4);
    expecti(sizeof(c),    40);
    expecti(sizeof(d),    8);
    expecti(sizeof(*d),   24);
    expecti(sizeof(**d),  8);
    expecti(sizeof(***d), 1);

    char _not_int_;
    expecti(sizeof((int)_not_int_), 4); // cast makes it sizeof(int)

    // the more complicated syntax cases
    expecti(sizeof(b[0]),     1);
    expecti(sizeof((b[0])),   1);
    expecti(sizeof((b)[0]),   1);
    expecti(sizeof(((b)[0])), 1);

    return 0;
}
