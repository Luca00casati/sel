#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <gc/gc.h>

extern int run_expression(const char* expr, uint64_t* out);

static int tests_passed = 0;
static int tests_total = 0;

void test(const char* expr, const char* expected_str) {
    tests_total++;
    uint64_t expected;
    if(strcmp(expected_str,"#t")==0) expected=1;
    else if(strcmp(expected_str,"#f")==0) expected=0;
    else expected = strtoull(expected_str,NULL,10);

    uint64_t result;
    if(run_expression(expr,&result)!=0) {
        printf("Test %d: ERROR evaluating %s\n", tests_total, expr);
        return;
    }

    if(result==expected) {
        printf("Test %d: PASSED\n", tests_total);
        tests_passed++;
    } else {
        printf("Test %d: FAILED\n  Input: %s\n  Expected: %" PRIu64 ", Got: %" PRIu64 "\n",
               tests_total, expr, expected, result);
    }
}

int main() {
    GC_INIT();

    // Arithmetic
    test("(+ 12 12)","24");
    test("(+ 1 2 3 4)","10");
    test("(* 2 3 4)","24");
    test("(- 10 4)","6");
    test("(/ 20 5)","4");

    // Comparisons
    test("(= (+ 10 21) 31)","#t");
    test("(< 10 20)","#t");
    test("(> 5 3)","#t");
    test("(<= 5 5)","#t");
    test("(>= 10 9)","#t");
    test("(= (* 2 3) 6)","#t");
    test("(= (* 2 3) 7)","#f");

    printf("\nPassed %d/%d tests.\n", tests_passed, tests_total);
    return 0;
}
