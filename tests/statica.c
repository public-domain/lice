// static assertions

int main(void) {
    _Static_assert(1, "failed");

    struct {
        _Static_assert(1, "failed");
    } _;

    return 0;
}
