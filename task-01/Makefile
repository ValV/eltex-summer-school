# Description	: Build rules for task-01 (complex number calculator)
# Author	: ValV <valv at linuxmail dot org>

##############################################################################
#                                 Variables                                  #
##############################################################################

# Project tree variables
BINDIR=bin
OBJDIR=$(BINDIR)/obj
PLGDIR=$(BINDIR)/plugins
SRCDIR=src

VPATH=$(SRCDIR)

# Compiler variables
CC=gcc
CFLAGS=-std=c99 -fPIC
LDFLAGS=-ldl -Wl,-R,'$$ORIGIN/plugins'

# Program files
PROG=complex
PLUG=cxadd cxsub cxmul cxdiv
APIS=funcscx.h
BINS=$(addprefix $(BINDIR)/, $(PROG))
MODS=$(addprefix $(PLGDIR)/, $(PLUG))

##############################################################################
#                               Generic rules                                #
##############################################################################

# All binaries target
all: $(BINS) $(MODS)

# Rule to build each binary from object files
$(BINDIR)/%: $(OBJDIR)/%.o
	@echo Building binary files
	$(CC) $(LDFLAGS) -o $@ $<

# Rule to build each object file (and output directory) if sources are changed
$(OBJDIR)/%.o: %.c %.h $(APIS) | $(OBJDIR)
	@echo Building object files
	$(CC) $(CFLAGS) -c -o $@ $<

# Rule (empty) for header files if a header is not found for an object above
%.h: ;

# Rule to create output directory for binary and object files
$(OBJDIR):
	mkdir -p $(OBJDIR) $(PLGDIR)

##############################################################################
#                              Specific rules                                #
##############################################################################
# For any targets with specific rules that can't be done with generic ones   #
##############################################################################

# Rule to build "complex" binary from object files
$(PLGDIR)/%: $(PLGDIR)/lib%.so ;

.PRECIOUS: $(PLGDIR)/lib%.so

$(PLGDIR)/lib%.so: $(OBJDIR)/%.o
	@echo Building binary files
	$(CC) -shared -o $@  $<


##############################################################################
#                                Misc rules                                  #
##############################################################################
# A rule for phony targets that are not files (avoid searching for such files)
.PHONY: clean

clean:
	@rm -rfv bin
