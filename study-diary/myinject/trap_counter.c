/*
 * trace_counter.cc
 * Copyright (C) 2015 hzsunshx <hzsunshx@onlinegame-13-180>
 *
 * Distributed under terms of the MIT license.
 */

#include <assert.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dump.h"

#define pt_regs user_regs_struct

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

// ptrace function wrapper
int ptrace_getregs(pid_t pid, struct pt_regs *regs){
	return ptrace(PTRACE_GETREGS, pid, NULL, regs);
}
int ptrace_setregs(pid_t pid, struct pt_regs *regs){
	return ptrace(PTRACE_SETREGS, pid, NULL, regs);
}
int ptrace_attach(pid_t pid){
	return ptrace(PTRACE_ATTACH, pid, NULL, 0);
}
int ptrace_detach(pid_t pid){
	return ptrace(PTRACE_DETACH, pid, NULL, 0);
}
int ptrace_continue(pid_t pid){
	return ptrace(PTRACE_CONT, pid, NULL, 0);
}
long ptrace_retval(struct pt_regs *regs){
#if defined(__arm__)
	return regs->ARM_r0;
#elif defined(__i386__)
	return regs->eax;
#elif defined(__amd64__)
	return regs->rax;
#else
#error "retval not supported"
#endif
}
long ptrace_ip(struct pt_regs *regs){
#if defined(__arm__)
	return regs->ARM_pc;
#elif defined(__i386__)
	return regs->eip;
#elif defined(__amd64__)
	return regs->rip;
#else
#error "retval not supported"
#endif
}


// function call using ptrace
#if defined(__arm__)
int ptrace_call(pid_t pid, long addr, long* args, int nargs, struct pt_regs regs){
	int i;
	for (i=0; i< nargs && i < 4; i++){
		regs->uregs[i] = args[i];
	}

	if (i < nargs) {
		regs->ARM_sp -= (nargs -i) * long_size;
		putdata(pid, regs->ARM_sp, (char*)&args[i], (nargs-i) * long_size);
	}

	regs->ARM_pc = addr;
	if (regs->ARM_pc & 1){
		/* thumb */
		regs->ARM_pc &= (~1u);
		regs->ARM_cpsr |= CPSR_T_MASK;
	} else {
		/* arm */
		regs->ARM_cpsr &= ~CPSR_T_MASK;
	}

	regs->ARM_lr = 0;

	if (ptrace_setregs(pid, regs) != 0
			|| ptrace_continue(pid) != 0){
		LOGE("error\n");
		return -1;
	}

	int stat = 0;
	waitpid(pid, &stat, WUNTRACED);
	while (stat != 0xb7f){
		if (ptrace_continue(pid) == -1){
			LOGE("error waitpid\n");
			return -1;
		}
		waitpid(pid, &stat, WUNTRACED);
	}

	return 0;
}
#elif defined(__amd64__)
//int ptrace_call(pid_t, long addr, long* args, int nargs, struct pt_regs regs){
//	return 0;
//}
#else
#error "Not supported"
#endif

long get_mod_base(pid_t pid, char * mod_name){
	FILE *fp;
	long addr = 0;
	char *pch;
	char filename[32];
	char line[1024];

	if (pid < 0){
		snprintf(filename, sizeof(filename), "/proc/self/maps", pid);
	} else {
		snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
	}

	fp = fopen(filename, "r");
	if (fp != NULL){
		while (fgets(line, sizeof(line), fp)){
			if (strstr(line, mod_name)) {
				pch = strtok(line, "-");
				addr = strtoul(pch, NULL, 16);
				// ? check addr == 0x8000
				break;
			}
		}
		fclose(fp);
	}
	return addr;
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


