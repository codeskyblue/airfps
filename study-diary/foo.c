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
	//printf("This is a shared lib\n");
	puts("This is a shared lib");
	//puts("shared lib");
}

void hook_entry(char*a){
	puts("Hook foo success");
}
