/*
 * counter.cc
 * Copyright (C) 2015 hzsunshx <hzsunshx@onlinegame-13-180>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <unistd.h>

int main(){
	printf("pid = %d\n", getpid());
	int i;
	for (i = 0; i < 100; i++){
		printf("My counter: %d\n", i);
		sleep(2);
	}
	return 0;
}

