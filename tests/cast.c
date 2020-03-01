// casts

int main(void) {
    expecti((int)1,     1);
    expectf((float)2,   2.0);
    expectd((double)3,  3.0);

    int a[3];
    *(int *)(a + 2) = 1024;
    expecti(a[2], 1024);

    return 0;
}
