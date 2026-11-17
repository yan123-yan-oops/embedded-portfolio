CC      ?= gcc
CFLAGS  ?= -std=c11 -O2 -g -Wall -Wextra

.PHONY: all clean

all: demo

demo: src/memory_pool.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f demo
