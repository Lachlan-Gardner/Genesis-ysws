; From this website http://wiki.osdev.org/User:Zesterer/Bare_Bones
; The kernel_main is an external file, and will be kernel.c.
.extern kernel_main

; Says that it can be called from other programs.
; Needed by the linker.
.global start

; This is needed so that grub can actually detect the kernel and load it.

; This is what is used by grub to get the kernels location.
; It's magic, idk why it works. The internet just seems to say they're the magic numbers (flashbacks to year 11 programming. I got marked so far down because of magic numbers that the next year I just made a single constants file. It was 400 lines long. I was so paranoid I straight up put in the character count of a single space " ".)
.set MB_MAGIC, 0x1BADB002

; Tells grub to load page boundaries and make a memory map.
; Apparently this is useful later.
.set MB_FLAGS, (1 << 0) | (1 << 1)


.set MB_CHECKSUM, (0 - (MB_MAGIC + MB_FLAGS))

; Start the section where the multiboot part will be.
; I think this is to reserve a place in memory for the multibootpart of the program.
.section .multiboot
    ; Make sure the memory is aligned in bytes of 4.
	.align 4
    ; Allocates some memory for these numbers.
    ; A long is 4 bytes.
	; Use the previously calculated constants in executable code. I wish I could tell you why.
	.long MB_MAGIC
	.long MB_FLAGS
	; Use the checksum calculated earlier. I have no idea why.
	.long MB_CHECKSUM

; Seems like this is for reserving memory for things that haven't been initialised yet.
.section .bssr memory for
    ; Same as align 4, but 16 instead.
    ; Maybe because this tutorial is for x86.
    .align 16

    ; Seems like it's skipping along 4096 bytes for the stack.
    ; Is this RAM?
    stack_bottom:
        .skip 4096
    stack_top:

.section .text
    ; Where the code actually starts.
    ; I guess we've just been allocating the memory for the program until now.
    ; Does the dots at the start mean it isn't actual code?
    .start
        ; Starts the stack so the C code can actually run by moving the pointer to the top of the stack.
        ; The stack grows downward, like one of the spring loaded dispenser.
        mov $stack_top %esp

        ; Now the C code has a stack to use, the main function can be called.
        call kernel_main

        ; Once kernel_main exits it will call this block.
        hang:
            ; Disble cpu interrupts.
            ; Why is it cli? cpu something interrupts?
            cli
            ; Halt the cpu.
            hlt
            ; Goes back to hang.
            ; The comments on the tutorial say it only loops back around if the previous instructions failed.
            ; Is it because the cpu woudln't be able to carry out the jump instruction since it's already stopped?
            jmp hang