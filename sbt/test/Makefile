RV_AS := riscv32-unknown-elf-as
RV_LD := riscv32-unknown-elf-ld

X86_AS := as
X86_LD := ld
X86_SYSCALL_O := $(TOPDIR)/build/sbt/CMakeFiles/x86_syscall.dir/x86_syscall.s.o

all: \
	rv-hello \
	x86-hello \
	rv-x86-hello \
	test_x86_syscall \
	#x86-main \
	rv-main \
	rv-x86-main.bc

### RISC-V
# native rv hello world, in asm, using syscalls

rv-hello.o: rv-hello.s
	$(RV_AS) -o $@ $<

rv-hello: rv-hello.o
	$(RV_LD) -m elf32lriscv -o $@ $<

run-rv-hello: rv-hello
	spike --isa=RV32G ../../toolchain/riscv32-unknown-elf/bin/pk rv-hello

### X86
# native x86 hello world, in asm, using syscalls

x86-hello.o: x86-hello.s
	$(X86_AS) --32 -o $@ $<

x86-hello: x86-hello.o
	$(X86_LD) -m elf_i386 -o $@ $<

### RISC-V@X86
# translated rv object

rv-x86-hello: rv-x86-hello.o
	$(X86_LD) -m elf_i386 -o $@ $< $(X86_SYSCALL_O)

rv-x86-hello.o: rv-x86-hello.s
	$(X86_AS) --32 -o $@ $<

rv-x86-hello.s: rv-x86-hello.bc
	llc -O3 -o $@ -march x86 $<

rv-x86-hello.bc: rv-hello.o
	riscv-sbt -o $@ $<
#	gdb --args riscv-sbt -o $@ $<


##
# x86_syscall test

test_x86_syscall: test_x86_syscall.o $(X86_SYSCALL_O)
	$(X86_LD) -m elf_i386 -o $@ $^

test_x86_syscall.o: test_x86_syscall.s
	$(X86_AS) --32 -o $@ $<

###
### main
###

CC := clang
CFLAGS := -Wall -Werror -O2

X86_DYNLINKER := -dynamic-linker /lib32/ld-linux.so.2
X86_CRT1 := /usr/lib32/crt1.o
X86_CRTI := /usr/lib32/crti.o
X86_CRTBEGIN := /usr/lib/gcc/x86_64-linux-gnu/4.9/32/crtbegin.o
X86_CRTEND := /usr/lib/gcc/x86_64-linux-gnu/4.9/32/crtend.o
X86_CRTN := /usr/lib32/crtn.o


RV_SYSROOT := $(TOPDIR)/toolchain/riscv64-unknown-elf
RV_LIB32 := $(RV_SYSROOT)/lib/32
RV_LIBGCC32 := $(TOPDIR)/toolchain/lib/gcc/riscv64-unknown-elf/6.1.0/32

RV_SYSROOT_FLAG := -isysroot $(RV_SYSROOT) -isystem $(RV_SYSROOT)/include

RV_CRTI :=     $(RV_LIBGCC32)/crti.o
RV_CRTBEGIN := $(RV_LIBGCC32)/crtbegin.o
RV_CRTEND :=   $(RV_LIBGCC32)/crtend.o
RV_CRTN :=     $(RV_LIBGCC32)/crtn.o
RV_LGCC :=     $(RV_LIBGCC32)/libgcc.a

RV_CRT0 :=     $(RV_LIB32)/crt0.o
RV_LDS :=      $(RV_LIB32)/riscv.ld
RV_LC :=       $(RV_LIB32)/libc.a
RV_LG :=       $(RV_LIB32)/libg.a
RV_LGLOSS :=   $(RV_LIB32)/libgloss.a
RV_LNOSYS :=   $(RV_LIB32)/libnosys.a


x86-main.s: main.c
	$(CC) -m32 $(CFLAGS) -S -o$@ $<

x86-main.o: x86-main.s
	$(X86_AS) --32 -o$@ $<

x86-main: x86-main.o
	$(X86_LD) -m elf_i386 -o$@ $(X86_DYNLINKER) $(X86_CRT1) $(X86_CRTI) $< -lc $(X86_CRTN)

rv-main.s: main.c
	$(CC) --target=riscv32 -mriscv=RV32IAMFD $(X86_SYSROOT_FLAG) $(CFLAGS) -S -o$@ $<

rv-main.o: rv-main.s
	$(RV_AS) -m32 -o $@ $<

rv-main: rv-main.o
	$(RV_LD) \
		-L$(RV_LIB32) -L$(RV_LIBGCC32) \
		-dT riscv.ld -m elf32lriscv -o$@ \
		$(RV_CRT0) \
		$(RV_CRTBEGIN) \
		$< \
		$(RV_CRTEND)

run-rv-main: rv-main
	spike --isa=RV32G ../../toolchain/riscv32-unknown-elf/bin/pk $<

rv-x86-main.bc: rv-main.o
	riscv-sbt -o $@ $<


CXXFLAGS := -Wall -Wextra -Werror -O0 -g -std=c++11 -pedantic

elf.o: elf.cpp
	$(CXX) -m32 $(CXXFLAGS) -o $@ -c $<

elf: elf.o
	$(CXX) -m32 $(CXXFLAGS) -o $@ $<

clean:
	rm -f \
		elf \
		rv-hello.o rv-hello \
		x86-hello.o x86-hello \
		test_x86_syscall.o test_x86_syscall \
		rv-x86-hello* \
		x86-main.s x86-main.o x86-main \
		rv-main.s rv-main.o rv-main
