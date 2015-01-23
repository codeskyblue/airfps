ELF学习笔记
================

多谢 <http://blog.163.com/tank_ge/blog/static/214396955201371183456907>

ELF分为3中类型

1. Relocatable  下文的max.o就是这种类型
2. Executable   可执行文件
3. ShareObject  共享文件

先准备点汇编代码 [max.s]

	# purpose: this program finds the maximum number of a set of data items.
	# 
	# variables: the registers have the following uses:
	#        %eax-current data item
	#        %ebx-largest data item found
	#        %edi-holds the index of the data item being examined
	#
	# the following memory locations are used:
	#    data_item-contains the data item. a 0 is used to terminate the data.
	.section .data
	data_items:           # these are the data_items
	.long 3, 67, 34, 222, 45, 75, 54, 34, 44, 33, 22, 11, 66, 0

	.section .text
	.globl _start
	_start:
	   movl $0, %edi       # move 0 into the index
	   movl data_items(, %edi, 4), %eax
	   movl %eax, %ebx
	start_loop:
	   cmpl $0, %eax       # check to see if we are hit the end.
	   je loop_exit
	   incl %edi
	   movl data_items(, %edi, 4), %eax
	   cmpl %ebx, %eax
	   jle start_loop
	   movl %eax, %ebx
	   jmp start_loop
	loop_exit:
	   movl $1, %eax       # 1 is the _exit() syscall 
	   int $0x80

一个求最大值的汇编程序，代码段，数据段区分的很清晰

	as -o max.o max.s
	ld -o max max.o
	./max
	echo $?  # 结果应该是222

ELF文件的结构是

	+----------------------+
	| elf header           |
	+----------------------+
	| ....                 +
	+----------------------+
	| program header table |
	+----------------------+
	|.........             |
	+----------------------+
	| section header table |
	+----------------------+
	|...                   |

program header table和section header table的位置由elf header中描述的确定

用`readelf`这个工具先解析下 `readelf --file-header max.o`

	ELF Header:
	  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 
	  Class:                             ELF64
	  Data:                              2nd complement, little endian
	  Version:                           1 (current)
	  OS/ABI:                            UNIX - System V
	  ABI Version:                       0
	  Type:                              REL (Relocatable file)
	  Machine:                           Advanced Micro Devices X86-64
	  Version:                           0x1
	  Entry point address:               0x0
	  Start of program headers:          0 (bytes into file)
	  Start of section headers:          224 (bytes into file)
	  Flags:                             0x0
	  Size of this header:               64 (bytes)
	  Size of program headers:           0 (bytes)
	  Number of program headers:         0
	  Size of section headers:           64 (bytes)
	  Number of section headers:         8
	  Section header string table index: 5

1. 还有个Magic，不知道有什么用。算了，无所谓
2. Program header - start: 0x0, size: 0x0
3. Section header - start: 224(0x14), size: 64(0x40)

知道这些应该就差不多了，program header是空的，忽略掉

然后看看Section有什么 `readelf --sections --wide max.o`

	There are 8 section headers, starting at offset 0xe0:

	Section Headers:
	  [Nr] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
	  [ 0]                   NULL            0000000000000000 000000 000000 00      0   0  0
	  [ 1] .text             PROGBITS        0000000000000000 000040 00002d 00  AX  0   0  4
	  [ 2] .rela.text        RELA            0000000000000000 0003c8 000030 18      6   1  8
	  [ 3] .data             PROGBITS        0000000000000000 000070 000038 00  WA  0   0  4
	  [ 4] .bss              NOBITS          0000000000000000 0000a8 000000 00  WA  0   0  4
	  [ 5] .shstrtab         STRTAB          0000000000000000 0000a8 000031 00      0   0  1
	  [ 6] .symtab           SYMTAB          0000000000000000 0002e0 0000c0 18      7   7  8
	  [ 7] .strtab           STRTAB          0000000000000000 0003a0 000028 00      0   0  1
	Key to Flags:
	  W (write), A (alloc), X (execute), M (merge), S (strings), l (large)
	  I (info), L (link order), G (group), T (TLS), E (exclude), x (unknown)
	  O (extra OS processing required) o (OS specific), p (processor specific)

除了.data和.text是代码里有的，其他的都是as这个汇编器自动生成的。

Sections Headers[0] 竟然是空的，好奇怪，也许是历史遗留的问题. 

