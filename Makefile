all: kernel.c linker.ld kernel.asm
	yasm -f elf32 kernel.asm -o kernasm.o
	gcc -m32 -c kernel.c -o kernc.o
	ld -m elf_i386 -T linker.ld -o kernel kernasm.o kernc.o

clean:
	rm kernc.o
	rm kernasm.o
	rm kernel

run:
	qemu-system-x86_64 -kernel kernel -display gtk
