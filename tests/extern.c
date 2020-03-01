// external linkage

extern int expecti(int, int);
extern int external_1;
int extern external_2;

int main(void) {
    expecti(external_1, 1337);
    expecti(external_2, 7331);

    return 0;
}
