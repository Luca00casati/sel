CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11
LDFLAGS = -lgc

# Source files
SRCS = sel.c
TEST_SRCS = test_sel.c sel.c

# Targets
all: sel test_sel

# Build the REPL
sel: $(SRCS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

# Build the test executable
test_sel: $(TEST_SRCS)
	$(CC) $(CFLAGS) -DTEST_BUILD $^ $(LDFLAGS) -o $@

# Run tests
run_tests: test_sel
	./test_sel

# Clean build artifacts
clean:
	rm -f sel test_sel

.PHONY: all clean run_tests
