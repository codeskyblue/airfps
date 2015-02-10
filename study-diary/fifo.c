/*
 * fifo.c
 * Copyright (C) 2015 hzsunshx <hzsunshx@onlinegame-13-180>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

int main(){
	char* fifo_name = "/tmp/my_fifo";
	if (access(fifo_name, F_OK) == -1){
		int res = mkfifo(fifo_name, 0777);
		printf("FIFO created %s\n", res == 0 ? "success" : "failed");
	} else {
		printf("file exists\n");
		unlink(fifo_name);
	}
	return 0;
}


