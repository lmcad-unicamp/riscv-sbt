include $(TOPDIR)/make/config.mk

### config ####

SRC_DIR     := $(shell pwd)/
SBT_OUT_DIR := $(BUILD_DIR)/sbt/$(BUILD_TYPE_DIR)/test/

TESTS := \
	hello \
	argv \
	mm

all: elf

include $(TOPDIR)/make/rules.mk

$(SBT_OUT_DIR):
	mkdir -p $@


### elf ###

CXX      = g++
CXXFLAGS = -m32 -Wall -Werror -g -std=c++11 -pedantic
LDFLAGS  = -m32

$(SBT_OUT_DIR)elf.o: elf.cpp elf.hpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(SBT_OUT_DIR)elf: $(SBT_OUT_DIR) $(SBT_OUT_DIR)elf.o
	$(CXX) $(LDFLAGS) -o $@ $(SBT_OUT_DIR)elf.o

.PHONY: elf
elf: $(SBT_OUT_DIR)elf


### hello ###

$(eval $(call BUILDS,RV32,$(SRC_DIR),$(SBT_OUT_DIR),rv32-hello,rv32-hello))
$(eval $(call BUILDS,X86,$(SRC_DIR),$(SBT_OUT_DIR),x86-hello,x86-hello))
$(eval $(call TRANSLATE_ASM,X86,$(SRC_DIR),$(SBT_OUT_DIR),rv32-hello,rv32-x86-hello))


### x86-test-syscall ###

$(eval $(call CBUILDS,X86,$(SRC_DIR),$(SBT_OUT_DIR),x86-test-syscall,x86-test-syscall,$(X86_SYSCALL_O)))


### argc/argv ###

$(eval $(call BUILD,X86,$(SRC_DIR),$(SBT_OUT_DIR),argv,x86-argv))
.PHONY: test-x86-argv
test-x86-argv: $(SBT_OUT_DIR)x86-argv
	$< one two three; [ $$? -eq 123 ]

$(eval $(call BUILDO,RV32,$(SRC_DIR),$(SBT_OUT_DIR),argv,rv32-argv))
$(eval $(call TRANSLATE,X86,$(SRC_DIR),$(SBT_OUT_DIR),rv32-argv,rv32-x86-argv))
$(eval $(call CLEAN,$(SBT_OUT_DIR),rv32-x86-argv,clean.s))
.PHONY: test-rv32-x86-argv
test-rv32-x86-argv: $(SBT_OUT_DIR)rv32-x86-argv
	$< one two three; [ $$? -eq 123 ]

.PHONY: test-argv
test-argv: test-x86-argv test-rv32-x86-argv


### mm (matrix multiply) ###

DIM := 5

$(eval $(call BUILDO,RV32,$(SRC_DIR),$(SBT_OUT_DIR),mm,rv32-mm,-DROWS=$(DIM)))
$(eval $(call CLINK,RV32,$(SBT_OUT_DIR),$(SBT_OUT_DIR),rv32-mm,rv32-mm))
$(eval $(call RUN,RV32,$(SBT_OUT_DIR),rv32-mm,save-run.out))
$(eval $(call ALIAS,$(SBT_OUT_DIR),rv32-mm))
$(eval $(call TRANSLATE,X86,$(SRC_DIR),$(SBT_OUT_DIR),rv32-mm,rv32-x86-mm))

$(eval $(call BUILD,X86,$(SRC_DIR),$(SBT_OUT_DIR),mm,x86-mm,,-DROWS=$(DIM)))

.PHONY: mm
mm: rv32-mm x86-mm rv32-x86-mm

.PHONY: test-mm
test-mm: mm run-rv32-mm run-x86-mm run-rv32-x86-mm
	diff $(SBT_OUT_DIR)rv32-mm.out $(SBT_OUT_DIR)rv32-x86-mm.out

### RV32 Translator unit tests ###

define UTEST
$(eval UTEST_SRCDIR = $(1))
$(eval UTEST_DSTDIR = $(2))
$(eval UTEST_NAME = $(3))
$(eval UTEST_RV32NAME = rv32-$(UTEST_NAME))
$(eval UTEST_X86NAME = rv32-x86-$(UTEST_NAME))

