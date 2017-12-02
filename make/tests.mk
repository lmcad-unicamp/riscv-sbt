###
### TEST targets
###


lc:
	cat $(TOPDIR)/sbt/*.h $(TOPDIR)/sbt/*.cpp $(TOPDIR)/sbt/*.s | wc -l


SBT_TEST_DIR := $(TOPDIR)/sbt/test

.PHONY: tests
tests:
	$(MAKE) -C $(TOPDIR)/test clean all run
	$(MAKE) -C $(SBT_TEST_DIR) clean alltests-run

### rv32-system test

SBT_OUT_DIR := $(BUILD_DIR)/sbt/$(BUILD_TYPE_DIR)/test/

.PHONY: system-test
system-test:
	rm -f $(SBT_OUT_DIR)rv32-*system*
	sudo ./scripts/setmsr.sh
	$(MAKE) -C $(SBT_TEST_DIR) system system-test

### QEMU tests

RV32TESTS_INCDIR   := $(TOPDIR)/riscv-qemu-tests
RV32TESTS_SRCDIR   := $(TOPDIR)/riscv-qemu-tests/rv32i
RV32TESTS_DSTDIR   := $(BUILD_DIR)/riscv-qemu-tests/rv32i

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

RVTNARCHS := rv32

$(foreach test,$(RV32_TESTS),$(eval \
$(call UTEST,$(RVTNARCHS),$(test),$(RV32TESTS_SRCDIR),$(RV32TESTS_DSTDIR),\
--asm -C,--sflags="-I $(RV32TESTS_INCDIR)",--save-output)))

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

mmm:
	$(MEASURE_PY) $(SBT_OUT_DIR) mm

.PHONY: mmtest
mmtest: sbt
	rm -f $(SBT_OUT_DIR)*-mm*
	$(MAKE) -C $(SBT_TEST_DIR) mm
	$(MAKE) mmm


### everything ###

.PHONY: alltests
alltests: tests system-test rv32tests-clean-run mmm
