#include <stdio.h>    
#include <stdlib.h>    
#include <sys/ptrace.h>
#include <sys/wait.h>    
#include <sys/mman.h>    
#include <dlfcn.h>    
#include <dirent.h>    
#include <unistd.h>    
#include <string.h>    
#include <elf.h>
#include <getopt.h>

// FIXME(ssx): add #cgo CFLAG in header
//#define GOLANG 1

#define CPSR_T_MASK     ( 1u << 5 )    

#if defined(__amd64__)
  #include <sys/reg.h>
  #include <sys/user.h>
  #include <sys/syscall.h>
  #define LOGD(fmt, args...) {printf("[D] "); printf(fmt, ##args); }
  #define LOGI(fmt, args...) {printf("[I] "); printf(fmt, ##args); }
  #define LOGE(fmt, args...) {printf("[E] "); printf(fmt, ##args); exit(1);}
  const char *libc_path = "/lib/x86_64-linux-gnu/libc-2.13.so";    
  const char *linker_path = "/lib/x86_64-linux-gnu/ld-2.13.so";    
#elif defined(__i386__)
  #define pt_regs         user_regs_struct    
  #define LOGD(fmt, args...) {printf("[D] "); printf(fmt, ##args); printf("\n");}
  #define LOGI(fmt, args...) {printf("[I] "); printf(fmt, ##args); printf("\n");}
  #define LOGE(fmt, args...) {printf("[E] "); printf(fmt, ##args); printf("\n"); exit(1);}
  const char *libc_path = "/system/lib/libc.so";    
  const char *linker_path = "/system/bin/linker";    
#elif defined(__arm__)
  #include <android/log.h>    
  #include <asm/user.h>    
  #include <asm/ptrace.h>    
  #define  LOG_TAG "INJECT"    
  #define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
  #define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
  #define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)
  const char *libc_path = "/system/lib/libc.so";    
  const char *linker_path = "/system/bin/linker";    
#else
  #error "Unsupported platform"
#endif
    
#define DEBUG_PRINT(format,args...)  LOGD(format, ##args)    
typedef unsigned long ulong;
    
int ptrace_readdata(pid_t pid,  const uint8_t *src, uint8_t *buf, size_t size){    
    uint32_t i, j, remain;    
    uint8_t *laddr;    
    
    union u {    
        long val;    
        char chars[sizeof(long)];    
    } d;
    
    j = size / sizeof(ulong);    
    remain = size % sizeof(ulong);    
    
    laddr = buf;    
    
    for (i = 0; i < j; i ++) {    
        d.val = ptrace(PTRACE_PEEKTEXT, pid, src, 0);    
        memcpy(laddr, d.chars, sizeof(ulong));    
        src += sizeof(ulong);    
        laddr += sizeof(ulong);    
    }    
    
    if (remain > 0) {    
        d.val = ptrace(PTRACE_PEEKTEXT, pid, src, 0);    
        memcpy(laddr, d.chars, remain);    
    }    
    
    return 0;    
}    
    
int ptrace_writedata(pid_t pid, uint8_t *dest, const uint8_t *data, size_t size)    
{    
    uint32_t i, j, remain;    
    const uint8_t *laddr;    
    
    union u {    
        long val;    
        char chars[sizeof(long)];    
    } d;    
    
    j = size / sizeof(ulong);    
    remain = size % sizeof(ulong);    
    
    laddr = data;
    
    for (i = 0; i < j; i ++) {    
        memcpy(d.chars, laddr, sizeof(ulong));    
        ptrace(PTRACE_POKETEXT, pid, dest, d.val);    
    
        dest  += sizeof(ulong);    
        laddr += sizeof(ulong);    
    }    
    
    if (remain > 0) {    
        d.val = ptrace(PTRACE_PEEKTEXT, pid, dest, 0);    
        for (i = 0; i < remain; i ++) {    
            d.chars[i] = *laddr ++;    
        }    
    
        ptrace(PTRACE_POKETEXT, pid, dest, d.val);    
    }    
    
    return 0;    
}    
    
#if defined(__arm__)    
int ptrace_call(pid_t pid, ulong addr, long *params, uint32_t num_params, struct pt_regs* regs)    
{    
    uint32_t i;
    for (i = 0; i < num_params && i < 4; i ++) {
        regs->uregs[i] = params[i];
    }    
    
    //    
    // push remained params onto stack    
    //    
    if (i < num_params) {
        regs->ARM_sp -= (num_params - i) * sizeof(long);
        ptrace_writedata(pid, (void *)regs->ARM_sp, (uint8_t *)&params[i], (num_params - i) * sizeof(long));
    } 
    
    regs->ARM_pc = addr; 
    if (regs->ARM_pc & 1) { 
        /* thumb */ 
        regs->ARM_pc &= (~1u); 
        regs->ARM_cpsr |= CPSR_T_MASK;
    } else { 
        /* arm */ 
        regs->ARM_cpsr &= ~CPSR_T_MASK;
    } 
    
    regs->ARM_lr = 0; 
    
    if (ptrace_setregs(pid, regs) == -1 
            || ptrace_continue(pid) == -1) {
        printf("error\n");
        return -1;
    } 
    
    int stat = 0;
    waitpid(pid, &stat, WUNTRACED);
    while (stat != 0xb7f) {
        if (ptrace_continue(pid) == -1) {
            printf("error\n");
            return -1;
        }
        waitpid(pid, &stat, WUNTRACED);
    }
    return 0;    
}
    
