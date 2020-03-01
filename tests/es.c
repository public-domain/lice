// expression statements

#define maxint(A, B) \
    ({ int a = (A), b = (B); a > b ? a : b; })

int main(void) {

    expecti(({ 'a'; 1; 64; }), 64);

    expecti(
        ({
            int a = 10;
            a;
        }), 10
    );

    expecti(
        ({
            int i = 0;
            for (; i < 10; i++)
                ;
            i;
        }), 10
    );

    expecti(maxint(100, 5), 100);

    expecti(({ (int){1}; }), 1);

    return 0;
}
