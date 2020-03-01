#include "util.h"
#include "list.h"

typedef struct list_node_s list_node_t;

struct list_node_s {
    void        *element;
    list_node_t *next;
    list_node_t *prev;
};

struct list_iterator_s {
    list_node_t *pointer;
};

list_t *list_create(void) {
    list_t *list = memory_allocate(sizeof(list_t));
    list->length = 0;
    list->head   = NULL;
    list->tail   = NULL;

    return list;
}

void *list_node_create(void *element) {
    list_node_t *node = memory_allocate(sizeof(list_node_t));
    node->element     = element;
    node->next        = NULL;
    node->prev        = NULL;
    return node;
}

void list_push(list_t *list, void *element) {
    list_node_t *node = list_node_create(element);
    if (!list->head)
        list->head = node;
    else {
        list->tail->next = node;
        node->prev       = list->tail;
    }
    list->tail = node;
    list->length++;
}

void *list_pop(list_t *list) {
    if (!list->head)
        return NULL;
    void *element = list->tail->element;
    list->tail = list->tail->prev;
    if (list->tail)
        list->tail->next = NULL;
    else
        list->head = NULL;
    list->length--;
    return element;
}

void *list_shift(list_t *list) {
    if (!list->head)
        return NULL;
    void *element = list->head->element;
    list->head = list->head->next;
    if (list->head)
        list->head->prev = NULL;
    else
        list->tail = NULL;
    list->length--;
    return element;
}

int list_length(list_t *list) {
    return list->length;
}

list_iterator_t *list_iterator(list_t *list) {
    list_iterator_t *iter = memory_allocate(sizeof(list_iterator_t));
    iter->pointer         = list->head;
    return iter;
}

void *list_iterator_next(list_iterator_t *iter) {
    void *ret;

    if (!iter->pointer)
        return NULL;

    ret           = iter->pointer->element;
    iter->pointer = iter->pointer->next;

    return ret;
}

bool list_iterator_end(list_iterator_t *iter) {
    return !iter->pointer;
}

static void list_shiftify(list_t *list, void *element) {
    list_node_t *node = list_node_create(element);
    node->next = list->head;
    if (list->head)
        list->head->prev = node;
    list->head = node;
    if (!list->tail)
        list->tail = node;
    list->length++;
}

list_t *list_reverse(list_t *list) {
    list_t *ret = list_create();
    for (list_iterator_t *it = list_iterator(list); !list_iterator_end(it); )
        list_shiftify(ret, list_iterator_next(it));
    return ret;
}

void *list_tail(list_t *list) {
    if (!list->head)
        return NULL;

    list_node_t *node = list->head;
    while (node->next)
        node = node->next;

    return node->element;
}

void *list_head(list_t *list) {
    return list->head->element;
}

void list_empty(list_t *list) {
    list->length = 0;
    list->head   = NULL;
    list->tail   = NULL;
}

list_t *list_default(void *item) {
    list_t *list = list_create();
    list_push(list, item);
    return list;
}
