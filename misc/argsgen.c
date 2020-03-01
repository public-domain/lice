#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../util.h"
#define EMAX       128 // maximum arguments
#define SPLIT_CALL 8   // argument count to split by newline on function calls
#define SPLIT_ARG  4   // argument count to split by newline in function decls

int main() {
    FILE *fp = fopen("tests/args.c", "w");
    fprintf(fp, "// function arguments\n\n");

    const char *types[] = {
        "int", "short", "long",
        "$"
    };

    table_t *table = table_create(NULL); // table for last thing

    for (int e = 0; e < sizeof(types)/sizeof(types[0]); e++) {
        if (types[e][0] != '$')
            fprintf(fp, "void test_%s(", types[e]);
        else
            fprintf(fp, "void test_mix(");

        for (int i = 1; i < EMAX; i++) {
            if (types[e][0] != '$')
                fprintf(fp, "%-6s value%-3d", types[e], i);
            else {
                string_t *string = string_create();
                const char *type = types[rand() % (sizeof(types)/sizeof(types[0]) - 1)];
                fprintf(fp, "%-6s value%-3d", type, i);
                string_catf(string, "%d", i);
                table_insert(table, string_buffer(string), (void*)type);
            }
            if (i != EMAX-1)
                fprintf(fp, ", ");
            if (i % SPLIT_ARG == 0) {
                fprintf(fp, "\n");
                if (types[e][0] != '$') {
                    fprintf(fp, "           "); // void test_ whitespace
                    for (int j = 0; j < strlen(types[e]); j++)
                        fprintf(fp, " ");
                } else {
                    fprintf(fp, "              ");
                }
            }
        }

        fprintf(fp, ") {\n");
        for (int i = 1; i < EMAX; i++) {
            char t = types[e][0];
            if (t == '$') {
                string_t *key = string_create();
                string_catf(key, "%d", i);
                const char *type = table_find(table, string_buffer(key));
                t = type[0];
            }
            if (types[e][0] == 'd')
                fprintf(fp, "    expect%c(value%d, 0.%d);\n", t, i, i);
            else if (types[e][0] == 'f')
                fprintf(fp, "    expect%c(value%d, 0.%d);\n", t, i, i);
            else
                fprintf(fp, "    expect%c(value%d, %d);\n", t, i, i);
        }
        fprintf(fp, "}\n");
    }

    fprintf(fp, "int main() {\n");
    for (int i = 0; i < sizeof(types)/sizeof(types[0]) - 1; i++) {
        fprintf(fp, "    test_%s(", types[i]);
        for (int j = 1; j < EMAX; j++) {
            if (types[i][0] == 'f') {
                string_t *len = string_create();
                string_catf(len, "0.%df", j);
                fprintf(fp, string_buffer(len), j);
                for (int p = strlen(string_buffer(len)); p <= 5; p++)
                    fprintf(fp, " ");
            } else if (types[i][0] == 'd')
                fprintf(fp, "0.%-3d", j);
            else
                fprintf(fp, "%-3d", j);

            if (i != EMAX-1)
                fprintf(fp, ", ");

            if (j % SPLIT_CALL == 0) {
                fprintf(fp, "\n    ");
                fprintf(fp, "      "); // test_
                for (int k = 0; k < strlen(types[i]); k++)
                    fprintf(fp, " ");
            }
        }
        fprintf(fp, "\n    );\n");
    }
    // mix is special since it needs to align everything
    // at 0.ffff <-- six things
    fprintf(fp, "    test_mix(");
    for (int j = 1; j < EMAX; j++) {
        string_t *key = string_create();
        string_catf(key, "%d", j);
        const char *type = table_find(table, string_buffer(key));

        if (type[0] == 'f') {
            string_t *len = string_create();
            string_catf(len, "0.%df", j);
            fprintf(fp, string_buffer(len), j);
            for (int p = strlen(string_buffer(len)); p <= 5; p++)
                fprintf(fp, " ");
        } else if (type[0] == 'd')
            fprintf(fp, "0.%-4d", j);
        else
            fprintf(fp, "%-6d", j);

        if (j != EMAX-1)
            fprintf(fp, ", ");

        if (j % SPLIT_CALL == 0) {
            fprintf(fp, "\n    ");
            fprintf(fp, "         "); // test_mix
        }
    }
    fprintf(fp, "\n    );\n");
    fprintf(fp, "    return 0;\n}\n");
    fclose(fp);
}
