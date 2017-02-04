### user config ###

#COMPILER := clang
#COMPILER := gcc
#SYSROOT := $(TOPDIR)/toolchain/riscv64-unknown-elf
#SYSROOT_FLAG := -isysroot $(SYSROOT) -isystem $(SYSROOT)/include

### config ###

#AS := riscv64-unknown-elf-as
#LD := riscv64-unknown-elf-ld
#GCC := riscv64-unknown-elf-gcc
#CFLAGS := -Wall -Werror -O2
#GCC_FLAGS := $(CFLAGS)
#ifeq ($(COMPILER),clang)
#CC := clang
#CC_RISCV32 := --target=riscv32 -mriscv=RV32IAMFD $(SYSROOT_FLAG)
#CC_RISCV64 := --target=riscv64 -mriscv=RV64IAMFD $(SYSROOT_FLAG)
#else
#CC := $(GCC)
#CC_RISCV32 := -m32
#CC_RISCV64 := -m64
#endif

### config ###

CFLAGS := -Wall -Werror -O2

### rules ###

all: hello hello64 hellos

###
### test gcc
###

### 32-bit ###

AS32 := riscv32-unknown-elf-as
LD32 := riscv32-unknown-elf-ld
GCC32 := riscv32-unknown-elf-gcc

hello.s: hello.c
	$(GCC32) $(CFLAGS) -S $< -o $@

hello.o: hello.s
	$(GCC32) $(CFLAGS) -c -o $@ $<

hello: hello.o
	$(GCC32) $(CFLAGS) -o $@ $<

run: hello
	spike $(TOPDIR)/toolchain/riscv32-unknown-elf/bin/pk hello

### 64-bit ###

AS64 := riscv64-unknown-elf-as
LD64 := riscv64-unknown-elf-ld
GCC64 := riscv64-unknown-elf-gcc

hello64.s: hello.c
	$(GCC64) $(CFLAGS) -S $< -o $@

hello64.o: hello64.s
	$(GCC64) $(CFLAGS) -c -o $@ $<

hello64: hello64.o
	$(GCC64) $(CFLAGS) -o $@ $<

run64: hello64
	spike --isa=RV64IMAFDC pk hello64

### assembly 32-bit ###

hellos.o: myhello.s
	$(AS32) -o$@ $<

hellos: hellos.o
	$(LD32) -o$@ $<

runs: hellos
	spike $(TOPDIR)/toolchain/riscv32-unknown-elf/bin/pk hellos

### clean ###

clean:
	rm -f \
		hello.s hello.o hello \
		hello64.s hello64.o hello64 \
		hellos hellos.o
