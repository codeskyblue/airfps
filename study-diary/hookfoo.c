/*
 * hookfoo.c
 * Copyright (C) 2015 hzsunshx <hzsunshx@onlinegame-13-180>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
 
void foo(void)
{
	puts("Hi, I'm a shared library");
}

void hook_entry(char*s){
	printf("HOOK success\n");
}
