.PHONY: test
test:
	rm -f log.txt sbt.log
	clang --target=riscv32 -isysroot /mnt/data/riscv-sbt/toolchain/release/opt/riscv/riscv32-unknown-elf -isystem /mnt/data/riscv-sbt/toolchain/release/opt/riscv/riscv32-unknown-elf/include  -emit-llvm -c -O3 -mllvm -disable-llvm-optzns /mnt/data/riscv-sbt/sbt/test/mm.c -o /mnt/data/riscv-sbt/build/sbt/debug/test/rv32-mm.bc
	llvm-dis /mnt/data/riscv-sbt/build/sbt/debug/test/rv32-mm.bc -o /mnt/data/riscv-sbt/build/sbt/debug/test/rv32-mm.ll
	opt -O3  /mnt/data/riscv-sbt/build/sbt/debug/test/rv32-mm.bc -o /mnt/data/riscv-sbt/build/sbt/debug/test/rv32-mm.opt.bc
	llvm-dis /mnt/data/riscv-sbt/build/sbt/debug/test/rv32-mm.opt.bc -o /mnt/data/riscv-sbt/build/sbt/debug/test/rv32-mm.opt.ll
	llc -relocation-model=static -O3  -march=riscv32 -mattr=+m /mnt/data/riscv-sbt/build/sbt/debug/test/rv32-mm.opt.bc -o /mnt/data/riscv-sbt/build/sbt/debug/test/rv32-mm.s
	riscv32-unknown-elf-as -o /mnt/data/riscv-sbt/build/sbt/debug/test/rv32-mm.o -c /mnt/data/riscv-sbt/build/sbt/debug/test/rv32-mm.s
	head /mnt/data/riscv-sbt/build/sbt/debug/test/rv32-mm.s
	riscv32-unknown-elf-objdump -S /mnt/data/riscv-sbt/build/sbt/debug/test/rv32-mm.o | head -n15

.PHONY: dbg
dbg:
	gdb $(TESTBIN)
