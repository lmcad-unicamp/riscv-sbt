### config ###

CFLAGS := -Wall -Werror -O2

### rules ###

all: gcc-hello-32 gcc-hello-64 asm-hello-32 clang-hello-32

###
### test gcc
###

### 32-bit ###

AS32 := riscv32-unknown-elf-as
LD32 := riscv32-unknown-elf-ld
GCC32 := riscv32-unknown-elf-gcc

gcc-hello-32.s: hello.c
	$(GCC32) $(CFLAGS) -o $@ -S $<

gcc-hello-32.o: gcc-hello-32.s
	$(GCC32) $(CFLAGS) -o $@ -c $<

gcc-hello-32: gcc-hello-32.o
	$(GCC32) $(CFLAGS) -o $@ $<

run-gcc-hello-32: gcc-hello-32
	spike $(TOPDIR)/toolchain/riscv32-unknown-elf/bin/pk gcc-hello-32

### 64-bit ###

AS64 := riscv64-unknown-elf-as
LD64 := riscv64-unknown-elf-ld
GCC64 := riscv64-unknown-elf-gcc

gcc-hello-64.s: hello.c
	$(GCC64) $(CFLAGS) -o $@ -S $<

gcc-hello-64.o: gcc-hello-64.s
	$(GCC64) $(CFLAGS) -o $@ -c $<

gcc-hello-64: gcc-hello-64.o
	$(GCC64) $(CFLAGS) -o $@ $<

run-gcc-hello-64: gcc-hello-64
	spike --isa=RV64IMAFDC pk gcc-hello-64

### assembly 32-bit ###

asm-hello-32.o: hello.s
	$(AS32) -o $@ $<

asm-hello-32: asm-hello-32.o
	$(LD32) -o $@ $<

run-asm-hello-32: asm-hello-32
	spike $(TOPDIR)/toolchain/riscv32-unknown-elf/bin/pk asm-hello-32

###
### test clang
###

### clean ###

CLANG := clang
SYSROOT := $(TOPDIR)/toolchain/riscv32-unknown-elf
SYSROOT_FLAG := -isysroot $(SYSROOT) -isystem $(SYSROOT)/include
CLANG_FLAGS := --target=riscv32 -mriscv=RV32IAMFD $(SYSROOT_FLAG)
TOOLCHAIN := $(TOPDIR)/toolchain
LIB := $(TOOLCHAIN)/riscv32-unknown-elf/lib
CRT0 := $(LIB)/crt0.o

### 32-bit ###

clang-hello-32.s: hello.c
	$(CLANG) $(CLANG_FLAGS) $(CFLAGS) -o $@ -S $<

clang-hello-32.o: clang-hello-32.s
	$(AS32) -o $@ $<

clang-hello-32: clang-hello-32.o
	$(LD32) -o $@ -dT ldscripts/elf32lriscv.x $(CRT0) $< -lc -lgloss -lc

run-clang-hello-32: clang-hello-32
	spike $(TOPDIR)/toolchain/riscv32-unknown-elf/bin/pk clang-hello-32

run: run-gcc-hello-32 run-gcc-hello-64 run-asm-hello-32 run-clang-hello-32
	@echo Success!

clean:
	rm -f \
		gcc-hello-32* gcc-hello-64* asm-hello-32* clang-hello-32*
