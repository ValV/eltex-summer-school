# Description	: Build rules for task-03 (trivial ncurses file browser)
# Author	: ValV <ValV@users.noreply.github.com>

CFLAGS=-lpanel -lmenu -lncurses

ncfm: ncfm.c
	gcc $(CFLAGS) -o $@ $<

.PHONY: clean

clean:
	@rm -fv ncfm
