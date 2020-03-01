// integers

int main(void) {
    short     a = 100;
    short int b = 150;
    long      c = 2048;
    long int  d = 4096;
    int       e = 214748364;

    expects(a + b,   250);
    expects(a + 100, 200);

    expectl(c,     2048);
    expectl(d * 2, 8192);
    expectl(100L,  100L);

    expectl(922337203685477807,     922337203685477807);
    expectl(922337203685477806 + 1, 922337203685477807);

    expecti(e, 214748364);

    return 0;
}
