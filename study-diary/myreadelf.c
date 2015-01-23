/*
 * myreadelf.c
 * dump section headers
 * Copyright (C) 2015 hzsunshx <hzsunshx@onlinegame-13-180>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#define LOGD(fmt, args...) {printf("[D] "); printf(fmt, ##args); printf("\n");}
#define LOGI(fmt, args...) {printf("[I] "); printf(fmt, ##args); printf("\n");}
#define LOGE(fmt, args...) {printf("[E] "); printf(fmt, ##args); printf("\n"); exit(1);}

#if defined(__amd64__)
# define ELF(item) Elf64_##item
#elif defined(__i386__)
# define ELF(item) Elf32_##item
#elif defined(__arm__)
# define ELF(item) Elf32_##item
#else
# error "Not supported"
#endif

typedef unsigned long ulong;

void dump(void* s, int len){
	int i;
	uint8_t *ent = (uint8_t*)s;
	for(i = 0; i < len; i++){
		if (i % 8 == 0){
			printf("\n");
		}
		printf("%02x ", ent[i]);
	}
	printf("\n");
}

#define LIBPATH  "max.o"

int main(){
	FILE* fp;
	fp = fopen(LIBPATH, "rb");
	if (fp == NULL){
		LOGE("file(max.o) open error");
	}
	ELF(Ehdr) ehdr;
	printf("size ehdr: %d\n", sizeof(ehdr));
	int n = fread(&ehdr, 1, sizeof(ehdr), fp);
	if (n != sizeof(ehdr)){
		LOGE("read error in %s at line %d", __FILE__, __LINE__);
	}
	LOGI("section header offset: 0x%x", ehdr.e_shoff);
	LOGI("strtab index: %d", ehdr.e_shstrndx);
	//dump(&ehdr, sizeof(ehdr));

	ulong sh_base = ehdr.e_shoff; // section header start
	ulong strndx = ehdr.e_shstrndx;

	// section header string tab
	ELF(Shdr) shdr;
	fseek(fp, sh_base + strndx*sizeof(ELF(Shdr)), SEEK_SET);
	n = fread(&shdr, 1, sizeof(shdr), fp);
	assert(n != 0);
	LOGD(".shstrtab: offset: 0x%x, size: 0x%x", shdr.sh_offset, shdr.sh_size);

	// read shstrtab
	char *shstrtab = (char*)malloc(sizeof(char) * shdr.sh_size);
	fseek(fp, shdr.sh_offset, SEEK_SET);
	n = fread(shstrtab, 1, shdr.sh_size, fp);
	assert(n == shdr.sh_size);
	
	// read shdr one by one and get it's name
	fseek(fp, sh_base, SEEK_SET);
	int i = 0;
	for (; i < ehdr.e_shnum; i++){
		n = fread(&shdr, 1, sizeof(shdr), fp);
		assert(n == sizeof(shdr));
		char *name = shstrtab + shdr.sh_name;
		LOGI("%2d name: %-14s\toffset: 0x%06x size: 0x%04x", i, name, shdr.sh_offset, shdr.sh_size);
	}
	
	//ELF(Shdr) shdr_dynsym, shdr_synstr;
	//dump(shstrtab, n);
	fclose(fp);
	return 0;
}
