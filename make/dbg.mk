TESTDIR   := $(BUILD_DIR)/mibench/network/dijkstra
TESTBIN   := $(TESTDIR)/rv32-x86-dijkstra-globals
TEST_ARGS := /mnt/ssd/riscv-sbt/mibench/network/dijkstra/input.dat

.PHONY: test
test: sbt
	cd $(TESTDIR) && \
	riscv-sbt -x -regs=globals -o rv32-x86-dijkstra-globals.bc rv32-dijkstra.o >/mnt/ssd/riscv-sbt/junk/rv32-x86-dijkstra-globals.log 2>&1 && \
	$(TESTBIN) $(TEST_ARGS)

test2: sbt
	make -C $(TOPDIR)/mibench dijkstra-clean dijkstra
	$(TESTBIN) $(TEST_ARGS)

.PHONY: dbg
dbg:
	gdb $(TESTBIN) core
