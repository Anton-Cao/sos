# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/

# includes
IDIR=./include
_DEPS=sos.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))

# libraries
LDIR=./lib
LIBS=

# source
SDIR=./src

# object files
ODIR=$(SDIR)/obj
_OBJ=sos.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

# compilation
CC=gcc
CFLAGS=-I$(IDIR) -I$(LDIR)

sos: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR):
	mkdir -p $@

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ sos
