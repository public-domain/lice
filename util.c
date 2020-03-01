#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "util.h"

#define MEMORY 0x8000000

static unsigned char *memory_pool = NULL;
static size_t         memory_next = 0;

static void memory_cleanup(void) {
    free(memory_pool);
}

void *memory_allocate(size_t bytes) {
    void *value;

    if (!memory_pool) {
        memory_pool = malloc(MEMORY);
        atexit(memory_cleanup);
    }

    if (memory_next > MEMORY)
        goto bail;

    value = &memory_pool[memory_next];
    memory_next += bytes;

    if (memory_next > MEMORY) {
bail:
        fprintf(stderr, "[lice] out of memory");
        exit(EXIT_FAILURE);
    }

    return value;
}

struct string_s {
    char *buffer;
    int   allocated;
    int   length;
};

static void string_reallocate(string_t *string) {
    int   size   = string->allocated * 2;
    char *buffer = memory_allocate(size);

    strcpy(buffer, string->buffer);
    string->buffer    = buffer;
    string->allocated = size;
}

void string_catf(string_t *string, const char *fmt, ...) {
    va_list  va;
    for (;;) {
        int left  = string->allocated - string->length;
        int write;

        va_start(va, fmt);
        write = vsnprintf(string->buffer + string->length, left, fmt, va);
        va_end(va);

        if (left <= write) {
            string_reallocate(string);
            continue;
        }
        string->length += write;
        return;
    }
}

string_t *string_create(void) {
    string_t *string  = memory_allocate(sizeof(string_t));
    string->buffer    = memory_allocate(8);
    string->allocated = 8;
    string->length    = 0;
    string->buffer[0] = '\0';
    return string;
}

char *string_buffer(string_t *string) {
    return string->buffer;
}

void string_cat(string_t *string, char ch) {
    if (string->allocated == (string->length + 1))
        string_reallocate(string);
    string->buffer[string->length++] = ch;
    string->buffer[string->length]   = '\0';
}

char *string_quote(char *p) {
    string_t *string = string_create();
    while (*p) {
        if (*p == '\"' || *p == '\\')
            string_catf(string, "\\%c", *p);
        else if (*p == '\n')
            string_catf(string, "\\n");
        else
            string_cat(string, *p);
        p++;
    }
    return string->buffer;
}

size_t string_length(string_t *string) {
    return (string) ? string->length : 0;
}

typedef struct {
    char *key;
    void *value;
} table_entry_t;

void *table_create(void *parent) {
    table_t *table = memory_allocate(sizeof(table_t));
    table->list    = list_create();
    table->parent  = parent;

    return table;
}

void *table_find(table_t *table, const char *key) {
    for (; table; table = table->parent) {
        for (list_iterator_t *it = list_iterator(table->list); !list_iterator_end(it); ) {
            table_entry_t *entry = list_iterator_next(it);
            if (!strcmp(key, entry->key))
                return entry->value;
        }
    }
    return NULL;
}

void table_insert(table_t *table, char *key, void *value) {
    table_entry_t *entry = memory_allocate(sizeof(table_entry_t));
    entry->key           = key;
    entry->value         = value;

    list_push(table->list, entry);
}

void *table_parent(table_t *table) {
    return table->parent;
}

list_t *table_values(table_t *table) {
    list_t *list = list_create();
    for (; table; table = table->parent)
        for (list_iterator_t *it = list_iterator(table->list); !list_iterator_end(it); )
            list_push(list, ((table_entry_t*)list_iterator_next(it))->value);
    return list;
}

list_t *table_keys(table_t *table) {
    list_t *list = list_create();
    for (; table; table = table->parent)
        for (list_iterator_t *it = list_iterator(table->list); !list_iterator_end(it); )
            list_push(list, ((table_entry_t*)list_iterator_next(it))->key);
    return list;
}

pair_t *pair_create(void *first, void *second) {
    pair_t *pair = memory_allocate(sizeof(pair_t));
    pair->first  = first;
    pair->second = second;
    return pair;
}

int strcasecmp(const char *s1, const char *s2) {
    const unsigned char *u1 = (const unsigned char *)s1;
    const unsigned char *u2 = (const unsigned char *)s2;

    while (tolower(*u1) == tolower(*u2++))
        if(*u1++ == '\0')
            return 0;
    return tolower(*u1) - tolower(*--u2);
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
    const unsigned char *u1 = (const unsigned char *)s1;
    const unsigned char *u2 = (const unsigned char *)s2;

    if (!n)
        return 0;

    do {
        if (tolower(*u1) != tolower(*u2++))
            return tolower(*u1) - tolower(*--u2);
        if (*u1++ == '\0')
            break;
    } while (--n != 0);

    return 0;
}

size_t getline(char **line, size_t *n, FILE *stream) {
    char *buf = NULL;
    char *p   = buf;

    int   size;
    int   c;

    if (!line)
        return -1;
    if (!stream)
        return -1;
    if (!n)
        return -1;

    buf  = *line;
    size = *n;

    if ((c = fgetc(stream)) == EOF)
        return -1;
    if (!buf) {
        buf  = malloc(128);
        size = 128;
    }
    p = buf;
    while(c != EOF) {
        if ((p - buf) > (size - 1)) {
            size = size + 128;
            buf  = realloc(buf, size);
        }
        *p++ = c;
        if (c == '\n')
            break;
        c = fgetc(stream);
    }

    *p++  = '\0';
    *line = buf;
    *n    = size;

    return p - buf - 1;
}
