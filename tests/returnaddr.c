// __builtin_return_address

static void *a(void) {
    return __builtin_return_address(1);
}

static void *b(void) {
    expecti(__builtin_return_address(0), a());
    return __builtin_return_address(0);
}

static void *c(void) {
_1:
    void *this = b();
_2:
    expecti((&&_1 < this && this <= &&_2), 1);

    return this;
}

int main(void) {
    expecti(c() <= b(), 1);

    return 0;
}
