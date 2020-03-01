// arrays

void pass(int x[][3], int want) {
    expecti(*(*(x + 1) + 1), want);
}

int main(void) {
    int  a[2][2];
    int *b = a;

    *b = 1024;
    expecti(*b, 1024);

    int d[2][3];
    int *e = d + 1;
    *e = 1;
    int *f = d;
    *e = 64;
    expecti(*(f + 3), 64);

    int g[4][5];
    int *h = g;
    *(*(g + 1) + 2) = 1024;
    expecti(*(h + 7), 1024);

    int i[] = { 1, 2, 3 };
    expecti(i[0], 1);
    expecti(i[1], 2);
    expecti(i[2], 3);

    int j[2][3];
    j[0][1] = 100;
    j[1][1] = 200;
    int *k = j;
    expecti(k[1], 100);
    expecti(k[4], 200);

    int l[2][3];
    int *m = l;
    *(m + 4) = 4096;
    pass(l, 4096);

    int n[5*5];
    n[10] = 25;
    expecti(n[10], 25);

    return 0;
}