#elif defined(__i386__)    
long ptrace_call(pid_t pid, ulong addr, long *params, uint32_t num_params, struct pt_regs * regs)    
{    
    regs->esp -= (num_params) * sizeof(long) ;    
    ptrace_writedata(pid, (void *)regs->esp, (uint8_t *)params, (num_params) * sizeof(long));    
    
    long tmp_addr = 0x00;    
    regs->esp -= sizeof(long);    
    ptrace_writedata(pid, regs->esp, (char *)&tmp_addr, sizeof(tmp_addr));     
    
    regs->eip = addr;
    
    if (ptrace_setregs(pid, regs) == -1     
            || ptrace_continue(pid) == -1) {    
        printf("error\n");    
        return -1;    
    }    
    
    int stat = 0;  
    waitpid(pid, &stat, WUNTRACED);  
    while (stat != 0xb7f) {   // ?
        if (ptrace_continue(pid) == -1) {  
            printf("error\n");  
            return -1;  
        }  
        waitpid(pid, &stat, WUNTRACED);  
    }  
    
    return 0;    
}    
#elif defined(__amd64__)    
long ptrace_call(pid_t pid, ulong addr, long *params, uint32_t num_params, struct pt_regs * regs)    
{    
    regs->rsp -= num_params * sizeof(long); 
    ptrace_writedata(pid, (void*)regs->rsp, (uint8_t *)params, num_params * sizeof(long));    
    
    ulong tmp_addr = 0x00;    
    regs->rsp -= sizeof(ulong);    
    ptrace_writedata(pid, (void*)regs->rsp, (char*)&tmp_addr, sizeof(tmp_addr));
    
    regs->rip = addr;
    
    if (ptrace_setregs(pid, regs) == -1     
            || ptrace_continue( pid) == -1) {    
        printf("error\n");    
        return -1;    
    }    
    
    int stat = 0;  
    waitpid(pid, &stat, WUNTRACED);  
    while (stat != 0xb7f) {  
        if (ptrace_continue(pid) == -1) {  
            printf("error\n");  
            return -1;  
        }  
        waitpid(pid, &stat, WUNTRACED);  
    }  
    
    return 0;    
}    
#else     
#error "Not supported"    
#endif    
    
int ptrace_getregs(pid_t pid, struct pt_regs * regs){    
    if (ptrace(PTRACE_GETREGS, pid, NULL, regs) < 0) {    
        perror("ptrace_getregs: Can not get register values");    
        return -1;    
    }    
    return 0;    
}    
    
int ptrace_setregs(pid_t pid, struct pt_regs * regs){    
    if (ptrace(PTRACE_SETREGS, pid, NULL, regs) < 0) {    
        perror("ptrace_setregs: Can not set register values");    
        return -1;    
    }    
    return 0;    
}    
    
int ptrace_continue(pid_t pid){    
    if (ptrace(PTRACE_CONT, pid, NULL, 0) < 0) {    
        perror("ptrace_cont");    
        return -1;    
    }    
    return 0;    
}    
    
int ptrace_attach(pid_t pid){    
    if (ptrace(PTRACE_ATTACH, pid, NULL, 0) < 0) {    
        perror("ptrace_attach");    
        return -1;    
    }    
    
    int status = 0;    
    waitpid(pid, &status , WUNTRACED);    
    
    return 0;    
}    
    
int ptrace_detach(pid_t pid){    
    if (ptrace(PTRACE_DETACH, pid, NULL, 0) < 0) {    
        perror("ptrace_detach");    
        return -1;    
    }    
    
    return 0;    
}    
    
ulong get_module_base(pid_t pid, const char* module_name){    
    FILE *fp;    
    ulong addr = 0;    
    char *pch;    
    char filename[32];    
    char line[1024];    
    
    if (pid < 0) {    
        /* self process */    
        snprintf(filename, sizeof(filename), "/proc/self/maps", pid);    
    } else {    
        snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);    
    }    
    
    fp = fopen(filename, "rb");    
    
    if (fp != NULL) {    
        while (fgets(line, sizeof(line), fp)) {    
            if (strstr(line, module_name)) {    
                pch = strtok(line, "-" );    
                addr = strtoul(pch, NULL, 16);
    
                if (addr == 0x8000)    
                    addr = 0;    
                break;    
            }    
        }    
    
        fclose(fp) ;    
    }    
    
    return addr;    
}
    
