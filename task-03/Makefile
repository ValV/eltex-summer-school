# Makefile for task-03
# (trivial ncurses file browser)

CFLAGS=-lpanel -lmenu -lncurses

PROG=ncfb

all : $(PROG)

%: %.c
	gcc $(CFLAGS) -o $@ $<

.PHONY: clean

clean:
	rm -f $(PROG)
