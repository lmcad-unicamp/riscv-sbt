### user config ###

COMPILER := clang
#COMPILER := gcc

### config ###

AS := riscv64-unknown-elf-as
LD := riscv64-unknown-elf-ld
GCC := riscv64-unknown-elf-gcc
CFLAGS := -Wall -Werror -O2
GCC_FLAGS := $(CFLAGS)
ifeq ($(COMPILER),clang)
CC := clang
CC_RISCV32 := --target=riscv32 -mriscv=RV32IAMFD
CC_RISCV64 := --target=riscv64 -mriscv=RV64IAMFD
else
CC := $(GCC)
CC_RISCV32 := -m32
CC_RISCV64 := -m64
endif

### rules ###

all: hello hello64 hellos

### 32-bit ###

hello.s: hello.c
	$(CC) $(CC_RISCV32) $(CFLAGS) -S $< -o $@

hello.o: hello.s
	$(GCC) -m32 $(GCC_FLAGS) -c -o $@ $<

hello: hello.o
	$(GCC) -m32 $(GCC_FLAGS) -o $@ $<

### 64-bit ###

hello64.s: hello.c
	$(CC) $(CC_RISCV64) $(CFLAGS) -S $< -o $@

hello64.o: hello64.s
	$(GCC) -m64 $(GCC_FLAGS) -c -o $@ $<

hello64: hello64.o
	$(GCC) -m64 $(GCC_FLAGS) -o $@ $<

### assembly 32-bit ###

hellos.o: myhello.s
	$(AS) -m32 -o$@ $<

hellos: hellos.o
	$(LD) -m elf32lriscv -o$@ $<

### run ###

run: hello
	spike --isa=RV32G ../toolchain/riscv32-unknown-elf/bin/pk hello

run64: hello64
	spike pk hello64

runs: hellos
	spike --isa=RV32G ../toolchain/riscv32-unknown-elf/bin/pk hellos

### clean ###

clean:
	rm -f \
		hello.s hello.o hello \
		hello64.s hello64.o hello64 \
		hellos hellos.o
