.PHONY: test
test:
	make -C $(TOPDIR)/sbt/test clean rv32-x86-mm
	$(BUILD_DIR)/sbt/debug/test/rv32-x86-mm

.PHONY: dbg
dbg:
	gdb $(TESTBIN)
