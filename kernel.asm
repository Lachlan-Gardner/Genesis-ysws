; Says to make the code to run in a 32 bit computer.
bits 32

section .text
;multiboot stuff, same as always
align 4
dd 0x1BADB002 ;magic
dd 0x00 ;flags
dd - (0x1BADB002 + 0x00) ;checksum

global start ;to set symbols from source code as global 
extern kernel_main ;kernal_main is main function in the kernel.c.

start:
cli ;clear interrupts. To diable interrupts
mov esp, stack_space ;set stack pointer
call kernel_main ;calls the main kernel function from c file
hlt ;halts the CPU

section .bss
resb 8192 ;8KB memory reserved for the stack
stack_space: