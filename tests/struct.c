// structures

struct A {
    int a;
    struct {
        char b;
        int  c;
    } y;
} A;

struct B {
    int a;
} B;

struct C {
    int a;
    int b;
} C;

int main(void) {
    struct {
        int a;
    } a;
    a.a = 1024;
    expecti(a.a, 1024);

    struct {
        int a;
        int b;
    } b;
    b.a = 1024;
    b.b = 2048;
    expecti(b.a, 1024);
    expecti(b.b, 2048);

    struct {
        int a;
        struct {
            char b;
            int  c;
        } b;
    } c;
    c.a   = 1024;
    c.b.b = 32;
    c.b.c = 2048;
    expecti(c.a,   1024);
    expecti(c.b.b, 32);
    expecti(c.b.c, 2048);

    struct name1 {
        int a;
        struct {
            char b;
            int  c;
        } b;
    };
    struct name1 d;
    d.a   = 16;
    d.b.b = 32;
    d.b.c = 64;
    expecti(d.a,   16);
    expecti(d.b.b, 32);
    expecti(d.b.c, 64);

    struct name2 {
        int a;
        int b;
    } e;
    struct name2 *f = &e;

    // obj->ptr
    e.a = 128;
    expecti((*f).a, 128);

    // ptr->obj
    (*f).a = 256;
    expecti(e.a, 256);

    // again
    e.b = 64;
    expecti((*f).b, 64);

    (*f).b = 128;
    expecti(e.b, 128);

    // over read?
    struct {
        int a[3];
        int b[3];
    } g;
    g.a[0] = 1024;
    g.b[1] = 2048;
    expecti(g.a[0], 1024);
    expecti(g.b[1], 2048);
    expecti(g.a[4], 2048); // &g.a[4] == &g.b[0]

    A.a   = 64;
    A.y.b = 65;
    A.y.c = 256;
    expecti(A.a,   64);
    expecti(A.y.b, 65);
    expecti(A.y.c, 256);

    struct B *h = &B;
    B.a = 128;
    expecti((*h).a, 128);
    expecti(B.a,    128);
    expecti(h->a,   128);

    h->a = 256;
    expecti((*h).a, 256);
    expecti(B.a,    256);
    expecti(h->a,   256);

    struct C i[3];
    i[0].a = 32;
    expecti(i[0].a, 32);
    i[0].b = 64;
    expecti(i[0].b, 64);
    i[1].b = 128;
    expecti(i[1].b, 128);
    int *j = i;
    expecti(j[3], 128); // &j[3] == &i[1].b

    struct { char c; } k = { 'a' };
    expecti(k.c, 'a');

    struct { int a[3]; } l = { { 1, 2, 3 } };
    expecti(l.a[0], 1);
    expecti(l.a[1], 2);
    expecti(l.a[2], 3);

    // unnamed shit
    struct {
        union {
            struct {
                int x;
                int y;
            };
            struct {
                char c[8];
            };
        };
    } m;
    m.x = 10;
    m.y = 20;
    expecti(m.c[0], 10);
    expecti(m.c[4], 20);

    // structure copy via assignment
    struct {
        int a;
        int b;
        int c;
    } X, Y, Z;
    X.a = 8;
    X.b = 16;
    X.c = 32;
    Y = X;
    Z.a = 64;
    Z.b = 128;
    Z.c = 256;
    expecti(Y.a, 8);
    expecti(Y.b, 16);
    expecti(Y.c, 32);
    Y = Z;
    expecti(Y.a, 64);
    expecti(Y.b, 128);
    expecti(Y.c, 256);

    // arrows
    struct cell {
        int value;
        struct cell *next;
    };
    struct cell aa = { 10, 0 };
    struct cell bb = { 20, &aa };
    struct cell cc = { 30, &bb };
    struct cell *dd = &cc;

    expecti(cc.value,              30);
    expecti(dd->value,             30);
    expecti(dd->next->value,       20);
    expecti(dd->next->next->value, 10);

    dd->value             = 16;
    dd->next->value       = 32;
    dd->next->next->value = 64;
    expecti(dd->value,             16);
    expecti(dd->next->value,       32);
    expecti(dd->next->next->value, 64);

    // addressing
    struct super {
        int a;
        struct {
            int b;
        } y;
    } poke = { 99, 100 };

    int *poke1 = &poke.a;
    int *poke2 = &poke.y.b;

    expecti(*poke1,     99);
    expecti(*poke2,     100);
    expecti(*&poke.a,   99);
    expecti(*&poke.y.b, 100);

    struct super *inward = &poke;
    int *poke3 = &inward->a;
    int *poke4 = &inward->y.b;

    expecti(*poke3,        99);
    expecti(*poke4,        100);
    expecti(*&inward->a,   99);
    expecti(*&inward->y.b, 100);

    // incomplete in the opposite direction
    struct incomplete1;
    struct incomplete2 {
        struct incomplete1 *next;
    };
    struct incomplete1 {
        int value;
    };

    struct incomplete1 in1 = { 3 };
    struct incomplete2 in2 = { &in1 };

    expecti(in2.next->value, 3);

    struct {
        int x;
    } __a = {
        1024
    }, *__b = &__a;
    int *__c = &__b->x;

    expecti(*__c, 1024);


    // bit fields
    union {
        int i;
        struct {
            int a: 5;
            int b: 5;
        };
    } bitfield;
    bitfield.i = 0;
    bitfield.a = 20;
    bitfield.b = 15;
    expecti(bitfield.i, 500); // (15 << 5) + 20 == 500

    struct {
        char a:4;
        char b:4;
    } bitfield2 = { 5, 10 };

    expecti(bitfield2.a, 5);
    expecti(bitfield2.b, 10);

    union {
        int  a: 10;
        char b: 5;
        char c: 5;
    } bitfield3;

    bitfield3.a = 2;
    expecti(bitfield3.a, 2);
    expecti(bitfield3.b, 2);
    expecti(bitfield3.c, 2);

    struct __empty {};
    expecti(sizeof(struct __empty), 0);

    // FAMS
    struct __fam0 { int a, b[]; };
    struct __fam1 { int a, b[0]; };

    expecti(sizeof(struct __fam0), 4);
    expecti(sizeof(struct __fam1), 4);

    struct __gccfam { int a[0]; };
    expecti(sizeof(struct __gccfam), 0);

    struct __famoverwrite { int a, b[]; };
    struct __famoverwrite OV = { 1, 2, 3, 4, 5 };

    expecti(OV.b[0], 2);
    expecti(OV.b[1], 3);
    expecti(OV.b[2], 4);
    expecti(OV.b[3], 5);

    return 0;
}
