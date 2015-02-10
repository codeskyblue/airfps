/*
 * counter.cc
 * Copyright (C) 2015 hzsunshx <hzsunshx@onlinegame-13-180>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <sys/stat.h>

#include "foo.h"

#include <android/log.h>  
#define LOG_TAG "INJECT"  
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)

int global = 1;

int main(){
	printf("pid = %d\n", getpid());
	//printf("mkfifo addr = %p\n", mkfifo);
	int i;
	for (i = 0; i < 100; i++){
		printf("counter: %d, global: %d\n", i, global++);
		LOGI("bar print %d\n", i);
		foo();
		sleep(2);
	}
	return 0;
}

