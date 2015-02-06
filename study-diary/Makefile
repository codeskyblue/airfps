#
# Makefile
# hzsunshx, 2015-01-23 17:23
#

OBJS = libfoo.so test libhookfoo.so counter bar inject libhookfoo2.so loadso
all: $(OBJS)
	@echo "DONE"

clean:
	rm $(OBJS)

inject: inject.c
	gcc -o inject inject.c -llog -ldl

libfoo.so: foo.c
	gcc -c -Wall -Werror -fpic foo.c
	gcc -shared -o libfoo.so foo.o

libhookfoo.so: hookfoo.c
	gcc -fpic -c -llog -landroid -lEGL -lGLESv2 hookfoo.c
	gcc -shared -o $@ hookfoo.o

libhookfoo2.so: hookfoo2.c
	gcc -fpic -c -llog -landroid -lEGL -lGLESv2 hookfoo2.c
	gcc -shared -o $@ hookfoo2.o

test: main.c libfoo.so
	gcc -L$(PWD) -Wall -o test main.c -lfoo

install: all
	sh push_file.sh airfps $(OBJS)

# bin files
counter: counter.c libfoo.so
	gcc -o counter counter.c

bar: bar.c libfoo.so
	gcc -o bar -lfoo -L$(PWD) bar.c

loadso: loadso.c
	gcc -o $@ loadso.c -ldl

# vim:ft=make
#
