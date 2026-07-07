all: kernel.c linker.ld kernel.asm
	yasm -f elf32 kernel.asm -o kernasm.o
	gcc -m32 -c kernel.c -o kernc.o -fno-stack-protector
	ld -m elf_i386 -T linker.ld -o kernel kernasm.o kernc.o

run-sdl:
	qemu-system-x86_64 -kernel kernel -display sdl

run-gtk:
	qemu-system-x86_64 -kernel kernel -display gtk

run-curses:
	qemu-system-x86_64 -kernel kernel -display curses
