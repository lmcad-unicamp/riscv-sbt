AS := riscv64-unknown-elf-as
LD := riscv64-unknown-elf-ld

X86_AS := as
X86_LD := ld

all: hello x86hello test_x86_syscall

### RISC-V

hello.o: hello.s
	$(AS) -m32 -o $@ $<

hello: hello.o
	$(LD) -m elf32lriscv -o $@ $<

run:
	spike --isa=RV32G ../../toolchain/riscv32-unknown-elf/bin/pk hello

### X86

x86hello.o: x86hello.s
	$(X86_AS) --32 -o $@ $<

x86hello: x86hello.o
	$(X86_LD) -m elf_i386 -o $@ $<

test_x86_syscall: test_x86_syscall.o ../build/CMakeFiles/x86_syscall.dir/x86_syscall.s.o
	$(X86_LD) -m elf_i386 -o $@ $^

test_x86_syscall.o: test_x86_syscall.s
	$(X86_AS) --32 -o $@ $<

clean:
	rm -f hello.o hello x86hello.o x86hello \
		test_x86_syscall.o test_x86_syscall
