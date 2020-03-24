all: libxahead.so libxahead32.so testexe

CFLAGS = `pkg-config --cflags glib-2.0 x11` -Wall -fPIC -shared
CFLAGS32 = `pkg-config --personality=i686-pc-linux-gnu --cflags glib-2.0 x11` -Wall -fPIC -shared
LDLIBS = `pkg-config --libs glib-2.0 x11`
EXEFLAGS= `pkg-config --cflags glib-2.0 x11` -Wall
CC = gcc
SOURCES=main.c config.c

libxahead.so: $(SOURCES)
	$(CC) $^ $(CFLAGS) $(LDLIBS) -o $@

libxahead32.so: $(SOURCES)
	$(CC) $^ $(CFLAGS32) $(LDLIBS) -L/usr/lib32/glib-2.0/include -m32 -o $@

testexe: test.c config.c
	$(CC) $^ $(EXEFLAGS) $(LDLIBS) -O0 -g -fprofile-arcs -ftest-coverage -o $@

test: testexe
	./testexe

config.gcda: testexe
	./testexe

config.c.gcov: config.gcda
	gcov -dfkq config.gcda

coverage: config.c.gcov
	cat $^

clean:
	rm -fv *.{gcda,gcno,gcov,so} testexe

install: libxahead.so libxahead32.so
	cp -f libxahead.so /usr/lib/
	cp -f libxahead32.so /usr/lib32/libxahead.so
