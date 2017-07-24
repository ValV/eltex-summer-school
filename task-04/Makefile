# Makefile for task-03
# (trivial ncurses file browser)

CFLAGS=-lform -lmenu -lpanel -lncurses

PROG=ncte

all : $(PROG)

%: %.c
	gcc $(CFLAGS) -o $@ $<

.PHONY: clean

clean:
	rm -f $(PROG)
