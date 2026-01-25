#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "sel.h"

/* ---------------- Test State ---------------- */
static int tests_total  = 0;
static int tests_failed = 0;

/* ---------------- Test Macro ---------------- */
#define TEST(expr, expected_val) do {                                \
    tests_total++;                                                   \
    Arena* arena = arena_create(1024);                               \
    word64 result = 0;                                               \
    word64 expected = (word64)(expected_val);                        \
                                                                     \
    if (run_expression((expr), &result, arena) != 0) {               \
        printf("[FAIL] %s => ERROR\n", (expr));                      \
        tests_failed++;                                              \
    } else if (result != expected) {                                 \
        printf("[FAIL] %s => got %" PRIu64                           \
               ", expected %" PRIu64 "\n",                           \
               (expr), result, expected);                            \
        tests_failed++;                                              \
    } else {                                                         \
        printf("[PASS] %s => %" PRIu64 "\n",                         \
               (expr), result);                                      \
    }                                                                \
                                                                     \
    arena_destroy(arena);                                            \
} while (0)

/* ---------------- Main ---------------- */
int main(void) {
    printf("Running sel tests...\n\n");

    /* Arithmetic */
    TEST("(+ 1 2)", 3);
    TEST("(+ 1 2 3 4)", 10);
    TEST("(* 2 3 4)", 24);
    TEST("(- 10 4)", 6);
    TEST("(/ 20 5)", 4);

    /* Nested */
    TEST("(+ 1 2 (+ 3 4))", 10);
    TEST("(+ (* 2 3) (- 10 4))", 12);

    /* Comparisons (booleans as 1 / 0) */
    TEST("(= 5 5)", 1);
    TEST("(= 5 6)", 0);
    TEST("(< 3 5)", 1);
    TEST("(> 3 5)", 0);
    TEST("(<= 5 5)", 1);
    TEST("(>= 6 5)", 1);

    /* ---------------- Summary ---------------- */
    printf("\n---------------------------------\n");
    printf("Tests run   : %d\n", tests_total);
    printf("Tests failed: %d\n", tests_failed);
    printf("Tests passed: %d\n", tests_total - tests_failed);
    printf("---------------------------------\n");

    /* Non-zero exit on failure (important!) */
    return tests_failed ? 1 : 0;
}
