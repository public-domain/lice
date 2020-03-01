// typeof keyword

int main(void) {
    // basic usage of it
    typeof(int) a = 1024;
    expecti(a, 1024);

    typeof(a) b = 2048;
    expecti(b, 2048);

    __typeof__(int) aa = 1024;
    expecti(aa, 1024);

    __typeof__(aa) bb = 2048;
    expecti(bb, 2048);


    // arrays?
    char c[] = "hello";
    typeof(c) d = "world";

    expectstr(d, "world");
    expecti(sizeof(d), 6);

    typeof(typeof (char *)[4]) odd;
    expecti(sizeof(odd)/sizeof(*odd), 4);


    char cc[] = "hello";
    __typeof__(cc) dd = "world";

    expectstr(dd, "world");
    expecti(sizeof(dd), 6);

    __typeof__(__typeof__ (char *)[4]) oddd;
    expecti(sizeof(oddd)/sizeof(*oddd), 4);

    // struct union enum
    typeof(struct { int a; }) __1 = { .a = 1 };
    typeof(union  { int a; }) __2 = { .a = 1 };
    typeof(enum   { A1,B2  }) __3 = { B2 };

    expecti(__1.a, 1);
    expecti(__2.a, 1);
    expecti(__3,   B2);

    __typeof__(struct { int a;  }) __11 = { .a = 1 };
    __typeof__(union  { int a;  }) __22 = { .a = 1 };
    __typeof__(enum   { A11,B22 }) __33 = { B22 };

    expecti(__11.a, 1);
    expecti(__22.a, 1);
    expecti(__33,   B22);

    return 0;
}
