// initializers

void testi(int *recieved, int *wanted, int length) {
    for (int i = 0; i < length; i++)
        expecti(recieved[i], wanted[i]);
}

void tests(short *recieved, short *wanted, int length) {
    for (int i = 0; i < length; i++)
        expects(recieved[i], wanted[i]);
}

void testarray(void) {
    int a[]     = { 1, 2, 3 };
    int b[]     = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    int c[3][3] = { { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } };
    int d[3][3] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    int e[]     = { 1, 0, 0, 2, 0, 0, 3, 0, 0, 4, 0, 0 };
    int f[4][3]  = { { 1 }, { 2 }, { 3 }, { 4 } };

    expecti(a[0], 1);
    expecti(a[1], 2);
    expecti(a[2], 3);

    testi(b, c, 9);
    testi(c, d, 9);
    testi(e, f, 12);

    short g[] = {
        1, 0, 0, 0, 0,
        0, 2, 3, 0, 0,
        0, 0, 4, 5, 6
    };

    short h[4][3][2] = {
        { 1 },
        { 2, 3 },
        { 4, 5, 6 }
    };

    tests(g, h, 15);
}

void teststruct(void) {
    int a[] = {
        1, 0, 0, 0,
        2, 0, 0, 0
    };

    struct {
        int a[3];
        int b;
    } data[] = { { 1 }, 2 };

    testi(&data, a, 8);
}

void testnesting(void) {
    struct {
        struct {
            struct {
                int a;
                int b;
            } a;

            struct {
                char a[8];
            } b;
        } a;
    } data = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

    expecti(data.a.a.a,    1);
    expecti(data.a.a.b,    2);
    expecti(data.a.b.a[0], 3);
    expecti(data.a.b.a[1], 4);
    expecti(data.a.b.a[2], 5);
    expecti(data.a.b.a[3], 6);
    expecti(data.a.b.a[4], 7);
    expecti(data.a.b.a[5], 8);
    expecti(data.a.b.a[6], 9);
    expecti(data.a.b.a[7], 10);
}

void testdesignated(void) {
    struct { int a, b, c; } a = {
        .a = 1024,
        .b = 2048,
        .c = 4096,
    };

    struct { int a, b, c; } b = {
        .c = 4096,
        .b = 2048,
        .a = 1024
    };

    struct { int a, b, c; } c = {
        .b = 2048,
        4096, // should be c
        .a = 1024
    };

    expecti(a.a, 1024);
    expecti(a.b, 2048);
    expecti(a.c, 4096);
    expecti(b.a, 1024);
    expecti(b.b, 2048);
    expecti(b.c, 4096);
    expecti(c.a, 1024);
    expecti(c.b, 2048);
    expecti(c.c, 4096);

    // array designated initializers
    int array[3] = { [1] = 1024 };
    expecti(array[0], 0);
    expecti(array[1], 1024);
    expecti(array[2], 0);

    // for structs too
    struct {
        int a, b;
    } data[2] = { [1] = { 1, 2 } };

    expecti(data[1].a, 1);
    expecti(data[1].b, 2);
    expecti(data[0].a, 0);
    expecti(data[0].b, 0);

    // splatted too
    struct {
        int a, b;
    } data2[3] = { [1] = 10, 20, 30, 40 };
    expecti(data2[0].a, 0);
    expecti(data2[0].b, 0);
    expecti(data2[1].a, 10);
    expecti(data2[1].b, 20);
    expecti(data2[2].a, 30);
    expecti(data2[2].b, 40);

    struct {
        struct {
            int a, b;
        } c[3];
    } again[] = {
        [1].c[0].b = 10, 20, 30, 40, 50,
        [0].c[2].b = 60, 70
    };
    expecti(10, again[1].c[0].b);
    expecti(20, again[1].c[1].a);
    expecti(30, again[1].c[1].b);
    expecti(40, again[1].c[2].a);
    expecti(50, again[1].c[2].b);
    expecti(60, again[0].c[2].b);
    expecti(70, again[1].c[0].a);

    int A[][3] = { [0][0] = 10, [1][0] = 20 };
    struct { int a, b[3]; } B = { .a = 10, .b[0] = 20, .b[1] = 30 };

    expecti(A[0][0], 10);
    expecti(A[1][0], 20);
    expecti(B.a, 10);
    expecti(B.b[0], 20);
    expecti(B.b[1], 30);
    expecti(B.b[2], 0);
}

void testorder(void) {
    int order[4] = { [2] = 30, [0] = 10, 20 };

    expecti(order[0], 10);
    expecti(order[1], 20);
    expecti(order[2], 30);
}

void testzero(void) {
    struct { int a, b; } a = { 1024 };
    struct { int a, b; } b = { .b = 1024 };

    expecti(a.a, 1024);
    expecti(a.b, 0);
    expecti(b.a, 0);
    expecti(b.b, 1024);
}

void testtypedef(void) {
    typedef int array[];
    array a = { 1, 2, 3 };
    array b = { 4, 5, 6 };

    expecti(sizeof(a) / sizeof(*a), 3);
    expecti(sizeof(b) / sizeof(*b), 3);

    expecti(a[0], 1);
    expecti(a[1], 2);
    expecti(a[2], 3);
    expecti(b[0], 4);
    expecti(b[1], 5);
    expecti(b[2], 6);
}

int main(void) {
    testarray();
    testnesting();
    teststruct();
    testdesignated();
    testzero();
    testtypedef();
    testorder();

    int primitive = { 1024 };
    expecti(primitive, 1024);

    // test odd string initializers
    char f[] = "abc";     expectstr(f, "abc");
    char g[] = { "cba" }; expectstr(g, "cba");

    return 0;
}
