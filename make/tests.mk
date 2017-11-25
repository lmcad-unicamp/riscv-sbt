###
### TEST targets
###


lc:
	cat $(TOPDIR)/sbt/*.h $(TOPDIR)/sbt/*.cpp $(TOPDIR)/sbt/*.s | wc -l


SBT_TEST_DIR := $(TOPDIR)/sbt/test

.PHONY: tests
tests:
	rm -f log.txt
	$(LOG) $(MAKE) -C $(TOPDIR)/test clean all run
	$(LOG) $(MAKE) -C $(SBT_TEST_DIR) clean alltests-run

### rv32-system test

SBT_OUT_DIR := $(BUILD_DIR)/sbt/$(BUILD_TYPE_DIR)/test/

.PHONY: system-test
system-test:
	rm -f log.txt $(SBT_OUT_DIR)rv32-*system*
	sudo ./scripts/setmsr.sh
	$(LOG) $(MAKE) -C $(SBT_TEST_DIR) system-test

### QEMU tests

$(eval ADD_ASM_SRC_PREFIX =)
RV32TESTS_INCDIR   := $(TOPDIR)/riscv-qemu-tests
RV32TESTS_SRCDIR   := $(TOPDIR)/riscv-qemu-tests/rv32i/
RV32TESTS_DSTDIR   := $(BUILD_DIR)/riscv-qemu-tests/rv32i/

$(RV32TESTS_DSTDIR):
	mkdir -p $@

RV32_TESTS := \
    add addi and andi \
    aiupc \
    beq bge bgeu blt bltu bne \
    jal jalr \
    lui lb lbu lhu lw sb sw \
    or ori \
    sll slli slt slti sltiu sltu sra srai srl srli \
    sub xor xori
RV32_TESTS_FAILING :=
RV32_TESTS_MISSING := \
    csrrw csrrs csrrc csrrwi csrrsi csrrci \
    ecall ebreak fence fence.i lh sh

.PHONY: rv32tests_status
rv32tests_status:
	@echo passing: `echo $(RV32_TESTS) | wc -w`
	@echo $(RV32_TESTS)
	@echo failing: `echo $(RV32_TESTS_FAILING) | wc -w`
	@echo $(RV32_TESTS_FAILING)
	@echo missing: `echo $(RV32_TESTS_MISSING) | wc -w`
	@echo $(RV32_TESTS_MISSING)
	@echo total: \
		`echo $(RV32_TESTS) $(RV32_TESTS_FAILING) $(RV32_TESTS_MISSING) | wc -w`

$(foreach test,$(RV32_TESTS),\
$(eval $(call SBT_TEST,$(UTEST_NARCHS),$(RV32TESTS_SRCDIR),$(RV32TESTS_DSTDIR),\
$(test),$(test),$(NOLIBS),-I $(RV32TESTS_INCDIR),$(ASM),$(NOC))))

.PHONY: rv32tests
rv32tests: $(RV32_TESTS)

rv32tests-run: rv32tests $(addsuffix -test,$(RV32_TESTS))
	@echo "All rv32tests passed!"

rv32tests-clean:
	rm -rf $(RV32TESTS_DSTDIR)

rv32tests-clean-run: rv32tests-clean rv32tests-run

###
### matrix multiply test
###

TARGETS := rv32 x86 rv32-x86

mmm:
	MODES="globals locals" $(LOG) $(TOPDIR)/scripts/measure.py $(SBT_OUT_DIR) mm

.PHONY: mmtest
mmtest: sbt
	rm -f log.txt $(SBT_OUT_DIR)*-mm*
	$(LOG) $(MAKE) -C $(SBT_TEST_DIR) mm
	$(MAKE) mmm

