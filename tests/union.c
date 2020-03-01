// unions

int global = 5;
int *pointer = &global;

int main(void) {
    union {
        int a;
        int b;
    } a = { 128 };

    union {
        char a[4];
        int  b;
    } b = { .b = 0 };

    b.a[1] = 1;

    union {
        char a[4];
        int  b;
    } c = { 0, 1, 0, 0 };

    expecti(a.b, 128);
    expecti(b.b, 256);
    expecti(c.b, 256);

    expecti(*pointer, 5);

    return 0;
}