void* get_remote_addr(pid_t target_pid, const char* module_name, ulong local_addr){    
    ulong local_handle, remote_handle;
    
    local_handle = get_module_base(-1, module_name); 
    remote_handle = get_module_base(target_pid, module_name);    
    
    DEBUG_PRINT("[+] [%s] get_remote_addr: local[%p], remote[%p]\n", 
			module_name, local_handle, remote_handle);    
    ulong ret_addr = local_addr + remote_handle - local_handle;
    
#if defined(__i386__)    
    if (!strcmp(module_name, libc_path)) {    
        ret_addr += 2;    
    }    
#endif    
    return (void*)ret_addr;    
}    
    
int find_pid_of(const char *process_name){    
    int id;    
    pid_t pid = -1;    
    DIR* dir;    
    FILE *fp;    
    char filename[32];    
    char cmdline[256];    
    
    struct dirent * entry;    
    
    if (process_name == NULL)    
        return -1;    
    
    dir = opendir("/proc");    
    if (dir == NULL)    
        return -1;    
    
    while((entry = readdir(dir)) != NULL) {    
        id = atoi(entry->d_name);    
        if (id != 0) {    
            sprintf(filename, "/proc/%d/cmdline", id);    
            fp = fopen(filename, "r");    
            if (fp) {    
                fgets(cmdline, sizeof(cmdline), fp);    
                fclose(fp);    
    
                if (strcmp(process_name, cmdline) == 0) {    
                    /* process found */    
                    pid = id;    
                    break;    
                }    
            }    
        }    
    }    
    
    closedir(dir);    
    return pid;    
}    
    
long ptrace_retval(struct pt_regs * regs)    
{    
#if defined(__arm__)    
    return regs->ARM_r0;    
#elif defined(__i386__)    
    return regs->eax;    
#elif defined(__amd64__)    
    return regs->rax;    
#else    
#error "Not supported"    
#endif    
}    
    
long ptrace_ip(struct pt_regs * regs)    
{    
#if defined(__arm__)    
    return regs->ARM_pc;    
#elif defined(__i386__)    
    return regs->eip;    
#elif defined(__amd64__)
	return regs->rip;
#else 
#error "Not supported"    
#endif    
}    
    
int ptrace_call_wrapper(pid_t target_pid, const char * func_name, void * func_addr, long * parameters, int param_num, struct pt_regs * regs)     
{    
    DEBUG_PRINT("[+] Calling %s in target process.\n", func_name);    
    if (ptrace_call(target_pid, (ulong)func_addr, parameters, param_num, regs) == -1)    
        return -1;    
    
    if (ptrace_getregs(target_pid, regs) == -1)    
        return -1;    
    DEBUG_PRINT("[+] Target process returned from %s, return value=%x, pc=%x \n",     
            func_name, ptrace_retval(regs), ptrace_ip(regs));    
    return 0;    
}    
    
