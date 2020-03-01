// functions

int i1()     { return 42; }
int i2(void) { return 42; }

int splat(int a, int b, int c, int d, int e, int f) {
    expecti(a, 1);
    expecti(b, 2);
    expecti(c, 3);
    expecti(d, 4);
    expecti(e, 5);
    expecti(f, 6);
}

int deref(int *a) {
    return *a;
}

int proto1();
int proto1() { return 1024; }

int proto2(int a, int b);
int proto2(int a, int b) {
    return a + b;
}

void proto3(int a, ...);
void proto3(int a, ...) {
    expecti(a, 1024);
}

void ignore(void) {
    return;
}

void ___func___(void) {
    expectstr(__func__, "___func___");
}

int funptr1(void) {
    return 1024;
}

int funptr2(int a) {
    return funptr1() * 2;
}

float funptr3(float a) {
    return a * 2;
}

int funptr4(int (*callback)(void), int *data) {
    *data = callback();
    return *data;
}

// emptyies
void empty1(void) {
    // nothing to see here
}

void empty2(void) {
    // empty semicolons
    ;;;
}

int main(void) {
    expecti(i1(), 42);
    expecti(i2(), 42);

    int a = 1024;
    expecti(deref(&a),       1024);
    expecti(proto1(),        1024);
    expecti(proto2(512,512), 1024);

    proto3(1024);
    splat(1, 2, 3, 4, 5, 6);

    ignore();
    ___func___();

    // function pointer tests
    int   (*ptr1)(void)              = funptr1;
    int   (*ptr2)(int)               = funptr2;
    float (*ptr3)(float)             = funptr3;
    int   (*ptr4)(int(*)(void),int*) = funptr4;

    expecti(ptr1(),        1024);
    expecti(ptr2(a),       2048);
    expecti(ptr4(ptr1,&a), 1024);
    expecti(a,             1024);
    expectf(ptr3(3.14),    6.28);

    empty1();
    empty2();

    return 0;
}
