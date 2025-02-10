#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdlib.h>
#include <stdatomic.h>

#endif

#ifndef LINKED_LIST_NAME
#error "LINKED_LIST_NAME must be defined"
#endif

#ifndef LINKED_LIST_TYPE
#error "LINKED_LIST_TYPE must be defined"
#endif

#define LINKED_LIST_CONCAT_(a, b) a ## b
#define LINKED_LIST_CONCAT(a, b) LINKED_LIST_CONCAT_(a, b)
#define LINKED_LIST_FUNC(name) LINKED_LIST_CONCAT(LINKED_LIST_NAME, _##name)
#define LINKED_LIST_TYPED(name) LINKED_LIST_CONCAT(LINKED_LIST_NAME, _##name)

#define LINKED_LIST_NODE LINKED_LIST_TYPED(node)
#define LINKED_LIST_HEAD LINKED_LIST_TYPED(head)

typedef struct LINKED_LIST_NODE {
    struct LINKED_LIST_NODE *next;
    LINKED_LIST_TYPE value;
} LINKED_LIST_NODE;

// Need double-wide compare-and-swap (DWCAS) for atomic linked list
typedef struct LINKED_LIST_HEAD {
    size_t version;
    LINKED_LIST_NODE *node;
} LINKED_LIST_HEAD;

#define LINKED_LIST_ITEM_MEMORY_POOL_NAME LINKED_LIST_TYPED(node_memory_pool)

#define MEMORY_POOL_NAME LINKED_LIST_ITEM_MEMORY_POOL_NAME
#define MEMORY_POOL_TYPE LINKED_LIST_NODE
#ifdef LINKED_LIST_THREAD_SAFE
#define MEMORY_POOL_THREAD_SAFE
#endif
#include "memory_pool/memory_pool.h"
#undef MEMORY_POOL_NAME
#undef MEMORY_POOL_TYPE
#ifdef LINKED_LIST_THREAD_SAFE
#undef MEMORY_POOL_THREAD_SAFE
#endif

#define LINKED_LIST_ITEM_MEMORY_POOL_FUNC(name) LINKED_LIST_CONCAT(LINKED_LIST_ITEM_MEMORY_POOL_NAME, _##name)

typedef struct {
    #ifdef LINKED_LIST_THREAD_SAFE
    _Atomic LINKED_LIST_HEAD head;
    atomic_size_t size;
    #else
    LINKED_LIST_HEAD head;
    size_t size;
    #endif
    LINKED_LIST_ITEM_MEMORY_POOL_NAME *pool;
} LINKED_LIST_NAME;


static LINKED_LIST_NAME *LINKED_LIST_FUNC(new_pool)(LINKED_LIST_ITEM_MEMORY_POOL_NAME *pool) {
    LINKED_LIST_NAME *list = malloc(sizeof(LINKED_LIST_NAME));
    if (list == NULL) return NULL;
    list->pool = pool;
    LINKED_LIST_HEAD head = (LINKED_LIST_HEAD){0, (LINKED_LIST_NODE *)NULL};
    #ifdef LINKED_LIST_THREAD_SAFE
    atomic_init(&list->head, head);
    atomic_init(&list->size, 0);
    #else
    list->head = head;
    list->size = 0;
    #endif
    return list;
}

static LINKED_LIST_NAME *LINKED_LIST_FUNC(new)(void) {
    LINKED_LIST_ITEM_MEMORY_POOL_NAME *pool = LINKED_LIST_ITEM_MEMORY_POOL_FUNC(new)();
    if (pool == NULL) {
        return NULL;
    }
    LINKED_LIST_NAME *list = LINKED_LIST_FUNC(new_pool)(pool);
    if (list == NULL) {
        LINKED_LIST_ITEM_MEMORY_POOL_FUNC(destroy)(pool);
        return NULL;
    }
    LINKED_LIST_HEAD head = (LINKED_LIST_HEAD){0, (LINKED_LIST_NODE *)NULL};
    #ifdef LINKED_LIST_THREAD_SAFE
    atomic_init(&list->head, head);
    atomic_init(&list->size, 0);
    #else
    list->head = head;
    list->size = 0;
    #endif
    return list;
}

bool LINKED_LIST_FUNC(push)(LINKED_LIST_NAME *list, LINKED_LIST_TYPE value) {
    LINKED_LIST_NODE *node = LINKED_LIST_ITEM_MEMORY_POOL_FUNC(get)(list->pool);
    if (node == NULL) return false;
    node->value = value;
    #ifdef LINKED_LIST_THREAD_SAFE
    LINKED_LIST_HEAD old_head, new_head;
    do {
        old_head = atomic_load(&list->head);
        node->next = old_head.node;
        new_head.version = old_head.version + 1;
        new_head.node = node;
    } while (!atomic_compare_exchange_weak(&list->head, &old_head, new_head));
    atomic_fetch_add(&list->size, 1);
    #else
    node->next = list->head.node;
    list->head.node = node;
    list->size++;
    #endif
    return true;
}

bool LINKED_LIST_FUNC(pop)(LINKED_LIST_NAME *list, LINKED_LIST_TYPE *result) {
    if (list == NULL || result == NULL) return false;
    #ifdef LINKED_LIST_THREAD_SAFE
    LINKED_LIST_HEAD old_head, new_head;
    do {
        old_head = atomic_load(&list->head);
        if (old_head.node == NULL) return false;
        // Only need the incremented version on one side, so we use push
        new_head.version = old_head.version;
        new_head.node = old_head.node->next;
    } while (!atomic_compare_exchange_weak(&list->head, &old_head, new_head));
    #else
    if (list->head.node == NULL) return false;
    LINKED_LIST_HEAD old_head = list->head;
    list->head.node = old_head.node->next;
    #endif
    LINKED_LIST_NODE *node = old_head.node;
    *result = node->value;
    LINKED_LIST_ITEM_MEMORY_POOL_FUNC(release)(list->pool, node);
    #ifdef LINKED_LIST_THREAD_SAFE
    atomic_fetch_sub(&list->size, 1);
    #else
    list->size--;
    #endif
    return true;
}

size_t LINKED_LIST_FUNC(size)(LINKED_LIST_NAME *list) {
    if (list == NULL) return 0;
    #ifdef LINKED_LIST_THREAD_SAFE
    return atomic_load(&list->size);
    #else
    return list->size;
    #endif
}

void LINKED_LIST_FUNC(destroy)(LINKED_LIST_NAME *list) {
    if (list == NULL) return;
    if (list->pool != NULL) {
        LINKED_LIST_ITEM_MEMORY_POOL_FUNC(destroy)(list->pool);
    }
    free(list);
}