int inject_remote_process(pid_t target_pid, const char *library_path, 
		char *function_name, char *param, size_t param_size)    
{    
    int ret = -1;    
    void *mmap_addr, *dlopen_addr, *dlsym_addr, *dlclose_addr, *dlerror_addr;    
    void *local_handle, *remote_handle, *dlhandle;    
    uint8_t *map_base = 0;    
    uint8_t *dlopen_param1_ptr, *dlsym_param2_ptr, *saved_r0_pc_ptr, *inject_param_ptr, *remote_code_ptr, *local_code_ptr;    
    
    struct pt_regs regs, original_regs;
    extern ulong  _dlopen_addr_s, _dlopen_param1_s, _dlopen_param2_s, _dlsym_addr_s, \
        _dlsym_param2_s, _dlclose_addr_s, _inject_start_s, _inject_end_s, _inject_function_param_s, \
        _saved_cpsr_s, _saved_r0_pc_s;    
    
    uint32_t code_length;    
    long parameters[10];    
    
    DEBUG_PRINT("[+] Injecting process: %d\n", target_pid);    
    
    if (ptrace_attach(target_pid) == -1)    
        goto exit;    
    
    if (ptrace_getregs(target_pid, &regs) == -1)    
        goto exit;    
    
    /* save original registers */    
    memcpy(&original_regs, &regs, sizeof(regs));    
    
    mmap_addr = get_remote_addr(target_pid, libc_path, (ulong)mmap);    
    DEBUG_PRINT("[+] Remote mmap address: %x\n", mmap_addr);    
    
    /* call mmap */    
    parameters[0] = 0;  // addr    
    parameters[1] = 0x4000; // size    
    parameters[2] = PROT_READ | PROT_WRITE | PROT_EXEC;  // prot    
    parameters[3] =  MAP_ANONYMOUS | MAP_PRIVATE; // flags    
    parameters[4] = 0; //fd    
    parameters[5] = 0; //offset    
    
    if (ptrace_call_wrapper(target_pid, "mmap", mmap_addr, parameters, 6, &regs) == -1)    
        goto exit;    
    
    map_base = (uint8_t*)ptrace_retval(&regs);
    
    dlopen_addr  = get_remote_addr(target_pid, linker_path, (ulong)dlopen );    
    dlsym_addr   = get_remote_addr(target_pid, linker_path, (ulong)dlsym );    
    dlclose_addr = get_remote_addr(target_pid, linker_path, (ulong)dlclose );    
    dlerror_addr = get_remote_addr(target_pid, linker_path, (ulong)dlerror );    
    
    DEBUG_PRINT("[+] Get imports: \n\tdlopen: %p, \n\tdlsym: %p, \n\tdlclose: %p, \n\tdlerror: %p\n",    
            dlopen_addr, dlsym_addr, dlclose_addr, dlerror_addr);    
    
    printf("library path = %s\n", library_path);    
    ptrace_writedata(target_pid, map_base, library_path, strlen(library_path) + 1);    
    
    parameters[0] = (long)map_base;       
    parameters[1] = RTLD_NOW| RTLD_GLOBAL;     
    
    if (ptrace_call_wrapper(target_pid, "dlopen", dlopen_addr, parameters, 2, &regs) == -1)    
        goto exit;    
    
    void* sohandle = (void*)ptrace_retval(&regs);    
    
#define FUNCTION_NAME_ADDR_OFFSET       0x100    
    ptrace_writedata(target_pid, map_base + FUNCTION_NAME_ADDR_OFFSET, function_name, strlen(function_name) + 1);    
    parameters[0] = (long)sohandle;       
    parameters[1] = (long)(map_base + FUNCTION_NAME_ADDR_OFFSET);
    
    if (ptrace_call_wrapper(target_pid, "dlsym", dlsym_addr, parameters, 2, &regs) == -1)    
        goto exit;    
    
    void* hook_entry_addr = (void*)ptrace_retval(&regs);
    DEBUG_PRINT("hook_entry_addr = %p\n", hook_entry_addr);    
    
#define FUNCTION_PARAM_ADDR_OFFSET      0x200    
    ptrace_writedata(target_pid, map_base + FUNCTION_PARAM_ADDR_OFFSET, param, strlen(param) + 1);
    parameters[0] = (long)(map_base + FUNCTION_PARAM_ADDR_OFFSET);
  
    if (ptrace_call_wrapper(target_pid, "hook_entry", hook_entry_addr, parameters, 1, &regs) == -1)    
        goto exit;        
    
    //printf("Press enter to dlclose and detach\n");
    //getchar();
    parameters[0] = (long)sohandle;       
    
    if (ptrace_call_wrapper(target_pid, "dlclose", dlclose, parameters, 1, &regs) == -1)    
        goto exit;    
    
    /* restore */    
	printf("Restore\n");
    ptrace_setregs(target_pid, &original_regs);    
    ptrace_detach(target_pid);    
    ret = 0;    
    
exit:    
    return ret;    
}    

#ifndef GOLANG
void print_usage( const char *pname, int exit_code ){
    printf( "Usage: %s -p pid -l libpath\n", pname );
    printf( "    -h  --help      Display this usage information.\n"
            "    -p  --pid       PID of target process.\n"
            "    -l  --libpath   Absolute path of the shared library that will be injected.\n" );

    exit( exit_code );
}

int main( int argc, char** argv )
{
    int target_pid;
    char *libpath;

    const char *pname = strrchr( argv[0], '/' ) + 1;
    if (argc < 2)
        print_usage(pname, 1);

    int next_opt;
    const char *short_opts = "hp:l:";
    const struct option long_opts[] = {
        {"help",    0, NULL, 'h'},
        {"pid",     1, NULL, 'p'},
        {"libpath", 1, NULL, 'l'},
        {NULL,      0, NULL,  0 }
    };

    do
    {
        next_opt = getopt_long( argc, argv, short_opts, long_opts, NULL );
        switch ( next_opt )
        {
            case 'h':
                print_usage( pname, 0 );
            case 'p':
                target_pid = atoi( optarg );
                break;
            case 'l':
                libpath = optarg;
                break;
            case '?':
                printf("\n");
                print_usage( pname, 1 );
            case -1:
                break;
            default:
                ;
        }
    } while ( next_opt != -1 );

    char *param = "";

    inject_remote_process(target_pid, libpath, "hook_entry", param, strlen(param));

    return 0;
}
#endif
