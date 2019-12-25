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
_OBJ=error_filter.o sos.o
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

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ sos

