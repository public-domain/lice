// dynamic loading
#include <stddef.h>

// should be good for the next ten thousand years :P
#define SOMAX 128

int main(void) {
    int so = 0;
    char buffer[32];
    void *handle = dlopen("libc.so", 1);

    if (!handle) {
        for (so = 0; so < SOMAX; so++) {
            snprintf(buffer, sizeof(buffer), "libc.so.%d", so);
            if ((handle = dlopen(buffer, 1)))
                break;
        }
    }

    if (!handle) {
        printf("failed to load libc\n");
        return 1;
    }

    // test must return 0 to succeed
    int (*a)(int) = dlsym(handle, "exit");
    a(0);

    return 1;
}
