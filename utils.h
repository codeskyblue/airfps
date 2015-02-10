/*
 * utils.h
 * Copyright (C) 2015 hzsunshx <hzsunshx@onlinegame-13-180>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>    
#include <stdlib.h>    
#include <unistd.h>    
#include <android/log.h>    

//const char * // point address not changed
//char const * // content not changed

#define LIBSF_PATH  "/system/lib/libsurfaceflinger.so"    

long get_module_base(pid_t pid, char* module_name);
long find_got_entry_address(pid_t pid, char *module_path, char *symbol_name);

#define LOG_TAG "INJECT"    
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)
#define DEBUG_PRINT(format,args...) LOGD(format, ##args)    

#define CPSR_T_MASK     ( 1u << 5 )    

#endif /* !UTILS_H */
