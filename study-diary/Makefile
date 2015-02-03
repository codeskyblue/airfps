#
# Makefile
# hzsunshx, 2015-01-23 17:23
#

all: libfoo.so test libhookfoo.so counter bar inject
	@echo "DONE"

clean:
	rm libfoo.so libhookfoo.so

inject: inject.c
	gcc -o inject inject.c -llog -ldl

libfoo.so: foo.c
	gcc -c -Wall -Werror -fpic foo.c
	gcc -shared -o libfoo.so foo.o

libhookfoo.so: hookfoo.c
	gcc -c -Wall -Werror -fpic hookfoo.c
	gcc -shared -o libhookfoo.so hookfoo.o

test: main.c libfoo.so
	gcc -L$(PWD) -Wall -o test main.c -lfoo

install: all
	@sh push_file.sh counter inject libfoo.so libhookfoo.so airfps bar

# bin files
counter: counter.c libfoo.so
	gcc -o counter counter.c

bar: bar.c libfoo.so
	gcc -o bar -lfoo -L$(PWD) bar.c

# vim:ft=make
#
