# Build rules for task-01 (complex number calculator)

CC=gcc

all: complex

complex: complex.c
	$(CC) -std=c99 -o $@ $^

.PHONY: clean

clean:
	@rm -fv complex
