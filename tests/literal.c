// literals

struct s1 {
    int a;
};

struct s2 {
    int a;
    struct s3 {
        int a[2];
    } *b;
};


int        a = (int)  { 42 };
int       *b = &(int) { 42 };
struct s1 *c = &(struct s1) { 42 };
struct s2 *d = &(struct s2) { 42 , &(struct s3) { 42, 42 } };

int main(void) {
    expecti('a', 97);
    expecti('A', 65);

    //expecti('\0', 0);
    expecti('\a', 7);
    expecti('\b', 8);
    expecti('\t', 9);
    expecti('\n', 10);
    expecti('\v', 11);
    expecti('\f', 12);
    expecti('\r', 13);

    expecti('\7', 7);
    expecti('\17', 15);
    expecti('\235', -99);

    expecti('\x0', 0);
    expecti('\xff', -1);
    expecti('\x012', 18);


    expectstr("hello world", "hello world");
    expecti("hello world"[5], ' ');
    expecti("hello world"[11], 0); // null terminator

    expectstr(
        (char []) { 'h', 'e', 'l', 'l', 'o', 0 },
        "hello"
    );

    expecti(L'c', 'c');
    expectstr(L"butt", "butt");

    // lexer could fail for 'L' so we need to ensure that it can be
    // used as an identifier still.
    int L = 1337;
    expecti(L, 1337);
    int L1 = L;
    expecti(L1, L);

    expecti(a,          42);
    expecti(*b,         42);
    expecti(c->a,       42);
    expecti(d->a,       42);
    expecti(d->b->a[0], 42);
    expecti(d->b->a[1], 42);

    // universal (unicode / host defined)
    expecti(L'\u0024', '$');
    expecti(L'\U00000024', '$');
    expectstr("\u0024", "$");
    expectstr("\U00000024", "$");

    return 0;
}
