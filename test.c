#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/utsname.h>

#include "util.h"

#define TEST_DIR "tests"
#define TEST_AS  "gcc -xassembler"
#define TEST_CPP "cpp -nostdinc -Iinclude/"
#define TEST_CC  "./lice -std=licec"

list_t *test_find(void) {
    list_t        *list = list_create();
    DIR           *dir;
    struct dirent *ent;

    if (!(dir = opendir(TEST_DIR)))
        return NULL;

    string_t *string;
    string_t *name;
    while ((ent = readdir(dir))) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
            continue;
        string = string_create();
        name   = string_create();

        string_catf(string, TEST_DIR "/%s", ent->d_name);

        // open file and read comment
        FILE   *fp   = fopen(string_buffer(string), "r");
        char   *line = NULL;
        size_t  len  = 0;
        getline(&line, &len, fp);
        fclose(fp);

        if (line[0] != '/' || line[1] != '/') {
            fprintf(stderr, "unnamed test: %s\n", string_buffer(string));
            fprintf(stderr, "please give it a name, aborting");
            exit(1);
        }

        // ignore whitespace
        char *skip = &line[2];
        while (isspace(*skip))
            skip++;

        // remove newline
        *strchr(skip, '\n')='\0';

        string_catf(name, skip);
        free(line);

        list_push(list, pair_create(string, name));
    }

    closedir(dir);
    return list;
}

int test_compile(string_t *file, bool ld) {
    string_t *command = string_create();
    FILE     *find;

    if (!(find = fopen("lice", "r"))) {
        fprintf(stderr, "lice not found");
        exit(1);
    }
    fclose(find);

    string_catf(command, "{ cat testrun.c; %s %s; } | %s | %s - ", TEST_CPP, string_buffer(file), TEST_CC, TEST_AS);
    if (ld)
        string_catf(command, " -ldl");
    string_catf(command, " && ./a.out");

    return system(string_buffer(command));
}

int main() {
    int     error = 0;
    list_t *list  = test_find();


    bool needld = false;
    struct utsname u;
    if (uname(&u) == -1) {
        fprintf(stderr, "uname call failed, aborting tests");
        return EXIT_FAILURE;
    }

    if (!strcmp(u.sysname, "Linux"))
        needld = true;


    if (!list) {
        fprintf(stderr, "%s/ %s\n", TEST_DIR, strerror(errno));
        return EXIT_FAILURE;
    }

    for (list_iterator_t *it = list_iterator(list); !list_iterator_end(it); ) {
        pair_t   *test = list_iterator_next(it);
        string_t *name = test->second;
        size_t    size = string_length(name);

        printf("testing %s ...", string_buffer(name));
        for (size_t i = 0; i < 40 - size; i++)
            printf(" ");

        if (test_compile(test->first, needld)) {
            printf("\033[31mERROR\033[0m\n");
            error++;
        } else {
            printf("\033[32mOK\033[0m\n");
        }
    }

    // print the command used for the tests
    printf("\nAll test were run with the following command:\n");
    if (!needld)
        printf("{ cat testrun.c; %s $SRC; } | %s | %s - && ./a.out\n", TEST_CPP, TEST_CC, TEST_AS);
    else
        printf("{ cat testrun.c; %s $SRC; } | %s | %s - -ldl && ./a.out\n", TEST_CPP, TEST_CC, TEST_AS);

    return (error) ? EXIT_FAILURE
                   : EXIT_SUCCESS;
}
