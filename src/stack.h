#ifndef STACK_H
#define STACK_H

#include <stdlib.h>

#endif

#ifndef STACK_NAME
#error "STACK_NAME must be defined"
#endif

#ifndef STACK_TYPE
#error "STACK_TYPE must be defined"
#endif

#ifndef STACK_MALLOC
#define STACK_MALLOC(size) malloc(size)
#define STACK_MALLOC_DEFINED
#endif

#ifndef STACK_FREE
#define STACK_FREE(ptr) free(ptr)
#define STACK_FREE_DEFINED
#endif

#define STACK_CONCAT_(a, b) a ## b
#define STACK_CONCAT(a, b) STACK_CONCAT_(a, b)
#define STACK_FUNC(name) STACK_CONCAT(STACK_NAME, _##name)
#define STACK_TYPED(name) STACK_CONCAT(STACK_NAME, _##name)

#define STACK_NODE STACK_TYPED(node)

typedef struct STACK_NODE {
    struct STACK_NODE *next;
    STACK_TYPE value;
} STACK_NODE;

#define STACK_ITEM_MEMORY_POOL_NAME STACK_TYPED(node_memory_pool)

#define MEMORY_POOL_NAME STACK_ITEM_MEMORY_POOL_NAME
#define MEMORY_POOL_TYPE STACK_NODE
#include "memory_pool/memory_pool.h"
#undef MEMORY_POOL_NAME
#undef MEMORY_POOL_TYPE

#define STACK_ITEM_MEMORY_POOL_FUNC(name) STACK_CONCAT(STACK_ITEM_MEMORY_POOL_NAME, _##name)

typedef struct {
    STACK_NODE *head;
    size_t size;
    bool own_pool;
    STACK_ITEM_MEMORY_POOL_NAME *pool;
} STACK_NAME;


static bool STACK_FUNC(init_pool)(STACK_NAME *list, STACK_ITEM_MEMORY_POOL_NAME *pool) {
    if (list == NULL || pool == NULL) return false;
    list->pool = pool;
    list->own_pool = false;
    list->head = NULL;
    list->size = 0;
    return true;
}

static bool STACK_FUNC(init)(STACK_NAME *list) {
    if (list == NULL) return false;
    STACK_ITEM_MEMORY_POOL_NAME *pool = STACK_ITEM_MEMORY_POOL_FUNC(new)();
    if (pool == NULL) return false;
    if (!STACK_FUNC(init_pool)(list, pool)) {
        STACK_ITEM_MEMORY_POOL_FUNC(destroy)(pool);
        return false;
    }
    list->own_pool = true;
    return true;
}

static STACK_NAME *STACK_FUNC(new_pool)(STACK_ITEM_MEMORY_POOL_NAME *pool) {
    STACK_NAME *list = STACK_MALLOC(sizeof(STACK_NAME));
    if (list == NULL) return NULL;
    if (!STACK_FUNC(init_pool)(list, NULL)) {
        STACK_FREE(list);
        return NULL;
    }
    return list;
}

static STACK_NAME *STACK_FUNC(new)(void) {
    STACK_NAME *list = STACK_MALLOC(sizeof(STACK_NAME));
    if (list == NULL) return NULL;
    if (!STACK_FUNC(init)(list)) {
        STACK_FREE(list);
        return NULL;
    }
    return list;
}

bool STACK_FUNC(push)(STACK_NAME *list, STACK_TYPE value) {
    STACK_NODE *node = STACK_ITEM_MEMORY_POOL_FUNC(get)(list->pool);
    if (node == NULL) return false;
    node->value = value;
    node->next = list->head;
    list->head = node;
    list->size++;
    return true;
}

bool STACK_FUNC(pop)(STACK_NAME *list, STACK_TYPE *result) {
    if (list == NULL || result == NULL) return false;
    if (list->head == NULL) return false;
    STACK_NODE *node = list->head;
    list->head = node->next;
    *result = node->value;
    STACK_ITEM_MEMORY_POOL_FUNC(release)(list->pool, node);
    list->size--;
    return true;
}

bool STACK_FUNC(release_node)(STACK_NAME *list, STACK_NODE *node) {
    if (list == NULL || node == NULL) return false;
    STACK_ITEM_MEMORY_POOL_FUNC(release)(list->pool, node);
    return true;
}

STACK_NODE *STACK_FUNC(pop_all)(STACK_NAME *list) {
    if (list == NULL) return NULL;
    STACK_NODE *result = NULL;
    result = list->head;
    list->head = NULL;
    list->size = 0;
    return result;
}

bool STACK_FUNC(peek)(STACK_NAME *list, STACK_TYPE *result) {
    if (list == NULL) return false;
    if (list->head == NULL) {
        return false;
    } else {
        *result = list->head->value;
    }
    return true;
}

size_t STACK_FUNC(size)(STACK_NAME *list) {
    if (list == NULL) return 0;
    return list->size;
}

void STACK_FUNC(destroy)(STACK_NAME *list) {
    if (list == NULL) return;
    if (list->pool != NULL && list->own_pool) {
        STACK_ITEM_MEMORY_POOL_FUNC(destroy)(list->pool);
    }
    STACK_FREE(list);
}

#ifdef STACK_MALLOC_DEFINED
#undef STACK_MALLOC_DEFINED
#undef STACK_MALLOC
#endif

#ifdef STACK_FREE_DEFINED
#undef STACK_FREE_DEFINED
#undef STACK_FREE
#endif


#undef STACK_CONCAT_
#undef STACK_CONCAT
#undef STACK_FUNC
#undef STACK_TYPED

#undef STACK_NODE

#undef STACK_ITEM_MEMORY_POOL_NAME