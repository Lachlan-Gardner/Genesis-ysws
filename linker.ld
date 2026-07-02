/* This is where the program will start. It's start.s. */
ENTRY(start)

/* Different parts of the executable. */
SECTIONS
{
    /* Starts the program memory at 1 Megabyte. Apparently anything below this is used by x86 stuff. */
    /* The dot is the current locations, so when saying .something it's saying it's at that location. I think it increments. */
    . = 1M;

    /* Multiboot part has to be near the start of the executable or else the bootloader can't find it. */
    /* The .rodata part means that it's read only data. It's here because it just doesn't need to modified, only found. */
    /* Aligns by 4 kilobytes. I think this is important when parcelling out memory so you don't end up with inconsistent blocks? */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        /* It's getting this from the assembly file. */
        *(.multiboot)
    }

    /* Executable code. */
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text)
    }

    /* Don't know what this is for. */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    /* The read and write data initialised. What does it do? */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    /* The uninitialised data and the stack. */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}