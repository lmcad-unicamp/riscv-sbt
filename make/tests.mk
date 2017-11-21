###
### TEST targets
###


lc:
	cat $(TOPDIR)/sbt/*.h $(TOPDIR)/sbt/*.cpp $(TOPDIR)/sbt/*.s | wc -l


SBT_TEST_DIR := $(TOPDIR)/sbt/test

.PHONY: tests
tests: sbt-debug
	rm -f log.txt
	$(LOG) $(MAKE) -C $(TOPDIR)/test clean all run
	$(LOG) $(MAKE) -C $(SBT_TEST_DIR) clean elf run-alltests

### rv32-system test

SBT_OUT_DIR := $(BUILD_DIR)/sbt/$(BUILD_TYPE_DIR)/test/

.PHONY: test-system
test-system: sbt
	rm -f log.txt $(SBT_OUT_DIR)rv32-*system*
	sudo ./scripts/setmsr.sh
	$(LOG) $(MAKE) -C $(SBT_TEST_DIR) test-system

### QEMU tests

.PHONY: rv32tests
rv32tests: qemu-tests-reset $(QEMU_TESTS_TOOLCHAIN) sbt
	rm -f log.txt
	$(MAKE) -C $(QEMU_TESTS_BUILD)/rv32i clean all
	$(LOG) $(MAKE) -C $(SBT_TEST_DIR) rv32tests

###
### matrix multiply test
###

TARGETS := rv32 x86 rv32-x86

mmm:
	$(LOG) $(TOPDIR)/scripts/measure.py $(SBT_OUT_DIR) mm

.PHONY: mmtest
mmtest: sbt
	rm -f log.txt $(SBT_OUT_DIR)*-mm*
	$(LOG) $(MAKE) -C $(SBT_TEST_DIR) mm
	$(MAKE) mmm

