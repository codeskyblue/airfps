/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <elf.h>
#include <fcntl.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <sys/mman.h>


#define LOG_TAG "INJECT"  
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)
  
#define PAGE_START(addr, size) ~((size) - 1) & (addr)

/* This is a trivial JNI example where we use a native method
 * to return a new VM String. See the corresponding Java source
 * file located at:
 *
 *   apps/samples/hello-jni/project/src/com/example/hellojni/HelloJni.java
 */

// EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf) = -1;
//
// EGLBoolean new_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
// {
//	LOGD("new eglSwapBuffers\n");
//	if(-1 == old_eglSwapBuffers)
//		LOGE("error\n");
//	return old_eglSwapBuffers(dpy, surface);
// }
//
//
/*
 uint32_t get_module_base(pid_t pid, const char* module_name)
 {
	FILE *fp = NULL;
	uint32_t addr = 0;
	char *pch =NULL;
	char filename[32];
	char line[512];
	
	if(0 > pid) {
		snprintf(filename, sizeof(filename), "/proc/self/maps", pid);
	}
	else {
		snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
	}
	
	if(NULL == (fp = fopen(filename, "r")))
	{
		LOGE("open %s failed!", filename);
		return 0;
	}
	
	while(fgets(line, sizeof(line), fp)){
		if(strstr(line, module_name)){
			pch = strtok(line, '-');
			addr = strtoul(pch, NULL, 16);
			break;
		}
	}
	fclose(fp);

	return addr;
 }
*/
void* get_module_base(pid_t pid, const char* module_name)  
{  
    FILE *fp;  
    long addr = 0;  
    char *pch;  
    char filename[32];  
    char line[1024];  
  
    if (pid < 0) {  
        /* self process */  
        snprintf(filename, sizeof(filename), "/proc/self/maps", pid);  
    } else {  
        snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);  
    }  
  
    fp = fopen(filename, "r");  
  
    if (fp != NULL) {  
        while (fgets(line, sizeof(line), fp)) {  
            if (strstr(line, module_name)) {  
                pch = strtok( line, "-" );  
                addr = strtoul( pch, NULL, 16 );  
  
                if (addr == 0x8000)  
                    addr = 0;  
  
                break;  
            }  
        }  
  
        fclose(fp) ;  
    }  
  
    return (void *)addr;  
}  
  
 

 /**
  * lookup symbol's GOT entry address
  *
  * module_path, absolute path of the module which imports symbol
  * symbol_name, name of the target symbol
  */
 uint32_t find_got_entry_address(const char *module_path, const char *symbol_name)
 {
	 LOGI("entry address1\n");
     void * module_base = get_module_base(-1, module_path);
	 LOGI("entry address2 %x\n", module_base);

     if ( module_base == 0 )
     {
         LOGE("[-] it seems that process %d does not dependent on %s", getpid(), module_path);
         return 0;
     }

     LOGI("[+] base address of %s: 0x%x", module_path, module_base);

     int fd = open(module_path, O_RDONLY);
     if ( fd == -1 )
     {
         LOGE("[-] open %s error!", module_path);
         return 0;
     }

     Elf32_Ehdr *elf_header = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
     if ( read(fd, elf_header, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr) )
     {
         LOGE("[-] read %s error! in %s at line %d", module_path, __FILE__, __LINE__);
         return 0;
     }

     uint32_t sh_base = elf_header->e_shoff;
     uint32_t ndx = elf_header->e_shstrndx;
     uint32_t shstr_base = sh_base + ndx * sizeof(Elf32_Shdr);
     LOGI("[+] start of section headers: 0x%x", sh_base);
     LOGI("[+] section header string table index: %d", ndx);
     LOGI("[+] section header string table offset: 0x%x", shstr_base);

     lseek(fd, shstr_base, SEEK_SET);
     Elf32_Shdr *shstr_shdr = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr));
     if ( read(fd, shstr_shdr, sizeof(Elf32_Shdr)) != sizeof(Elf32_Shdr) )
     {
         LOGE("[-] read %s error! in %s at line %d", module_path, __FILE__, __LINE__);
         return 0;
     }
     LOGI("[+] section header string table offset: 0x%x", shstr_shdr->sh_offset);

     char *shstrtab = (char *)malloc(sizeof(char) * shstr_shdr->sh_size);
     lseek(fd, shstr_shdr->sh_offset, SEEK_SET);
     if ( read(fd, shstrtab, shstr_shdr->sh_size) != shstr_shdr->sh_size )
     {
         LOGE("[-] read %s error! in %s at line %d", module_path, __FILE__, __LINE__);
         return 0;
     }

     Elf32_Shdr *shdr = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr));
     Elf32_Shdr *relplt_shdr = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr));
     Elf32_Shdr *dynsym_shdr = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr));
     Elf32_Shdr *dynstr_shdr = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr));

     lseek(fd, sh_base, SEEK_SET);
     if ( read(fd, shdr, sizeof(Elf32_Shdr)) != sizeof(Elf32_Shdr) )
     {
         LOGE("[-] read %s error! in %s at line %d", module_path, __FILE__, __LINE__);
         perror("Error");
         return 0;
     }
     int i = 1;
     char *s = NULL;
     for ( ; i < elf_header->e_shnum; i++ )
     {
         s = shstrtab + shdr->sh_name;
         if ( strcmp(s, ".rel.plt") == 0 )
             memcpy(relplt_shdr, shdr, sizeof(Elf32_Shdr));
         else if ( strcmp(s, ".dynsym") == 0 )
             memcpy(dynsym_shdr, shdr, sizeof(Elf32_Shdr));
         else if ( strcmp(s, ".dynstr") == 0 )
             memcpy(dynstr_shdr, shdr, sizeof(Elf32_Shdr));

         if ( read(fd, shdr, sizeof(Elf32_Shdr)) != sizeof(Elf32_Shdr) )
         {
             LOGE("[-] read %s error! i = %d, in %s at line %d", module_path, i, __FILE__, __LINE__);
             return 0;
         }
     }

     LOGI("[+] offset of .rel.plt section: 0x%x", relplt_shdr->sh_offset);
     LOGI("[+] offset of .dynsym section: 0x%x", dynsym_shdr->sh_offset);
     LOGI("[+] offset of .dynstr section: 0x%x", dynstr_shdr->sh_offset);

     // read dynmaic symbol string table
     char *dynstr = (char *)malloc(sizeof(char) * dynstr_shdr->sh_size);
     lseek(fd, dynstr_shdr->sh_offset, SEEK_SET);
     if ( read(fd, dynstr, dynstr_shdr->sh_size) != dynstr_shdr->sh_size )
     {
         LOGE("[-] read %s error!", module_path);
         return 0;
     }

     // read dynamic symbol table
     Elf32_Sym *dynsymtab = (Elf32_Sym *)malloc(dynsym_shdr->sh_size);
     lseek(fd, dynsym_shdr->sh_offset, SEEK_SET);
     if ( read(fd, dynsymtab, dynsym_shdr->sh_size) != dynsym_shdr->sh_size )
     {
         LOGE("[-] read %s error!", module_path);
         return 0;
     }

     // read each entry of relocation table
     Elf32_Rel *rel_ent = (Elf32_Rel *)malloc(sizeof(Elf32_Rel));
     lseek(fd, relplt_shdr->sh_offset, SEEK_SET);
     if ( read(fd, rel_ent, sizeof(Elf32_Rel)) != sizeof(Elf32_Rel) )
     {
         LOGE("[-] read %s error!", module_path);
         return 0;
     }
     for ( i = 0; i < relplt_shdr->sh_size / sizeof(Elf32_Rel); i++ )
     {
         ndx = ELF32_R_SYM(rel_ent->r_info);
         if ( strcmp(dynstr + dynsymtab[ndx].st_name, symbol_name) == 0 )
         {
             LOGI("[+] got entry offset of %s: 0x%x", symbol_name, rel_ent->r_offset);
             break;
         }
         if ( read(fd, rel_ent, sizeof(Elf32_Rel)) != sizeof(Elf32_Rel) )
         {
             LOGE("[-] read %s error!", module_path);
             return 0;
         }
     }

     uint32_t offset = rel_ent->r_offset;
     Elf32_Half type = elf_header->e_type; // ET_EXEC or ET_DYN

     free(elf_header);
     free(shstr_shdr);
     free(shstrtab);
     free(shdr);
     free(relplt_shdr);
     free(dynsym_shdr);
     free(dynstr_shdr);
     free(dynstr);
     free(dynsymtab);
     free(rel_ent);

     // GOT entry offset is different between ELF executables and shared libraries
     if ( type == ET_EXEC )
         return offset;
     else if ( type == ET_DYN )
         return offset + module_base;

     return 0;
 }

