#! /bin/sh
#
# push_file.sh
# Copyright (C) 2015 hzsunshx <hzsunshx@onlinegame-13-180>
#
# Distributed under terms of the MIT license.
#


for name in "$@"
do
	echo "push -- $name"
	adb push "$name" /data/local/tmp/
	adb shell chmod 777 /data/local/tmp/$name
done
