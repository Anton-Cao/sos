# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/

# includes
IDIR=./include
_DEPS=sos.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))

# libraries
LDIR=./lib
LIBS=-lcurl

# source
SDIR=./src

# object files
ODIR=$(SDIR)/obj
_OBJ=cmd.o error_filter.o sos.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

# compilation
CC=gcc
CFLAGS=-I$(IDIR) -I$(LDIR)

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
endif

sos: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR):
	mkdir -p $@

.PHONY: clean install uninstall

# installation
PREFIX=/usr/local/bin
BINARY=sos
CONFIG=.sosrc

install: sos
	cp $< $(PREFIX)/$(BINARY)
	cp -n $(CONFIG).default $(HOME)/$(CONFIG) || true

uninstall:
	rm $(PREFIX)/$(BINARY)

clean:
	rm -f $(ODIR)/*.o sos

