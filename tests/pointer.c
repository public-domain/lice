// pointers

int main(void) {
    int   a   = 1024;
    int  *b   = &a;
    char *c   = "hi";
    char *d   = "hi" + 1;
    char  e[] = "abc";
    char *f   = e + 2;
    char  g[] = "abc";

    *g = 32;
    expecti(*b, 1024);
    expecti(*c, 'h');
    expecti(*d, 'i');
    expecti(*f, 'c');
    expecti(*g, 32);

    int aa[] = { 1, 2, 3 };
    int *p = aa;
    expecti(*p, 1); p++;
    expecti(*p, 2); p++;
    expecti(*p, 3);
    expecti(*p, 3); p--;
    expecti(*p, 2); p--;
    expecti(*p, 1);

    return 0;
}
