// compound assignment

int main(void) {
    int a = 5;
    expecti((a += 5), 10);
    expecti((a -= 5), 5);
    expecti((a *= 5), 25);
    expecti((a /= 5), 5);
    expecti((a %= 3), 2);

    int b = 14;
    expecti((b &= 7), 6);
    expecti((b |= 8), 14);
    expecti((b ^= 3), 13);
    expecti((b <<= 2), 52);
    expecti((b >>= 2), 13);

    return 0;
}
