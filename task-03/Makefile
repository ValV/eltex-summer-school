# Description	: Build rules for task-03 (trivial ncurses file browser)
# Author	: ValV <ValV@users.noreply.github.com>

CFLAGS=-lpanel -lmenu -lncurses

PROG=ncfb

all : $(PROG)

%: %.c
	gcc $(CFLAGS) -o $@ $<

.PHONY: clean

clean:
	rm -f $(PROG)
