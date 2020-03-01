// comparison

int main(void) {
    expecti(1 < 2,  1);
    expecti(1 > 2,  0);
    expecti(1 == 1, 1);
    expecti(1 == 2, 0);
    expecti(1 <= 2, 1);
    expecti(2 <= 2, 1);
    expecti(2 <= 1, 0);
    expecti(1 >= 2, 0);
    expecti(2 >= 2, 1);
    expecti(2 >= 1, 1);
    expecti(1 != 1, 0);
    expecti(1 != 0, 1);
    expecti(1.0f == 1.0f, 1);
    expecti(1.0f == 2.0f, 0);
    expecti(1.0f != 1.0f, 0);
    expecti(1.0f != 2.0f, 1);
    expecti(1.0  == 1.0f, 1);
    expecti(1.0  == 2.0f, 0);
    expecti(1.0  != 1.0f, 0);
    expecti(1.0  != 2.0f, 1);
    expecti(1.0f == 1.0,  1);
    expecti(1.0f == 2.0,  0);
    expecti(1.0f != 1.0,  0);
    expecti(1.0f != 2.0,  1);
    expecti(1.0  == 1.0,  1);
    expecti(1.0  == 2.0,  0);
    expecti(1.0  != 1.0,  0);
    expecti(1.0  != 2.0,  1);

    return 0;
}
