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
   cmpl $0, %eax       # check to see if we're hit the end.
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
