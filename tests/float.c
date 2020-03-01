// floats

float  ffunc1(float  arg) { return arg; }
float  ffunc2(double arg) { return arg; }
float  ffunc3(int    arg) { return arg; }
double dfunc1(float  arg) { return arg; }
double dfunc2(double arg) { return arg; }
double dfunc3(int    arg) { return arg; }

// deal with recursive calls for floats / doubles
// requires stack alignment on some architectures
// to properly work.
float frecurse(float a) {
    if (a < 10)
        return a;
    return frecurse(3.14);
}

double drecurse(double a) {
    if (a < 10)
        return a;
    return drecurse(6.28);
}

int main(void) {
    // all float
    expectf(1.0,       1.0);
    expectf(1.0 + 0.5, 1.5);
    expectf(1.0 - 0.5, 0.5);
    expectf(1.0 * 2.0, 2.0);
    expectf(1.0 / 4.0, 0.25);

    // float and int
    expectf(1.0,       1.0);
    expectf(1.0 + 1,   2.0);
    expectf(1.0 - 1,   0.0);
    expectf(1.0 * 2,   2.0);
    expectf(1.0 / 4,   0.25);

    expectf(ffunc1(3.14f), 3.14f);
    expectf(ffunc1(3.0f),  3.0f);
    expectf(ffunc2(3.14f), 3.14f);
    expectf(ffunc2(3.0f),  3.0f);
    expectf(ffunc3(3.14f), 3.0f);
    expectf(ffunc3(3),    3);
    expectd(dfunc1(1.0),  1.0);
    expectd(dfunc1(10.0), 10.0);
    expectd(dfunc2(2.0),  2.0);
    expectd(dfunc2(10),   10.0);
    expectd(dfunc3(11.5), 11.0);
    expectd(dfunc3(10),   10.0);
    // Bug: these are still broken
    //expectf(frecurse(1024), 3.14);
    //expectd(drecurse(1024), 6.28);
    float a = 1024.0f;
    float b = a;
    expectf(a, 1024.0f);
    expectf(b, 1024.0f);

    double c = 2048.0;
    double d = c;
    expectd(c, 2048.0);
    expectd(d, 2048.0);

    expectf(0.7, .7);

    return 0;
}
