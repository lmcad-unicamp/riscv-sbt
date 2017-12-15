.PHONY: test
test: sbt
	$(MAKE) mibench-clean mibench MIBENCHS=crc32
	$(MAKE) -C mibench crc32-test

.PHONY: dbg
dbg:
	gdb /mnt/ssd/riscv-sbt/build/mibench/telecomm/CRC32/rv32-x86-crc32-globals $(TOPDIR)/mibench/core
