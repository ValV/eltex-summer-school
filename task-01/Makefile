# Description	: Build rules for task-01 (complex number calculator)
# Author	: ValV <valv at linuxmail dot org>

##############################################################################
#                                 Variables                                  #
##############################################################################
# Compiler variables
CC=gcc
CFLAGS=-std=c99

# Project tree variables
BINDIR=bin
OBJDIR=bin/obj
SRCDIR=src

VPATH=$(SRCDIR)

# Program binary files
PROG=complex
LIST=$(addprefix $(BINDIR)/, $(PROG))

##############################################################################
#                               Generic rules                                #
##############################################################################
# All binaries target
all: $(LIST)

# Rule to build each binary from object files
$(BINDIR)/%: $(OBJDIR)/%.o
	@echo Building binary files
	$(CC) -o $@ $<

# Rule to build each object file (and output directory) if sources are changed
$(OBJDIR)/%.o: %.c %.h | $(OBJDIR)
	@echo Building object files
	$(CC) $(CFLAGS) -c -o $@ $<

# Rule (empty) for header files if a header is not found for an object above
%.h: ;

# Rule to create output directory for binary and object files
$(OBJDIR):
	mkdir -p $(OBJDIR)

##############################################################################
#                              Specific rules                                #
##############################################################################
# For any targets with specific rules that can't be done with generic ones

##############################################################################
#                                Misc rules                                  #
##############################################################################
# A rule for phony targets that are not files (avoid searching for such files)
.PHONY: clean

clean:
	@rm -rfv bin
