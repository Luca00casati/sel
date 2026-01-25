CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11
LDFLAGS = 

SRCS = sel.c
TEST_SRCS = test_sel.c sel.c

all: sel test_sel

sel: $(SRCS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

test_sel: $(TEST_SRCS)
	$(CC) $(CFLAGS) -DTEST_BUILD $^ $(LDFLAGS) -o $@

run_tests: test_sel
	./test_sel

clean:
	rm -f sel test_sel

.PHONY: all clean run_tests
