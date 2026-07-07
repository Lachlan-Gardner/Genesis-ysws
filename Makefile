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

iso:
# Requires a cross compiler and grub.
	i686-elf-gcc -T linker.ld -o genesis-os -ffreestanding -O2 -nostdlib kernasm.o kernc.o -lgcc
	mkdir -p isodir/boot/grub
	cp genesis-os isodir/boot/genesis-os
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o genesis-os.iso isodir
