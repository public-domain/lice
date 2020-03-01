// types

int main(void) {
    char a;
    short b;
    int c;
    long d;
    long long e;
    short int f;
    long int g;
    long long int f;
    long int long g;
    float h;
    double i;
    long double j;

    signed char k;
    signed short l;
    signed int m;
    signed long n;
    signed long long o;
    signed short int p;
    signed long int q;
    signed long long int r;

    unsigned char s;
    unsigned short t;
    unsigned int u;
    unsigned long v;
    unsigned long long w;
    unsigned short int x;
    unsigned long int y;
    unsigned long long int z;

    static A;
    auto B;
    register C;
    static int D;
    auto int E;
    register int F;

    int *G;
    int *H[5];
    int (*I)[5];
    expecti(sizeof(G), 8);
    expecti(sizeof(H), 40);
    expecti(sizeof(I), 8);

    int unsigned auto* const* const* J;

    typedef int K;
    K L = 5;
    expecti(L, 5);

    typedef K M[3];
    M N = { 1, 2, 3 };
    expecti(N[0], 1);
    expecti(N[1], 2);
    expecti(N[2], 3);

    typedef struct { int a; } O;
    O P;
    P.a = 64;
    expecti(P.a, 64);

    typedef int __take_precedence_1;
    typedef int __take_precedence_2;
    __take_precedence_1 __take_precedence_2 = 100;
    expecti(__take_precedence_2, 100);

    return 0;
}
