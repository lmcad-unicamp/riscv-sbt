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


### sbt test ###

define SBT_TEST1
$(eval TEST1_DSTARCH = $(1))
$(eval TEST1_DSTDIR = $(2))
$(eval TEST1_NAME = $(3))
$(eval TEST1_MODE = $(4))
$(eval TEST1_ASM = $(5))
$(eval TEST1_TGT = $(TEST1_NAME)-$(TEST1_MODE))
$(eval TEST1_IN = rv32-$(TEST1_NAME))
$(eval TEST1_OUT = rv32-$($(TEST1_DSTARCH)_PREFIX)-$(TEST1_TGT))
$(eval TEST1_NAT = $($(TEST1_DSTARCH)_PREFIX)-$(TEST1_NAME))

$(call $(if $(TEST1_ASM),TRANSLATE_ASM,TRANSLATE),\
$(TEST1_DSTARCH),$(TEST1_DSTDIR),$(TEST1_DSTDIR),\
$(TEST1_IN),$(TEST1_OUT),,-regs=$(TEST1_MODE))

.PHONY: $(TEST1_TGT)
$(TEST1_TGT): $(TEST1_DSTDIR)$(TEST1_OUT)

test-$(TEST1_TGT): run-$(TEST1_OUT)
	diff $(TEST1_DSTDIR)$(TEST1_IN).out $(TEST1_DSTDIR)$(TEST1_OUT).out
	diff $(TEST1_DSTDIR)$(TEST1_NAT).out $(TEST1_DSTDIR)$(TEST1_OUT).out

endef

define SBT_BUILD
$(eval SBTB_ARCH = $(1))
$(eval SBTB_SRCDIR = $(2))
$(eval SBTB_DSTDIR = $(3))
$(eval SBTB_NAME = $(4))
$(eval SBTB_ASM = $(5))

endef

define SBT_TEST
$(eval TEST_SRCDIR = $(1))
$(eval TEST_DSTDIR = $(2))
$(eval TEST_NAME = $(3))
$(eval TEST_ASM = $(4))

# rv32 bin
$(eval $(call BUILDS,RV32,$(TEST_SRCDIR),$(TEST_DSTDIR),rv32-$(TEST_NAME),rv32-$(TEST_NAME)))

# x86 bin
$(eval $(call BUILDS,RV32,$(TEST_SRCDIR),$(TEST_DSTDIR),x86-$(TEST_NAME),x86-$(TEST_NAME)))

$(foreach mode,globals locals,\
$(call SBT_TEST1,X86,$(TEST_DSTDIR),$(TEST_NAME),$(mode),$(TEST_ASM)))

endef


### hello ###

$(eval $(call SBT_TEST,$(SRC_DIR),$(SBT_OUT_DIR),hello,asm))


# translated binaries

define HELLO_TEST
$(eval HELLO_DSTDIR = $(1))
$(eval HELLO_NAME = $(2))
$(eval HELLO_FLAGS = $(3))
$(eval HELLO_RV32_X86 = rv32-x86-$(HELLO_NAME))

$(eval $(call TRANSLATE_ASM,X86,$(HELLO_DSTDIR),$(HELLO_DSTDIR),\
rv32-hello,$(HELLO_RV32_X86),,$(HELLO_FLAGS)))

.PHONY: $(HELLO_NAME)
$(HELLO_NAME): $(HELLO_DSTDIR)$(HELLO_RV32_X86)

test-$(HELLO_NAME): run-$(HELLO_RV32_X86)
	diff $(HELLO_DSTDIR)rv32-hello.out $(HELLO_DSTDIR)$(HELLO_RV32_X86).out
	diff $(HELLO_DSTDIR)x86-hello.out $(HELLO_DSTDIR)$(HELLO_RV32_X86).out

endef

# globals
$(eval $(call HELLO_TEST,$(SBT_OUT_DIR),hello-globals,-regs=globals))
# locals
$(eval $(call HELLO_TEST,$(SBT_OUT_DIR),hello-locals,-regs=locals))

.PHONY: hello
hello: $(foreach bin,rv32-hello x86-hello,$(SBT_OUT_DIR)$(bin)) \
		hello-locals hello-globals

.PHONY: test-hello
test-hello: run-rv32-hello run-x86-hello test-hello-globals test-hello-locals


### x86-test-syscall ###

$(eval $(call CBUILDS,X86,$(SRC_DIR),$(SBT_OUT_DIR),x86-test-syscall,x86-test-syscall,$(X86_SYSCALL_O)))


### argc/argv ###

$(eval $(call SBT_TEST,$(SRC_DIR),$(SBT_OUT_DIR),argv))

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


### tests targets ###

.PHONY: tests
tests: $(TESTS)

.PHONY: run-tests
run-tests: tests $(foreach test,$(TESTS),test-$(test))


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
RV32_TESTS_MISSING := csrrw csrrs csrrc csrrwi csrrsi csrrci \
    ecall ebreak fence fence.i lh sh

RV32_TESTS_TARGETS := $(addprefix rv32-x86-,$(RV32_TESTS))

define TEST2
$(eval TEST2_RV32  = $(2))
$(eval TEST2_TRANS = rv32-x86-$(2))

.PHONY: test-$(2)
test-$(2):
	$(RV32_RUN) $(1)$(TEST2_RV32)
	$(1)$(TEST2_TRANS); rc=$$$$?; \
	if [ "$$$$rc" -ne 0 ]; then \
		echo $$$$rc; \
		false; \
	fi

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
