#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "threading/threading.h"
#include "greatest/greatest.h"

#define LINKED_LIST_NAME linked_list_uint32_single_threaded
#define LINKED_LIST_TYPE uint32_t
#include "linked_list.h"
#undef LINKED_LIST_NAME
#undef LINKED_LIST_TYPE

#define LINKED_LIST_NAME linked_list_uint32
#define LINKED_LIST_TYPE uint32_t
#define LINKED_LIST_THREAD_SAFE
#include "linked_list.h"
#undef LINKED_LIST_NAME
#undef LINKED_LIST_TYPE
#undef LINKED_LIST_THREAD_SAFE


typedef struct {
    uint64_t i;
    double x[3];
} id_point_t;

#define NUM_THREADS 8
#define NUM_PUSHES 50000

int linked_list_thread(void *arg) {
    linked_list_uint32 *list = (linked_list_uint32 *)arg;
    for (size_t i = 0; i < NUM_PUSHES; i++) {
        linked_list_uint32_push(list, i);
        if (i % 4 == 0) {
            uint32_t val;
            if (!linked_list_uint32_pop(list, &val)) {
                return 1;
            }
        }
    }
    return 0;
}


TEST test_linked_list(void) {
    linked_list_uint32_single_threaded *list = linked_list_uint32_single_threaded_new();
    for (uint32_t i = 0; i < NUM_PUSHES; i++) {
        ASSERT(linked_list_uint32_single_threaded_push(list, i));
    }
    size_t size = linked_list_uint32_single_threaded_size(list);
    ASSERT_EQ(size, NUM_PUSHES);
    for (uint32_t i = 0; i < NUM_PUSHES; i++) {
        uint32_t val;
        ASSERT(linked_list_uint32_single_threaded_pop(list, &val));
        ASSERT_EQ(val, NUM_PUSHES - 1 - i);
    }
    size = linked_list_uint32_single_threaded_size(list);
    ASSERT_EQ(size, 0);
    linked_list_uint32_single_threaded_destroy(list);
    PASS();
}


TEST test_linked_list_multithreaded(void) {
    linked_list_uint32 *list = linked_list_uint32_new();

    thrd_t threads[NUM_THREADS];
    for (size_t i = 0; i < NUM_THREADS; i++) {
        thrd_create(&threads[i], linked_list_thread, list);
    }
    for (size_t i = 0; i < NUM_THREADS; i++) {
        thrd_join(threads[i], NULL);
    }
    size_t size = atomic_load(&list->size);
    ASSERT_EQ(size, NUM_THREADS * NUM_PUSHES * 3 / 4);

    linked_list_uint32_destroy(list);
    PASS();
}

/* Add definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line options, initialization. */

    RUN_TEST(test_linked_list);
    RUN_TEST(test_linked_list_multithreaded);

    GREATEST_MAIN_END();        /* display results */
}