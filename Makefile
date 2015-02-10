#
# Makefile
# hzsunshx, 2015-02-10 09:45
#

OBJS = libfps.so airfps

all: $(OBJS)
	@echo "Makefile needs your attention"


libfps.so: fps.c utils.c
	gcc -Werror -fpic -c -llog -landroid -lEGL -lGLESv2 fps.c
	gcc -shared -o $@ fps.o
	adb push libfps.so /data/local/tmp/

airfps: airfps.go utils.c
	go build
	adb push airfps /data/local/tmp
	adb shell chmod 777 /data/local/tmp/airfps

install: clean all

clean:
	rm -f $(OBJS)

# vim:ft=make
#
