/*
 * utils.c
 * Copyright (C) 2015 hzsunshx <hzsunshx@onlinegame-13-180>
 *
 * Distributed under terms of the MIT license.
 */

#include <elf.h>
#include <stdio.h>    
#include <stdlib.h>
#include <unistd.h>    
#include <fcntl.h>
#include <sys/stat.h>

#include "utils.h"
#define LOG(fmt, args...)		printf(fmt, ##args)

long get_module_base(pid_t pid, char* module_name){  
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
    return addr;  
}  
/**
* lookup symbol's GOT entry address
*
* module_path, absolute path of the module which imports symbol
* symbol_name, name of the target symbol
*/
long find_got_entry_address(pid_t pid, char *module_path, char *symbol_name){
	long module_base = get_module_base(pid, module_path);

	if ( module_base == 0 ){
		LOG("[-] it seems that process %d does not dependent on %s\n", pid, module_path);
		return 0;
	}

	LOG("[+] base address of %s: 0x%x\n", module_path, module_base);

	int fd = open(module_path, O_RDONLY);
	if ( fd == -1 ) {
		LOG("[-] open %s error!", module_path);
		return 0;
	}

	Elf32_Ehdr *elf_header = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
	if ( read(fd, elf_header, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr) ) {
		LOG("[-] read %s error! in %s at line %d\n", module_path, __FILE__, __LINE__);
		return 0;
	}

	uint32_t sh_base = elf_header->e_shoff;
	uint32_t ndx = elf_header->e_shstrndx;
	uint32_t shstr_base = sh_base + ndx * sizeof(Elf32_Shdr);
	LOG("[+] start of section headers: 0x%x\n", sh_base);
	LOG("[+] section header string table index: %d\n", ndx);
	LOG("[+] section header string table offset: 0x%x\n", shstr_base);

	lseek(fd, shstr_base, SEEK_SET);
	Elf32_Shdr *shstr_shdr = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr));
	if ( read(fd, shstr_shdr, sizeof(Elf32_Shdr)) != sizeof(Elf32_Shdr) )
	{
	 LOG("[-] read %s error! in %s at line %d\n", module_path, __FILE__, __LINE__);
	 return 0;
	}
	LOGI("[+] section header string table offset: 0x%x", shstr_shdr->sh_offset);

	char *shstrtab = (char *)malloc(sizeof(char) * shstr_shdr->sh_size);
	lseek(fd, shstr_shdr->sh_offset, SEEK_SET);
	if ( read(fd, shstrtab, shstr_shdr->sh_size) != shstr_shdr->sh_size ) {
		LOG("[-] read %s error! in %s at line %d\n", module_path, __FILE__, __LINE__);
		return 0;
	}

	Elf32_Shdr *shdr = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr));
	Elf32_Shdr *relplt_shdr = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr));
	Elf32_Shdr *dynsym_shdr = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr));
	Elf32_Shdr *dynstr_shdr = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr));

	lseek(fd, sh_base, SEEK_SET);
	if ( read(fd, shdr, sizeof(Elf32_Shdr)) != sizeof(Elf32_Shdr) ) {
		LOG("[-] read %s error! in %s at line %d\n", module_path, __FILE__, __LINE__);
		perror("Error");
		return 0;
	}
	int i = 1;
	char *s = NULL;
	for ( ; i < elf_header->e_shnum; i++ ) {
		s = shstrtab + shdr->sh_name;
		if ( strcmp(s, ".rel.plt") == 0 )
			memcpy(relplt_shdr, shdr, sizeof(Elf32_Shdr));
		else if ( strcmp(s, ".dynsym") == 0 )
			memcpy(dynsym_shdr, shdr, sizeof(Elf32_Shdr));
		else if ( strcmp(s, ".dynstr") == 0 )
			memcpy(dynstr_shdr, shdr, sizeof(Elf32_Shdr));

		if ( read(fd, shdr, sizeof(Elf32_Shdr)) != sizeof(Elf32_Shdr) ) {
			LOG("[-] read %s error! i = %d, in %s at line %d", module_path, i, __FILE__, __LINE__);
			return 0;
		}
	}

	LOG("[+] offset of .rel.plt section: 0x%x\n", relplt_shdr->sh_offset);
	LOG("[+] offset of .dynsym section: 0x%x\n", dynsym_shdr->sh_offset);
	LOG("[+] offset of .dynstr section: 0x%x\n", dynstr_shdr->sh_offset);

	// read dynmaic symbol string table
	char *dynstr = (char *)malloc(sizeof(char) * dynstr_shdr->sh_size);
	lseek(fd, dynstr_shdr->sh_offset, SEEK_SET);
	if ( read(fd, dynstr, dynstr_shdr->sh_size) != dynstr_shdr->sh_size ) {
		LOGE("[-] read %s error!", module_path);
		return 0;
	}

	// read dynamic symbol table
	Elf32_Sym *dynsymtab = (Elf32_Sym *)malloc(dynsym_shdr->sh_size);
	lseek(fd, dynsym_shdr->sh_offset, SEEK_SET);
	if ( read(fd, dynsymtab, dynsym_shdr->sh_size) != dynsym_shdr->sh_size ) {
		LOGE("[-] read %s error!", module_path);
		return 0;
	}

	// read each entry of relocation table
	Elf32_Rel *rel_ent = (Elf32_Rel *)malloc(sizeof(Elf32_Rel));
	lseek(fd, relplt_shdr->sh_offset, SEEK_SET);
	if ( read(fd, rel_ent, sizeof(Elf32_Rel)) != sizeof(Elf32_Rel) ) {
		LOG("[-] read %s error!\n", module_path);
		return 0;
	}
	int found = 0;
	for ( i = 0; i < relplt_shdr->sh_size / sizeof(Elf32_Rel); i++ ) {
		ndx = ELF32_R_SYM(rel_ent->r_info);
		LOG("name: %s\n", dynstr + dynsymtab[ndx].st_name);

		if ( strcmp(dynstr + dynsymtab[ndx].st_name, symbol_name) == 0 ) {
			LOG("[+] got entry offset of %s: 0x%x\n", symbol_name, rel_ent->r_offset);
			found = 1;
			break;
		}
		if ( read(fd, rel_ent, sizeof(Elf32_Rel)) != sizeof(Elf32_Rel) ) {
			LOG("[-] read %s error!\n", module_path);
			return 0;
		}
	}
	if (!found){
		LOG("[-] symbol not found\n");
		return 0;
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
	LOG("offset: %p, base: %p\n", offset, module_base);
	if ( type == ET_EXEC )
		return (long)offset;
	else if ( type == ET_DYN )
		return (long)(offset + module_base);

	return 0;
}
