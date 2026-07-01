// The kernel_main is an external file, and will be kernel.c.
.extern kernel_main

// Says that it can be called from other programs.
// Needed by the linker.
.global start

// This is needed so that grub can actually detect the kernel and load it.

// This is what is used by grub to get the kernels location.
// It's magic, idk why it works. The internet just seems to say they're the magic numbers (flashbacks to year 11 programming. I got marked so far down because of magic numbers that the next year I just made a single constants file. It was 400 lines long. I was so paranoid I straight up put in the character count of a single space " ".)
.set MB_MAGIC, 0x1BADB002

// Tells grub to load page boundaries and 
.set MB_FLAGS, (1 << 0) | (1 << 1)