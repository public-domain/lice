// arithmetic on void

int main(void) {
    expecti(sizeof(void), 1);
    expecti(sizeof(main), 8); // sizeof function == sizeof(&function)
                              // i.e the function pointer for that
                              // function (GCC extension)

    int a[] = { 1, 2, 3 };
    void *p = (void*)a;

    expecti(*(int*)p, 1); p += 4;
    expecti(*(int*)p, 2); p += 4;
    expecti(*(int*)p, 3);

    return 0;
}
