#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "threading/threading.h"
#include "greatest/greatest.h"

#define STACK_NAME stack_uint32_single_threaded
#define STACK_TYPE uint32_t
#include "stack.h"
#undef STACK_NAME
#undef STACK_TYPE

#define STACK_NAME stack_uint32
#define STACK_TYPE uint32_t
#define STACK_THREAD_SAFE
#include "stack.h"
#undef STACK_NAME
#undef STACK_TYPE
#undef STACK_THREAD_SAFE


typedef struct {
    uint64_t i;
    double x[3];
} id_point_t;

#define NUM_THREADS 8
#define NUM_PUSHES 50000

int stack_thread(void *arg) {
    stack_uint32 *list = (stack_uint32 *)arg;
    for (size_t i = 0; i < NUM_PUSHES; i++) {
        stack_uint32_push(list, i);
        if (i % 4 == 0) {
            uint32_t val;
            if (!stack_uint32_pop(list, &val)) {
                return 1;
            }
        }
    }
    return 0;
}


TEST test_stack(void) {
    stack_uint32_single_threaded *list = stack_uint32_single_threaded_new();
    for (uint32_t i = 0; i < NUM_PUSHES; i++) {
        ASSERT(stack_uint32_single_threaded_push(list, i));
    }
    size_t size = stack_uint32_single_threaded_size(list);
    ASSERT_EQ(size, NUM_PUSHES);
    for (uint32_t i = 0; i < NUM_PUSHES; i++) {
        uint32_t val;
        ASSERT(stack_uint32_single_threaded_pop(list, &val));
        ASSERT_EQ(val, NUM_PUSHES - 1 - i);
    }
    size = stack_uint32_single_threaded_size(list);
    ASSERT_EQ(size, 0);
    stack_uint32_single_threaded_destroy(list);
    PASS();
}


TEST test_stack_multithreaded(void) {
    stack_uint32 *list = stack_uint32_new();

    thrd_t threads[NUM_THREADS];
    for (size_t i = 0; i < NUM_THREADS; i++) {
        thrd_create(&threads[i], stack_thread, list);
    }
    for (size_t i = 0; i < NUM_THREADS; i++) {
        thrd_join(threads[i], NULL);
    }
    size_t size = atomic_load(&list->size);
    ASSERT_EQ(size, NUM_THREADS * NUM_PUSHES * 3 / 4);

    stack_uint32_destroy(list);
    PASS();
}

/* Add definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line options, initialization. */

    RUN_TEST(test_stack);
    RUN_TEST(test_stack_multithreaded);

    GREATEST_MAIN_END();        /* display results */
}