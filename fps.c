/*
 * foo.c
 * Copyright (C) 2015 hzsunshx <hzsunshx@onlinegame-13-180>
 *
 * Distributed under terms of the MIT license.
 */
#include <unistd.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <EGL/egl.h>  
#include <GLES/gl.h>  
#include <fcntl.h>  
#include <sys/mman.h>  
#include <time.h>
#include <sys/stat.h>
  
#include <android/log.h>  
#define LOG_TAG "INJECT"  
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)

#define PAGE_START(addr, size) ~((size) - 1) & (addr)

EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf) = NULL;

float time_elapsed(struct timeval t0, struct timeval t1){
	return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

FILE * logfp;
#define LOG(fmt, args...) fprintf(logfp, fmt, ##args)

unsigned int frames = 0;
struct timeval start_time, curr_time;
float elapsed = 0.0;
float fps = 0.0;

EGLBoolean new_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)  
{  
	if (frames == 0){
		gettimeofday(&start_time, NULL);
		LOG("INJECT reset clock");
	}
	frames++;
	gettimeofday(&curr_time, NULL);
	elapsed = time_elapsed(start_time, curr_time);
	if (elapsed > 1000.0) { // > 1s
		fps = frames * 1000.0 / elapsed;
		LOG("INJECT fps=%.2f frames=%d elapsed=%.2f\n", fps, frames, elapsed);
		frames = 0;
	}

    if (old_eglSwapBuffers == NULL){
        LOG("error\n");  
	}
    return old_eglSwapBuffers(dpy, surface);  
}  

int new_puts(const char *a){
	printf("hacked, This is new shard lib\n");
}

int func_replace(long func_addr, long new_func){
	LOG("INJECT Found eglSwapBuffers in got\n");    

	long page_size = getpagesize();  
	long entry_page_start = (func_addr) & (~(page_size - 1));  
	LOG("page start: %p\n", entry_page_start);
	int ret = mprotect((long *)entry_page_start, page_size, PROT_READ | PROT_WRITE);  
	LOG("mprotect return: %d\n", ret);
	if (ret == 0){
		LOG("origin: %p\n", *(long*)(func_addr));// = new_func; //new_eglSwapBuffers;    
		LOG("new   : %p\n", new_func);
		LOG("puts  : %p\n", puts);
		long *target = (long*)(func_addr);
		if (*target == (long)puts){
			*target = new_func;
		} else {
			*target = (long)puts;
		}
	}
}

int hook_entry(char*a){
	logfp = fopen("/data/local/tmp/log.txt", "a+");
	if (logfp == NULL){
		fprintf(logfp, "create log file failed\n");
	}

	LOG("arguments: %s\n", a);
	long addr = 0;
	sscanf(a, "%p", &addr);
	printf("addr = %p\n", addr);
	func_replace(addr, (long)new_puts);
	//int ret = mkfifo("/data/local/tmp/my_fifo", 0777);
	//printf("ret: %d\n", ret);
	printf("Hook foo success\n");

	fclose(logfp);
	return 0;
}