$(call S2O,RV32,$(UTEST_SRCDIR),$(UTEST_DSTDIR),$(UTEST_RV32NAME),$(UTEST_RV32NAME))
$(call CLINK,RV32,$(UTEST_DSTDIR),$(UTEST_DSTDIR),$(UTEST_RV32NAME),$(UTEST_RV32NAME))
$(call RUN,RV32,$(UTEST_DSTDIR),$(UTEST_RV32NAME),save-run.out)

$(call TRANSLATE,X86,$(UTEST_DSTDIR),$(UTEST_DSTDIR),$(UTEST_RV32NAME),$(UTEST_X86NAME),$(X86_COUNTERS_O))

.PHONY: $(UTEST_NAME)
$(UTEST_NAME): $(UTEST_DSTDIR) $(addprefix $(UTEST_DSTDIR),$(UTEST_RV32NAME) $(UTEST_X86NAME))

.PHONY: test-$(UTEST_NAME)
test-$(UTEST_NAME): $(UTEST_NAME) run-$(UTEST_RV32NAME) run-$(UTEST_X86NAME)
	diff $(UTEST_DSTDIR)$(UTEST_RV32NAME).out $(UTEST_DSTDIR)$(UTEST_X86NAME).out

endef

UTESTS := load-store alu-ops branch fence system m
RV32_UTESTS := $(foreach test,$(UTESTS),rv32-$(test))
RV32_X86_UTESTS := $(foreach test,$(UTESTS),rv32-x86-$(test))

$(foreach test,$(UTESTS),\
$(eval $(call UTEST,$(SRC_DIR),$(SBT_OUT_DIR),$(test))))

.PHONY: utests
utests: $(UTESTS)

# XXX removed system from utests to run (need MSR access and performance
# counters support (not always available in VMs))
run-utests: utests $(foreach test,$(filter-out system,$(UTESTS)),test-$(test))
	@echo "All unit tests passed!"

###


# TEST
# 1: host arch
# 2: dir/
# 3: module
define TEST
$(eval TEST_RV32 = $(RV32_PREFIX)-$(3))
$(eval TEST_TRANS = $(RV32_PREFIX)-$($(1)_PREFIX)-$(3))

test-$(3): run-$(TEST_RV32) run-$(TEST_TRANS)
	diff $(2)$(TEST_RV32).out $(2)$(TEST_TRANS).out

endef


$(foreach test,hello,\
$(eval $(call TEST,X86,$(SBT_OUT_DIR),$(test))))

.PHONY: tests
tests: rv32-hello x86-hello rv32-x86-hello

.PHONY: run-tests
run-tests: tests $(foreach test,$(TESTS),test-$(test)) run-x86-hello

###

.PHONY: alltests
alltests: tests utests x86-test-syscall

.PHONY: run-alltests
run-alltests: run-tests run-utests run-x86-test-syscall

clean:
	rm -rf $(SBT_OUT_DIR)

###
### QEMU tests
###

QEMU_TESTS_BUILD := $(BUILD_DIR)/riscv-qemu-tests
OUT_DIR := $(QEMU_TESTS_BUILD)/rv32i/

RV32_TESTS := add addi and andi aiupc \
    beq bge bgeu blt bltu bne \
    jal jalr \
    lui lb lbu lhu lw sb sw or ori \
    sll slli slt slti sltiu sltu sra srai srl srli sub xor xori
RV32_TESTS_FAILING :=
RV32_TESTS_MISSING := csrrw csrrs csrrc csrrwi csrrsi csrrci \
    ecall ebreak fence fence.i lh sh

RV32_TESTS_TARGETS := $(addprefix rv32-x86-,$(RV32_TESTS))

define TEST2
$(eval TEST2_RV32  = $(2))
$(eval TEST2_TRANS = rv32-x86-$(2))

.PHONY:
run-$(TEST2_RV32):
	$(RV32_RUN) $(1)$(TEST2_RV32) | tee $(1)$(TEST2_RV32).out

test-$(2): run-$(TEST2_RV32) run-$(TEST2_TRANS)
	diff $(1)$(TEST2_RV32).out $(1)$(TEST2_TRANS).out
endef

$(foreach test,$(RV32_TESTS),\
    $(eval $(call TEST2,$(OUT_DIR),$(test))))

$(foreach test,$(RV32_TESTS),\
    $(eval $(call TRANSLATE_ASM,X86,$(OUT_DIR),$(OUT_DIR),$(test),rv32-x86-$(test))))

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

.PHONY: rv32tests
rv32tests:
	$(MAKE) $(RV32_TESTS_TARGETS) $(foreach test,$(RV32_TESTS),test-$(test))
