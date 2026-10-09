CC = gcc
LD = ld
ASM = nasm

CFLAGS = -m32 -ffreestanding -fno-pic -fno-stack-protector -Wall -Wextra -O2 -c
LDFLAGS = -m elf_i386 -T linker.ld

all: garfuld.iso

boot.o: boot.asm
	$(ASM) -f elf32 boot.asm -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

garfuld.elf: boot.o kernel.o
	$(LD) $(LDFLAGS) -o garfuld.elf boot.o kernel.o

iso: garfuld.elf
	mkdir -p iso/boot/grub
	cp garfuld.elf iso/boot/garfuld.elf
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o garfuld.iso iso

garfuld.iso: iso

clean:
	rm -rf *.o garfuld.elf garfuld.iso iso