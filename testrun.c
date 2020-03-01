
// some decls to suppress some warnings the compiler will emit
void exit(int);
int printf(const char *__format, ...);
int strcmp(const char *__s1, const char *__s2);
int vsprintf(char *__src, const char *__format, void *);
void *dlsym(void *__handle, const char *__symbol);
void *dlopen(const char *__library, long __flags);
int snprintf(char *__s, unsigned long __maxlen, const char *__format);

int external_1 = 1337;
int external_2 = 7331;

void expecti(int a, int b) {
    if (a != b) {
        printf("    Expected: %d\n", b);
        printf("    Result:   %d\n", a);
        exit(1);
    }
}

void expectl(long a, long b) {
    if (a != b) {
        printf("    Expected: %d\n", b);
        printf("    Result:   %d\n", a);
        exit(1);
    }
}

void expectf(float a, float b) {
    if (a != b) {
        printf("    Expected: %f\n", b);
        printf("    Result:   %f\n", a);
        exit(1);
    }
}

void expectd(double a, double b) {
    if (a != b) {
        printf("    Expected: %f\n", b);
        printf("    Result:   %f\n", a);
        exit(1);
    }
}

void expects(short a, short b) {
    if (a != b) {
        printf("    Expected: %s\n", b);
        printf("    Result:   %s\n", a);
        exit(1);
    }
}

void expectstr(const char *a, const char *b) {
    if (strcmp(a, b)) {
        printf("    Expected: %s\n", b);
        printf("    Result:   %s\n", a);
        exit(1);
    }
}
