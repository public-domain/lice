#ifndef LICE_UTIL_LIST_HDR
#define LICE_UTIL_LIST_HDR
#include <stdbool.h>

/*
 * Macro: SENTINEL_LIST
 *  Initialize an empty list in place
 */
#define SENTINEL_LIST ((list_t) { \
        .length    = 0,           \
        .head      = NULL,        \
        .tail      = NULL         \
})

/*
 * Type: list_iterator_t
 *  A type capable of representing an itrator for a <list>
 */
typedef struct list_iterator_s list_iterator_t;

/*
 * Type: list_t
 *  A standard double-linked list
 */
typedef struct list_s list_t;

/*
 * Function: list_create
 *  Creates a new list
 */
list_t *list_create(void);

/*
 * Function: list_push
 *  Push an element onto a list
 */
void list_push(list_t *list, void *element);

/*
 * Function: list_pop
 *  Pop an element from a list
 */
void *list_pop(list_t *list);

/*
 * Function: list_length
 *  Used to retrieve length of a list object
 */
int list_length(list_t *list);

/*
 * Function: list_shift
 *  Like a list_pop but shift from the head (instead of the tail)
 */
void *list_shift(list_t *list);

/*
 * Function: list_reverse
 *  Reverse the contents of a list
 */
list_t *list_reverse(list_t *list);

/*
 * Function: list_iterator
 *  Create an iterator for a given list object
 */
list_iterator_t *list_iterator(list_t *list);

/*
 * Function: list_iterator_next
 *  Increment the list iterator while returning the given element
 */
void *list_iterator_next(list_iterator_t *iter);

/*
 * Function: list_iterator_end
 *  Test if the iterator is at the end of the list
 */
bool list_iterator_end(list_iterator_t *iter);

/*
 * Function: list_tail
 *  Get the last element in a list
 *
 * Parameters:
 *  list - The list in question to get the last element of.
 *
 * Returns:
 *  The last element in the list.
 */
void *list_tail(list_t *list);

/*
 * Function: list_head
 *  Get the first element in a list
 *
 * Parameters:
 *  list - The list in question to get the first element of.
 *
 * Returns:
 *  The first element in the list.
 */
void *list_head(list_t *list);

/*
 * Function: list_empty
 *  Empty the contents of a list
 *
 * Parameters:
 *  list - The list to empty the contents of.
 */
void list_empty(list_t *list);

/*
 * Function: list_default
 *  Create a new list default initializing the first element with
 *  an item.
 *
 * Parameters:
 *  item - The first item to default initialize the list with.
 *
 * Returns:
 *  A new list containing one element holding `item`.
 */
list_t *list_default(void *item);

struct list_s {
    int                 length;
    struct list_node_s *head;
    struct list_node_s *tail;
};

#endif
