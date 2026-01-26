CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11
LDFLAG =
SRCS = sel.c

all: sel test_sel

sel: $(SRCS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

test_sel: $(SRCS)
	$(CC) $(CFLAGS) -DTEST $^ $(LDFLAGS) -o $@

test: test_sel
	./test_sel

clean:
	rm -f sel test_sel

.PHONY: all clean test
