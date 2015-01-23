#
# Makefile
# hzsunshx, 2015-01-23 17:23
#

all: inject libfoo.so test libhookfoo.so
	@echo "Makefile needs your attention"

inject: inject.c
	gcc -o inject inject.c -ldl

libfoo.so: foo.c
	gcc -c -Wall -Werror -fpic foo.c
	gcc -shared -o libfoo.so foo.o

libhookfoo.so: hookfoo.c
	gcc -c -Wall -Werror -fpic hookfoo.c
	gcc -shared -o libhookfoo.so hookfoo.o

test: main.c libfoo.so
	gcc -L$(PWD) -Wall -o test main.c -lfoo


# vim:ft=make
#
