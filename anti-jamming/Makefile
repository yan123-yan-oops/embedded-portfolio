CC      ?= gcc
CFLAGS  ?= -std=c11 -O2 -g -Wall -Wextra
LDLIBS   = -lm

.PHONY: all clean

all: demo

demo: src/full_chain_check.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -f demo
