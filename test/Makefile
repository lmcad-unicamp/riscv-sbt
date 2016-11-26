AS := riscv64-unknown-elf-as
LD := riscv64-unknown-elf-ld
CC := riscv64-unknown-elf-gcc
CFLAGS := -Wall -Werror -O2

all: hello hello64 hellos

hello.o: hello.c
	$(CC) -m32 -c $(CFLAGS) -o$@ $<

hello: hello.o
	$(CC) -m32 -o$@ $<

hello64.o: hello.c
	$(CC) -c $(CFLAGS) -o$@ $<

hello64: hello64.o
	$(CC) -o$@ $<

hellos.o: hello.s
	$(AS) -m32 -o$@ $<

hellos: hellos.o
	$(LD) -m elf32lriscv -o$@ $<

run: hello
	spike --isa=RV32G ../toolchain/riscv32-unknown-elf/bin/pk hello

run64: hello64
	spike pk hello64

runs: hellos
	spike --isa=RV32G ../toolchain/riscv32-unknown-elf/bin/pk hellos

clean:
	rm -f hello hello.o hello64 hello64.o hellos hellos.o
