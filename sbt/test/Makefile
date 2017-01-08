AS := riscv64-unknown-elf-as
LD := riscv64-unknown-elf-ld

X86_AS := as
X86_LD := ld
X86_SYSCALL_O := ../build/CMakeFiles/x86_syscall.dir/x86_syscall.s.o

all: hello x86hello rv-x86-hello test_x86_syscall

### RISC-V
# native rv hello world, in asm, using syscalls

hello.o: hello.s
	$(AS) -m32 -o $@ $<

hello: hello.o
	$(LD) -m elf32lriscv -o $@ $<

run:
	spike --isa=RV32G ../../toolchain/riscv32-unknown-elf/bin/pk hello

### X86
# native x86 hello world, in asm, using syscalls

x86hello.o: x86hello.s
	$(X86_AS) --32 -o $@ $<

x86hello: x86hello.o
	$(X86_LD) -m elf_i386 -o $@ $<

### RISC-V@X86
# translated rv object

rv-x86-hello: rv-x86-hello.o
	$(X86_LD) -m elf_i386 -o $@ $< $(X86_SYSCALL_O)

rv-x86-hello.o: rv-x86-hello.s
	$(X86_AS) --32 -o $@ $<

rv-x86-hello.s: rv-x86-hello.bc
	llc -O3 -o $@ -march x86 $<

rv-x86-hello.bc: hello.o
	riscv-sbt -o $@ $<


##
# x86_syscall test

test_x86_syscall: test_x86_syscall.o $(X86_SYSCALL_O)
	$(X86_LD) -m elf_i386 -o $@ $^

test_x86_syscall.o: test_x86_syscall.s
	$(X86_AS) --32 -o $@ $<

clean:
	rm -f hello.o hello x86hello.o x86hello \
		test_x86_syscall.o test_x86_syscall \
		rv-x86-hello*
