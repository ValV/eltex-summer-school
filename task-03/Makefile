# Makefile for task-03

CFLAGS=-lpanel -lmenu -lncurses

ncfm: ncfm.c
	gcc $(CFLAGS) -o $@ $<

.PHONY: clean

clean:
	@rm -fv ncfm
