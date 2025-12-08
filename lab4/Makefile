CC=gcc
CFLAGS=-std=c11 -O2 -Wall -Wextra -Wpedantic -I./include
LDFLAGS=
LDLIBS=

LIBDIR=lib
LIBSRC=libsrc
SRCDIR=src

.PHONY: all clean run1 run2

all: $(LIBDIR)/libvariant36_impl1.so $(LIBDIR)/libvariant36_impl2.so program1 program2

$(LIBDIR)/libvariant36_impl1.so: $(LIBSRC)/variant36_impl1.c include/contracts.h | $(LIBDIR)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

$(LIBDIR)/libvariant36_impl2.so: $(LIBSRC)/variant36_impl2.c include/contracts.h | $(LIBDIR)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

$(LIBDIR):
	mkdir -p $(LIBDIR)

# Program #1: link-time binding to impl1
program1: $(SRCDIR)/program1_linktime.c include/contracts.h $(LIBDIR)/libvariant36_impl1.so
	$(CC) $(CFLAGS) -o $@ $(SRCDIR)/program1_linktime.c -L./$(LIBDIR) -lvariant36_impl1 -Wl,-rpath,'$$ORIGIN/$(LIBDIR)'

# Program #2: run-time loading via dlopen
program2: $(SRCDIR)/program2_runtime.c include/contracts.h
	$(CC) $(CFLAGS) -o $@ $(SRCDIR)/program2_runtime.c -ldl

run1: program1
	./program1

run2: all
	./program2

clean:
	rm -f program1 program2
	rm -f $(LIBDIR)/*.so
