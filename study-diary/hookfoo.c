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

#include <android/log.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <sys/mman.h>


#define LOG_TAG "INJECT"  
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)
  
#define PAGE_START(addr, size) ~((size) - 1) & (addr)

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
uint32_t find_got_entry_address(const char *module_path, const char *symbol_name){
	void * module_base = get_module_base(-1, module_path);

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
	 return (uint32_t)(offset + module_base);

	return 0;
}

/**
* replace GOT entry content of the function indicated by symbol name
* with the address of hook_func
*
* return original addr
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


unsigned int frames = 0;
struct timeval start_time, curr_time;
float elapsed = 0.0;
float fps = 0.0;

EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf);
float time_elapsed(struct timeval t0, struct timeval t1){
	// return millisecond
	return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
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
	LOGI("[INJECT] HOOK success, pid=%d\n", getpid());
	LOGI("[INJECT] Start hooking\n");
	/*
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
	*/
}

