.PHONY: test
test:
	rm -f log.txt sbt.log
	clang --target=riscv32 -isysroot $(TCR)/opt/riscv/riscv32-unknown-elf -isystem $(TCR)/opt/riscv/riscv32-unknown-elf/include  -emit-llvm -c -O3 -mllvm -disable-llvm-optzns $(TOPDIR)/sbt/test/mm.c -o $(BUILD_DIR)/sbt/debug/test/rv32-mm.bc
	llvm-dis $(BUILD_DIR)/sbt/debug/test/rv32-mm.bc -o $(BUILD_DIR)/sbt/debug/test/rv32-mm.ll
	opt -O3  $(BUILD_DIR)/sbt/debug/test/rv32-mm.bc -o $(BUILD_DIR)/sbt/debug/test/rv32-mm.opt.bc
	llvm-dis $(BUILD_DIR)/sbt/debug/test/rv32-mm.opt.bc -o $(BUILD_DIR)/sbt/debug/test/rv32-mm.opt.ll
	llc -relocation-model=static -O3  -march=riscv32 -mattr=+m $(BUILD_DIR)/sbt/debug/test/rv32-mm.opt.bc -o $(BUILD_DIR)/sbt/debug/test/rv32-mm.s
	sed -i "1i.option norelax" $(BUILD_DIR)/sbt/debug/test/rv32-mm.s
	riscv32-unknown-elf-as -o $(BUILD_DIR)/sbt/debug/test/rv32-mm.o -c $(BUILD_DIR)/sbt/debug/test/rv32-mm.s
	head $(BUILD_DIR)/sbt/debug/test/rv32-mm.s
	riscv32-unknown-elf-objdump -S $(BUILD_DIR)/sbt/debug/test/rv32-mm.o | head -n15

.PHONY: dbg
dbg:
	gdb $(TESTBIN)
