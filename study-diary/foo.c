/*
 * foo.c
 * Copyright (C) 2015 hzsunshx <hzsunshx@onlinegame-13-180>
 *
 * Distributed under terms of the MIT license.
 */

#include "foo.h"

#include <stdio.h>

int HWC = 7;

void foo(){
	printf("This is shard lib, foo(%d)\n", HWC);
}

void hook_entry(char*a){
	puts("Hook foo success");
}
