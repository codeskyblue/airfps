/*
 * trace_counter.cc
 * Copyright (C) 2015 hzsunshx <hzsunshx@onlinegame-13-180>
 *
 * Distributed under terms of the MIT license.
 */

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dump.h"

const int long_size = sizeof(long);

void getdata(pid_t child, long addr, char *str, int len){
	char *laddr;
	int i, j;
	union u{
		long val;
		char chars[long_size];
	} data;

	i = 0;
	j = len / long_size;
	laddr = str;

	while(i < j){
		data.val = ptrace(PTRACE_PEEKDATA, child, addr + i*long_size, NULL); // 1 or 0
		memcpy(laddr, data.chars, long_size);
		++i;
		laddr += long_size;
	}
	j = len % long_size;
	if (j != 0){
		data.val = ptrace(PTRACE_PEEKDATA, child, addr + i*long_size, NULL);
		memcpy(laddr, data.chars, j);
	}
}

void putdata(pid_t child, long addr, char *str, int len){
	char *laddr;
	int i, j;
	union u {
		long val;
		char chars[long_size];
	} data;
	
	i = 0; 
	j = len / long_size;
	laddr = str;
	while(i < j){
		memcpy(data.chars, laddr, long_size);
		ptrace(PTRACE_POKEDATA, child, addr + i * long_size, data.val);
		++i;
		laddr += long_size;
	}
	j = len % long_size;
	if (j != 0) {
		data.val = ptrace(PTRACE_PEEKDATA, child, addr + i*long_size, NULL); // necessary
		memcpy(data.chars, laddr, j);
		ptrace(PTRACE_POKEDATA, child, addr + i * long_size, data.val);
	}
}

int main(int argc, char *argv[]){
	pid_t traced_process;
	struct user_regs_struct regs, newregs;
	long ins;
	/* int 0x80, int3 */
	char code[] = {0xcd, 0x80, 0xcc};
	char backup[20];
	char backup2[20];
	if(argc != 2){
		printf("Usage: %s <pid to be traced>\n", argv[0]);
		exit(1);
	}

	traced_process = atoi(argv[1]);
	ins = ptrace(PTRACE_ATTACH, traced_process, NULL, NULL);
	printf("attrch: %d\n", ins);
	if (ins == -1){
		printf("traced failed\n");
		exit(1);
	}
	wait(NULL);
	ins = ptrace(PTRACE_GETREGS, traced_process, NULL, &regs);
	assert(ins == 0);

	// Copy instructions into a backup variable
	getdata(traced_process, regs.rip, backup, 16);
	hex_dump("backup", backup, 16);
	// breakpoint
	putdata(traced_process, regs.rip, code, 3);

	// debug
	getdata(traced_process, regs.rip, backup2, 16);
	hex_dump("backup2", backup2, 16);

	// execute the int 3 instruction
	ptrace(PTRACE_CONT, traced_process, NULL, NULL);
	wait(NULL);
	printf("Process stopped\n");
	printf("Press <enter> to continue: \n");
	getchar();

	// restore data
	putdata(traced_process, regs.rip, backup, 3);
	ptrace(PTRACE_SETREGS, traced_process, NULL, &regs);

	// debug
	getdata(traced_process, regs.rip, backup2, 8);
	hex_dump("backup2", backup2, 8);

	//printf("getregs: %d\n", ins);
	//ins = ptrace(PTRACE_PEEKTEXT, traced_process, regs.rip, ins);
	//printf("RIP: %lx Instruction executed: %lx\n", regs.rip, ins);
	ptrace(PTRACE_DETACH, traced_process, NULL, NULL);
	return 0;
}