/**
* replace GOT entry content of the function indicated by symbol name
* with the address of hook_func
*
* return original name if SUCC
* return 0 if FAILED
*/
void* do_hook(const char *module_path, void* hook_func, const char *symbol_name){
 uint32_t entry_addr = find_got_entry_address(module_path, symbol_name);

 if ( entry_addr == 0 )
	 return 0;

 void * original_addr = 0;
 // save original GOT entry content
 memcpy(&original_addr, (uint32_t *)entry_addr, sizeof(uint32_t));

 LOGD("[+] hook_fun addr: 0x%x", hook_func);
 LOGD("[+] got entry addr: 0x%x", entry_addr);
 LOGD("[+] original addr: 0x%x", original_addr);

 uint32_t page_size = getpagesize();
 uint32_t entry_page_start = PAGE_START(entry_addr, page_size);
 LOGD("[+] page size: 0x%x", page_size);
 LOGD("[+] entry page start: 0x%x", entry_page_start);

 // change the property of current page to writeable
 mprotect((uint32_t *)entry_page_start, page_size, PROT_READ | PROT_WRITE);

 // replace GOT entry content with hook_func's address
 memcpy((uint32_t *)entry_addr, &hook_func, sizeof(uint32_t));

 return original_addr;
}

 /**
 #define LIBSF_PATH "/system/lib/libsurfaceflinger.so"
 int hook_eglSwapBuffers()
 {
	old_eglSwapBuffers = eglSwapBuffers;
	LOGD("Orig eglSwapBuffers = %p\n", old_eglSwapBuffers);
	void* base_addr = get_module_base(getpid(), LIBSF_PATH);
	LOGD("libsurfaceflinger.so addr = %p\n", base_addr);
	
	int fd;
	fd = open(LIBSF_PATH, O_RDONLY);
	if(-1 == fd)
	{
		LOGD("error\n");
		return -1;
	}
	
	Elf32_Ehdr ehdr;
	read(fd, &ehdr, sizeof(Elf32_Ehdr));
	
	unsigned long shdr_addr = ehdr.e_shoff;
	int shnum = ehdr.e_shnum;
	int shent_size = ehdr.e_shentsize;
	unsigned long stridx = ehdr.e_shstrndx;
	
	Elf32_Shdr shdr;
	lseek(fd, shdr_addr + stridx * shent_size, SEEK_SET);
	read(fd, &shdr, shent_size);

	char * string_table = (char *)malloc(shdr.sh_size);
	lseek(fd, shdr.sh_offset, SEEK_SET);
	read(fd, string_table, shdr.sh_size);
	lseek(fd, shdr_addr, SEEK_SET);

	int i;
	uint32_t out_addr = 0;
	uint32_t out_size = 0;
	uint32_t got_item = 0;
	int32_t got_found = 0;

	for(i = 0; i < shnum; i++)
	{
		read(fd, &shdr, shent_size);
		if(shdr.sh_type == SHT_PROGBITS)
		{
			int name_idx = shdr.sh_name;
			if(strcmp(&(string_table[name_idx]), ".got.plt") == 0
					|| strcmp(&(string_table[name_idx]), ".got") == 0)
			{
				out_addr = base_addr + shdr.sh_addr;
				out_size = shdr.sh_size;
				LOGD("out_addr = %lx, out_size = %lx\n", out_addr, out_size);

				for(i = 0; i < out_size; i += 4)
				{
					got_item = *(uint32_t *)(out_addr + i);
					if(got_item == old_eglSwapBuffers)
					{
						LOGD("found eglSwapBuffers in got\n");
						got_found = 1;

						uint32_t page_size = getpagesize();
						uint32_t entry_page_start = (out_addr + i)&(~(page_size -1));
						mprotect((uint32_t *)entry_page_start, page_size, PROT_READ|PROT_WRITE);
						*(uint32_t *)(out_addr + i) = new_eglSwapBuffers;
						break;
					}
					else if(got_item == new_eglSwapBuffers)
					{
						LOGD("Already hooked\n");
						break;
					}
				}
				if(got_found)
					break;
			}
		}
	}
	free(string_table);
	close(fd);
 }
 
*/
 

EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf);
EGLBoolean (*orig_eglCopyBuffers)(EGLDisplay dpy, EGLSurface surf, NativePixmapType pixmap);
EGLBoolean (*orig_eglInitialize)(EGLDisplay display, EGLint * major, EGLint * minor);

unsigned int frames = 0;
struct timeval start_time, curr_time;
float elapsed = 0.0;
float fps = 0.0;

// return millisecond
float time_elapsed(struct timeval t0, struct timeval t1){
	return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

EGLBoolean hooked_eglCopyBuffers(EGLDisplay display, EGLSurface surface, NativePixmapType native_pixmap){
	LOGI("INJECT hook copy buffer");
	return EGL_TRUE;
}

EGLBoolean hooked_eglInitialize(EGLDisplay display, EGLint * major, EGLint * minor){
	LOGI("INJECT hook initialize");
	return orig_eglInitialize(display, major, minor);
}

EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf)
{
	if (frames == 0){
		gettimeofday(&start_time, NULL);
		LOGI("INJECT hook reset clock");
	}
	frames++;
	gettimeofday(&curr_time, NULL);
	elapsed = time_elapsed(start_time, curr_time);
	if (elapsed > 1000.0) { // > 1s
		fps = frames * 1000.0 / elapsed;
		LOGI("INJECT hook fps=%.2f frames=%d elapsed=%.2f\n", fps, frames, elapsed);
		frames = 0;
	}

	LOGI("eglSwapBuffers is invoked %p %p\n", orig_eglSwapBuffers, eglSwapBuffers);

	return (*orig_eglSwapBuffers)(dpy, surf);
}


void hook_entry(char* a)
{
	LOGI("HOOK success, pid=%d\n", getpid());
	LOGI("Start hooking\n");
	//hook_eglSwapBuffers();
	char *sym = "eglSwapBuffers";
	char *module_path = "/system/lib/libsurfaceflinger.so";

	orig_eglSwapBuffers = do_hook(module_path, hooked_eglSwapBuffers, sym);
	if(0 == orig_eglSwapBuffers){
		LOGE("Hooked %s failed", sym);
		return ;
	}
	orig_eglCopyBuffers = do_hook(module_path, hooked_eglSwapBuffers, "eglCopyBuffers");
	orig_eglInitialize = do_hook(module_path, hooked_eglInitialize, "eglInitialize");

	LOGI("orignal eglSwapBufffers 0x%x", orig_eglSwapBuffers);
}

//jstring
//Java_com_example_hellojni_HelloJni_unimplementedStringFromJNI(JNIENV* env, jobject this)
//{
//	return (*env)->NewStringUTF(env, "unimplementedStringFromJNI!");
//}

