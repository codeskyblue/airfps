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

有偏移地址(Off)和大小(Size)就可以直接看数据了。 

比如这个.data段(start: 0x70, size: 56(0x38))：`hexdump -s 0x70 -n 56 -vC max.o`

	00000070  03 00 00 00 43 00 00 00  22 00 00 00 de 00 00 00  |....C...".......|
	00000080  2d 00 00 00 4b 00 00 00  36 00 00 00 22 00 00 00  |-...K...6..."...|
	00000090  2c 00 00 00 21 00 00 00  16 00 00 00 0b 00 00 00  |,...!...........|
	000000a0  42 00 00 00 00 00 00 00                           |B.......|
	000000a8
