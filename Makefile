all: libxahead.so libxahead32.so

CFLAGS = `pkg-config --cflags glib-2.0 x11` -Wall -fPIC -shared
CFLAGS32 = `pkg-config --personality=i686-pc-linux-gnu --cflags glib-2.0 x11` -Wall -fPIC -shared
LDLIBS = `pkg-config --libs glib-2.0 x11`
CC = gcc
SOURCES=main.c config.c

libxahead.so: $(SOURCES)
	$(CC) $(CFLAGS) $(LDLIBS) $^ -o $@

libxahead32.so: $(SOURCES)
	$(CC) $^ -m32 $(CFLAGS32) $(LDLIBS) -L/usr/lib32/glib-2.0/include -m32 -o $@

install: libxahead.so libxahead32.so
	cp -f libxahead.so /usr/lib/
	cp -f libxahead32.so /usr/lib32/libxahead.so
