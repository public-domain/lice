// global variables

int  a = 1024;
int *b = &a;

int  c[10];
int  d[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

int  e, f;
int  g, h = 10;
int  i = 11, j;

char  k[] = "hello";
char *l   = "world";

long m  = 32;
int *n  = &(int) { 64 };

int main(void) {
    expecti(a, 1024);

    // can write to global?
    a = 2048;
    expecti(a, 2048);
    expecti(*b, 2048);

    c[1] = 2;
    expecti(c[1], 2);
    expecti(d[1], 2);

    e = 7;
    f = 8;
    expecti(e, 7);
    expecti(f, 8);
    g = 9;
    expecti(g, 9);
    expecti(h, 10);
    expecti(i, 11);
    j = 12;
    expecti(j, 12);

    expectstr(k, "hello");
    expectstr(l, "world");

    expectl(m, 32);
    expectl(*n, 64);

    return 0;
}
