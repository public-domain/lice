// stdarg

#include <stdarg.h>
char buffer[1024];

void testi(int a, ...) {
    va_list ap;
    va_start(ap, a);
    expecti(a,               1);
    expecti(va_arg(ap, int), 2);
    expecti(va_arg(ap, int), 3);
    expecti(va_arg(ap, int), 4);
    expecti(va_arg(ap, int), 5);

    va_end(ap);
}

void testf(float a, ...) {
    va_list ap;
    va_start(ap, a);

    expectf(a,                 1.0f);
    expectf(va_arg(ap, float), 2.0f);
    expectf(va_arg(ap, float), 4.0f);
    expectf(va_arg(ap, float), 8.0f);

    va_end(ap);
}

void testm(char *p, ...) {
    va_list ap;
    va_start(ap, p);

    expectstr(p,                 "hello world");
    expectf  (va_arg(ap, float),  3.14);
    expecti  (va_arg(ap, int),    1024);
    expectstr(va_arg(ap, char *), "good bye world");
    expecti  (va_arg(ap, int),    2048);

    va_end(ap);
}

char *format(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap); // fuck yeah
    va_end(ap);
    return buffer;
}

void testt(void) {
    expectstr(format(""),        ""); // nothing
    expectstr(format("%d", 10),  "10");
    expectstr(
        format("%d,%.1f,%d,%.1f,%s", 1024, 3.14, 2048, 6.28, "hello world"),
        "1024,3.1,2048,6.3,hello world"
    );
}

int main(void) {
    testi(1, 2, 3, 4, 5);
    testf(1.0f, 2.0f, 4.0f, 8.0f);
    testm("hello world", 3.14, 1024, "good bye world", 2048);
    testt();

    return 0;
}
