#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "greatest/greatest.h"

#define STACK_NAME stack_uint32
#define STACK_TYPE uint32_t
#include "stack.h"
#undef STACK_NAME
#undef STACK_TYPE

#define NUM_PUSHES 50000

TEST test_stack(void) {
    stack_uint32 *list = stack_uint32_new();
    for (uint32_t i = 0; i < NUM_PUSHES; i++) {
        ASSERT(stack_uint32_push(list, i));
    }
    size_t size = stack_uint32_size(list);
    ASSERT_EQ(size, NUM_PUSHES);
    for (uint32_t i = 0; i < NUM_PUSHES; i++) {
        uint32_t val;
        ASSERT(stack_uint32_pop(list, &val));
        ASSERT_EQ(val, NUM_PUSHES - 1 - i);
    }
    size = stack_uint32_size(list);
    ASSERT_EQ(size, 0);
    stack_uint32_destroy(list);
    PASS();
}

/* Add definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line options, initialization. */

    RUN_TEST(test_stack);

    GREATEST_MAIN_END();        /* display results */
}