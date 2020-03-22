all: libxahead.so libxahead32.so

CC=gcc
OPTS=-Wall -lX11 -ldl -fPIC -shared
SOURCES=main.c

libxahead.so: $(SOURCES)
	$(CC) $(OPTS) $^ -o $@

libxahead32.so: $(SOURCES)
	$(CC) $(OPTS) -m32 $^ -o $@

install: libxahead.so libxahead32.so
	cp -f libxahead.so /usr/lib/
	cp -f libxahead32.so /usr/lib32/libxahead.so
