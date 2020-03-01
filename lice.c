#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>

#include "lexer.h"
#include "parse.h"
#include "gen.h"
#include "opt.h"

bool compile_warning = true;

static void compile_diagnostic(const char *type, const char *fmt, va_list va) {
    fprintf(stderr, "%s %s: ", lexer_marker(), type);
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");
}

void compile_error(const char *fmt, ...) {
    va_list  a;
    va_start(a, fmt);
    compile_diagnostic("error", fmt, a);
    va_end(a);
    exit(EXIT_FAILURE);
}

void compile_warn(const char *fmt, ...) {
    if (!compile_warning)
        return;

    va_list a;
    va_start(a, fmt);
    compile_diagnostic("warning", fmt, a);
    va_end(a);
}

void compile_ice(const char *fmt, ...) {
    va_list a;
    va_start(a, fmt);
    compile_diagnostic("internal error", fmt, a);
    va_end(a);

    /* flush all streams */
    fflush(NULL);
    abort();
}

int compile_begin(bool dump) {
    parse_init();

    list_t *block = parse_run();
    size_t  index = 0;

    for (list_iterator_t *it = list_iterator(block); !list_iterator_end(it); index++) {
        printf("# block %zu\n", index);
        if (!dump) {
            gen_toplevel(list_iterator_next(it));
        } else {
            printf("%s", ast_string(list_iterator_next(it)));
        }
    }
    return true;
}

static bool parse_option(const char *optname, int *cargc, char ***cargv, char **out, int ds, bool split) {
    int    argc = *cargc;
    char **argv = *cargv;
    size_t len  = strlen(optname);

    if (strncmp(argv[0]+ds, optname, len))
        return false;

    /* it's --optname, check how the parameter is supplied */
    if (argv[0][ds+len] == '=') {
        *out = argv[0]+ds+len+1;
        return true;
    }

    if (!split || argc < ds) /* no parameter was provided, or only single-arg form accepted */
        return false;

    /* using --opt param */
    *out = argv[1];
    --*cargc;
    ++*cargv;

    return true;
}

int main(int argc, char **argv) {
    bool  dumpast  = false;
    char *standard = NULL;

    while (argc > 1) {
        ++argv;
        --argc;

        if (argv[0][0] == '-') {
            if (parse_option("std", &argc, &argv, &standard, 1, false))
                continue;
        }

        if (!strcmp(*argv, "--dump-ast")) {
            dumpast = true;
            continue;
        }

        fprintf(stderr, "unknown option: %s\n", argv[argc-1]);
        return EXIT_FAILURE;
    }

    if (standard) {
        if      (!strcmp(standard, "licec")) opt_std_set(STANDARD_LICEC);
        else if (!strcmp(standard, "gnuc"))  opt_std_set(STANDARD_GNUC);
        else if (!strcmp(standard, "c90"))   opt_std_set(STANDARD_C90);
        else if (!strcmp(standard, "c99"))   opt_std_set(STANDARD_C99);
        else if (!strcmp(standard, "c11"))   opt_std_set(STANDARD_C11);
        else if (!strcmp(standard, "kandr")) opt_std_set(STANDARD_KANDR);
        else {
            fprintf(stderr, "unknown standard: %s\n", standard);
            return EXIT_FAILURE;
        }
    }

    return compile_begin(dumpast);
}