用hexdump可以看看程序里面是个什么样子，头部大致贴出来，有部分被我省去

	00000000  7f 45 4c 46 02 01 01 00  00 00 00 00 00 00 00 00  |.ELF............|
	00000010  01 00 3e 00 01 00 00 00  00 00 00 00 00 00 00 00  |..>.............|
	00000020  00 00 00 00 00 00 00 00  e0 00 00 00 00 00 00 00  |................|
	00000030  00 00 00 00 40 00 00 00  00 00 40 00 08 00 05 00  |....@.....@.....|
	00000040  bf 00 00 00 00 67 8b 04  bd 00 00 00 00 89 c3 83  |.....g..........|
	00000050  f8 00 74 12 ff c7 67 8b  04 bd 00 00 00 00 39 d8  |..t...g.......9.|
	00000060  7e ed 89 c3 eb e9 b8 01  00 00 00 cd 80 00 00 00  |~...............|
	00000070  03 00 00 00 43 00 00 00  22 00 00 00 de 00 00 00  |....C...".......|
	00000080  2d 00 00 00 4b 00 00 00  36 00 00 00 22 00 00 00  |-...K...6..."...|
	00000090  2c 00 00 00 21 00 00 00  16 00 00 00 0b 00 00 00  |,...!...........|
	000000a0  42 00 00 00 00 00 00 00  00 2e 73 79 6d 74 61 62  |B.........symtab|
	000000b0  00 2e 73 74 72 74 61 62  00 2e 73 68 73 74 72 74  |..strtab..shstrt|
	000000c0  61 62 00 2e 72 65 6c 61  2e 74 65 78 74 00 2e 64  |ab..rela.text..d|
	000000d0  61 74 61 00 2e 62 73 73  00 00 00 00 00 00 00 00  |ata..bss........|
	000000e0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
	*
	.......... 忽略部分
	00000380  00 00 00 00 00 00 00 00  21 00 00 00 10 00 01 00  |........!.......|
	00000390  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
	000003a0  00 64 61 74 61 5f 69 74  65 6d 73 00 73 74 61 72  |.data_items.star|
	000003b0  74 5f 6c 6f 6f 70 00 6c  6f 6f 70 5f 65 78 69 74  |t_loop.loop_exit|
	000003c0  00 5f 73 74 61 72 74 00  09 00 00 00 00 00 00 00  |._start.........|
	000003d0  0b 00 00 00 02 00 00 00  00 00 00 00 00 00 00 00  |................|
	000003e0  1a 00 00 00 00 00 00 00  0b 00 00 00 02 00 00 00  |................|
	000003f0  00 00 00 00 00 00 00 00                           |........|
	000003f8

有偏移地址(Off)和大小(Size)就可以直接看数据了。 

比如这个.data段(start: 0x70, size: 56(0x38))：`hexdump -s 0x70 -n 56 -vC max.o`

	00000070  03 00 00 00 43 00 00 00  22 00 00 00 de 00 00 00  |....C...".......|
	00000080  2d 00 00 00 4b 00 00 00  36 00 00 00 22 00 00 00  |-...K...6..."...|
	00000090  2c 00 00 00 21 00 00 00  16 00 00 00 0b 00 00 00  |,...!...........|
	000000a0  42 00 00 00 00 00 00 00                           |B.......|
	000000a8

汇编的代码是这样的

	.section .data
	data_items:           # these are the data_items
	.long 3, 67, 34, 222, 45, 75, 54, 34, 44, 33, 22, 11, 66, 0

因为是LittleEndian的嘛，所以其中0x00000003 对应 03 00 00 00

其他段的意思分别是

1. .text: 放代码的地方,一般都是只读
2. .bss: 用来存放全局未初始化的变量
3. .data: 存放全局初始化的变量
4. .stack: 栈区。 由编译器自动分配释放, 可以搞搞溢出攻击
5. .heap: 堆区, malloc, new之类的用到的

.text可以用objdump这个工具反汇编 `objdump -d max.o`. 看代码跟原来的代码，很像

	tmp/max.o:     file format elf64-x86-64
	Disassembly of section .text:

	0000000000000000 <_start>:
	   0:	bf 00 00 00 00       	mov    $0x0,%edi
	   5:	67 8b 04 bd 00 00 00 	mov    0x0(,%edi,4),%eax
	   c:	00 
	   d:	89 c3                	mov    %eax,%ebx

	000000000000000f <start_loop>:
	   f:	83 f8 00             	cmp    $0x0,%eax
	  12:	74 12                	je     26 <loop_exit>
	  14:	ff c7                	inc    %edi
	  16:	67 8b 04 bd 00 00 00 	mov    0x0(,%edi,4),%eax
	  1d:	00 
	  1e:	39 d8                	cmp    %ebx,%eax
	  20:	7e ed                	jle    f <start_loop>
	  22:	89 c3                	mov    %eax,%ebx
	  24:	eb e9                	jmp    f <start_loop>

	0000000000000026 <loop_exit>:
	  26:	b8 01 00 00 00       	mov    $0x1,%eax
	  2b:	cd 80                	int    $0x80

了解了大概的头部结构，然后用C语言写出一个读取elf header的代码

	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <elf.h>
	#include <unistd.h>
	#include <fcntl.h>

	#define LOGD(fmt, args...) {printf("[DEUBG] "); printf(fmt, ##args); printf("\n");}
	#define LOGE(fmt, args...) {printf("[ERROR] "); printf(fmt, ##args); printf("\n"); exit(1);}

	#define ELFHDR Elf64_Ehdr

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

	int main(){
		FILE* fp;
		fp = fopen("max.o", "rb");
		if (fp == NULL){
			LOGE("file(max.o) open error");
		}
		ELFHDR ehdr;
		printf("size ehdr: %d\n", sizeof(ehdr));
		int n = fread(&ehdr, 1, sizeof(ehdr), fp);
		if (n != sizeof(ehdr)){
			LOGE("read error in %s at line %d", __FILE__, __LINE__);
		}
		fclose(fp);
		printf("section header offset: %d\n", ehdr.e_shoff);
		printf("section header string table index: %d\n", ehdr.e_shstrndx);
		printf("section entry size: %d\n", ehdr.e_shentsize);
		printf("flags: 0x%x\n", ehdr.e_flags);
		printf("version: %d\n", ehdr.e_version);
		printf("type: %d\n", ehdr.e_type);
		return 0;
	}

为了能兼容更多平台，ELFHDR可以这么定义

	#if defined(__amd64__)
	# define ELFHDR Elf64_Ehdr
	#elif defined(__i386__)
	# define ELFHDR Elf32_Ehdr
	#elif defined(__arm__)
	# define ELFHDR Elf32_Ehdr
	#else
	# error "Not supported"
	#endif

代码我放在了[myreadelf.c](study-diary/myreadelf.c)中
