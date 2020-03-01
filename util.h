#ifndef GMCC_UTIL_HDR
#define GMCC_UTIL_HDR
#include <stdbool.h>
#include <stdio.h>

#include "list.h"

/*
 * Type: string_t
 *  A type capable of representing a self-resizing string with
 *  O(1) strlen.
 */
typedef struct string_s string_t;

/*
 * Function: string_create
 *  Create a string object
 */
string_t *string_create(void);

/*
 * Function: string_buffer
 *  Return the raw buffer of a string object
 */
char *string_buffer(string_t *string);

/*
 * Function: string_cat
 *  Append a character to a string object
 */
void string_cat(string_t *string, char ch);

/*
 * Function: string_catf
 *  Append a formatted string to a string object
 */
void string_catf(string_t *string, const char *fmt, ...);

/*
 * Function: string_quote
 *  Escape a string's quotes
 */
char *string_quote(char *p);

/*
 * Function: string_length
 *  Get the length of the given string
 */
size_t string_length(string_t *string);

/*
 * Type: table_t
 *  A key value associative table
 */
typedef struct table_s table_t;

struct table_s {
    list_t  *list;
    table_t *parent;
};

/*
 * Function: table_create
 *  Creates a table_t object
 */
void *table_create(void *parent);

/*
 * Funciton: table_find
 *  Searches for a given value in the table based on the
 *  key associated with it.
 */
void *table_find(table_t *table, const char *key);

/*
 * Function: table_insert
 *  Inserts a value for the given key as an entry in the
 *  table.
 */
void  table_insert(table_t *table, char *key, void *value);

/*
 * Function: table_parent
 *  Returns the parent opaque object for the given table to
 *  be used as the argument to a new table.
 */
void *table_parent(table_t *table);

/*
 * Function: table_values
 *  Generates a list of all the values in the table, useful for
 *  iterating over the values.
 */
list_t *table_values(table_t *table);

/*
 * Function: table_keys
 *  Generate a list of all the keys in the table, useful for
 *  iteration over the keys.
 */
list_t *table_keys(table_t *table);

/*
 * Macro: SENTINEL_TABLE
 *  Initialize an empty table in place
 */
#define SENTINEL_TABLE ((table_t) { \
    .list   = &SENTINEL_LIST,       \
    .parent = NULL                  \
})


#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define MAX(A, B) (((A) > (B)) ? (A) : (B))

/*
 * Function: memory_allocate
 * Allocate some memory
 */
void *memory_allocate(size_t bytes);

/*
 * Structure: pair_t
 *  A class container describing a pair
 */
typedef struct {
    /* Variable: first */
    void *first;
    /* Variable: second */
    void *second;
} pair_t;

/*
 * Function: pair_create
 *  Used to create a <pair_t>.
 *
 * Parameters:
 *  first  - Pointer to first object
 *  second - Pointer to second object
 *
 * Returns:
 *  A pointer to a constructed <pair_t> containing first and last
 *  pointers which point to the same address as the ones supplied
 *  as parameters to this function.
 */
pair_t *pair_create(void *first, void *last);

/* String utilities */
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
size_t getline(char **line, size_t *n, FILE *stream);

/*
 * Macro: bool_stringa
 *  Returns a "true" or "false" for an expression that evaluates to a
 *  boolean representation enforced with cast-to-bool `!!`
 */
#define bool_string(BOOL) \
    ((!!(BOOL)) ? "true" : "false")

#endif
