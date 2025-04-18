#ifndef STACK_H
#define STACK_H

#include <stdlib.h>
#include <stdatomic.h>

#endif

#ifndef STACK_NAME
#error "STACK_NAME must be defined"
#endif

#ifndef STACK_TYPE
#error "STACK_TYPE must be defined"
#endif

#define STACK_CONCAT_(a, b) a ## b
#define STACK_CONCAT(a, b) STACK_CONCAT_(a, b)
#define STACK_FUNC(name) STACK_CONCAT(STACK_NAME, _##name)
#define STACK_TYPED(name) STACK_CONCAT(STACK_NAME, _##name)

#define STACK_NODE STACK_TYPED(node)
#define STACK_HEAD STACK_TYPED(head)

typedef struct STACK_NODE {
    struct STACK_NODE *next;
    STACK_TYPE value;
} STACK_NODE;

#ifdef STACK_THREAD_SAFE
// Need double-wide compare-and-swap (DWCAS) for atomic stack
typedef struct STACK_HEAD {
    size_t version;
    STACK_NODE *node;
} STACK_HEAD;
#endif

#define STACK_ITEM_MEMORY_POOL_NAME STACK_TYPED(node_memory_pool)

#define MEMORY_POOL_NAME STACK_ITEM_MEMORY_POOL_NAME
#define MEMORY_POOL_TYPE STACK_NODE
#ifdef STACK_THREAD_SAFE
#define MEMORY_POOL_THREAD_SAFE
#endif
#include "memory_pool/memory_pool.h"
#undef MEMORY_POOL_NAME
#undef MEMORY_POOL_TYPE
#ifdef STACK_THREAD_SAFE
#undef MEMORY_POOL_THREAD_SAFE
#endif

#define STACK_ITEM_MEMORY_POOL_FUNC(name) STACK_CONCAT(STACK_ITEM_MEMORY_POOL_NAME, _##name)

typedef struct {
    #ifdef STACK_THREAD_SAFE
    _Atomic STACK_HEAD head;
    atomic_size_t size;
    #else
    STACK_NODE *head;
    size_t size;
    #endif
    STACK_ITEM_MEMORY_POOL_NAME *pool;
} STACK_NAME;


static STACK_NAME *STACK_FUNC(new_pool)(STACK_ITEM_MEMORY_POOL_NAME *pool) {
    STACK_NAME *list = malloc(sizeof(STACK_NAME));
    if (list == NULL) return NULL;
    list->pool = pool;
    #ifdef STACK_THREAD_SAFE
    STACK_HEAD head = (STACK_HEAD){0, (STACK_NODE *)NULL};
    atomic_init(&list->head, head);
    atomic_init(&list->size, 0);
    #else
    list->head = NULL;
    list->size = 0;
    #endif
    return list;
}

static STACK_NAME *STACK_FUNC(new)(void) {
    STACK_ITEM_MEMORY_POOL_NAME *pool = STACK_ITEM_MEMORY_POOL_FUNC(new)();
    if (pool == NULL) {
        return NULL;
    }
    STACK_NAME *list = STACK_FUNC(new_pool)(pool);
    if (list == NULL) {
        STACK_ITEM_MEMORY_POOL_FUNC(destroy)(pool);
        return NULL;
    }
    return list;
}

bool STACK_FUNC(push)(STACK_NAME *list, STACK_TYPE value) {
    STACK_NODE *node = STACK_ITEM_MEMORY_POOL_FUNC(get)(list->pool);
    if (node == NULL) return false;
    node->value = value;
    #ifdef STACK_THREAD_SAFE
    STACK_HEAD old_head, new_head;
    do {
        old_head = atomic_load(&list->head);
        node->next = old_head.node;
        new_head.version = old_head.version + 1;
        new_head.node = node;
    } while (!atomic_compare_exchange_weak(&list->head, &old_head, new_head));
    atomic_fetch_add(&list->size, 1);
    #else
    node->next = list->head;
    list->head = node;
    list->size++;
    #endif
    return true;
}

bool STACK_FUNC(pop)(STACK_NAME *list, STACK_TYPE *result) {
    if (list == NULL || result == NULL) return false;
    #ifdef STACK_THREAD_SAFE
    STACK_HEAD old_head, new_head;
    do {
        old_head = atomic_load(&list->head);
        if (old_head.node == NULL) return false;
        // Only need the incremented version on one side, so we use push
        new_head.version = old_head.version;
        new_head.node = old_head.node->next;
    } while (!atomic_compare_exchange_weak(&list->head, &old_head, new_head));
    STACK_NODE *node = old_head.node;
    #else
    if (list->head == NULL) return false;
    STACK_NODE *node = list->head;
    list->head = node->next;
    #endif
    *result = node->value;
    STACK_ITEM_MEMORY_POOL_FUNC(release)(list->pool, node);
    #ifdef STACK_THREAD_SAFE
    atomic_fetch_sub(&list->size, 1);
    #else
    list->size--;
    #endif
    return true;
}

size_t STACK_FUNC(size)(STACK_NAME *list) {
    if (list == NULL) return 0;
    #ifdef STACK_THREAD_SAFE
    return atomic_load(&list->size);
    #else
    return list->size;
    #endif
}

void STACK_FUNC(destroy)(STACK_NAME *list) {
    if (list == NULL) return;
    if (list->pool != NULL) {
        STACK_ITEM_MEMORY_POOL_FUNC(destroy)(list->pool);
    }
    free(list);
}