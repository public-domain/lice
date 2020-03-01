// scope

int main(void) {
    int i = 1024;

    {
        int i = 2048;
        expecti(i, 2048);
        i = 4096;
    }

    expecti(i, 1024);

    {
        int i = 4096;
        expecti(i, 4096);
        i = 2046;
    }

    expecti(i, 1024);

    return 0;
}
