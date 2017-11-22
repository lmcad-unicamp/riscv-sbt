include $(TOPDIR)/make/config.mk

### config ####

SRC_DIR     := $(shell pwd)/
SBT_OUT_DIR := $(BUILD_DIR)/sbt/$(BUILD_TYPE_DIR)/test/

TESTS := \
	hello \
	argv \
	mm

all: elf tests

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


### x86-test-syscall ###

$(eval $(call BUILD,X86,$(SRC_DIR),$(SBT_OUT_DIR),\
x86-test-syscall,x86-test-syscall,$(X86_SYSCALL_O),$(NOFLAGS),$(ASM),$(C)))


### sbt test ###

MODES        := globals locals
# NARCHS = native archs to build bins to
TEST_NARCHS  := RV32 X86
UTEST_NARCHS := RV32


define NBUILD
$(eval N_ARCH = $(1))
$(eval N_SRCDIR = $(2))
$(eval N_DSTDIR = $(3))
$(eval N_NAME = $(4))
$(eval N_LIBS = $(5))
$(eval N_FLAGS = $(6))
$(eval N_ASM = $(7))
$(eval N_C = $(8))
$(eval N_ARGS = $(9))

$(eval N_PREFIX = $($(N_ARCH)_PREFIX))
$(eval N_IN = $(if $(N_ASM),$(N_PREFIX)-,)$(N_NAME))
$(eval N_OUT = $(N_PREFIX)-$(N_NAME))

# native bins
$(call BUILD,$(N_ARCH),$(N_SRCDIR),$(N_DSTDIR),$(N_IN),$(N_OUT),\
$(N_LIBS),$(N_FLAGS),$(N_ASM),$(N_C),$(N_ARGS))

endef


define SBT_TEST1
$(eval TEST1_DSTARCH = $(1))
$(eval TEST1_DSTDIR = $(2))
$(eval TEST1_NAME = $(3))
$(eval TEST1_MODE = $(4))
$(eval TEST1_LIBS = $(5))
$(eval TEST1_C = $(6))
$(eval TEST1_ARGS = $(7))

$(eval TEST1_TGT = $(TEST1_NAME)-$(TEST1_MODE))
$(eval TEST1_IN = $(RV32_PREFIX)-$(TEST1_NAME))
$(eval TEST1_OUT = $(RV32_PREFIX)-$($(TEST1_DSTARCH)_PREFIX)-$(TEST1_TGT))
$(eval TEST1_NAT = $($(TEST1_DSTARCH)_PREFIX)-$(TEST1_NAME))

$(call TRANSLATE,$(TEST1_DSTARCH),$(TEST1_DSTDIR),$(TEST1_DSTDIR),\
$(TEST1_IN),$(TEST1_OUT),$(TEST1_LIBS),-regs=$(TEST1_MODE),\
$(TEST1_C),$(TEST1_ARGS))

.PHONY: $(TEST1_TGT)
$(TEST1_TGT): $(TEST1_DSTDIR)$(TEST1_OUT)

test-$(TEST1_TGT): $(TEST1_TGT) run-$(TEST1_OUT)
	diff $(TEST1_DSTDIR)$(TEST1_IN).out $(TEST1_DSTDIR)$(TEST1_OUT).out

endef


define SBT_TEST
$(eval TEST_ARCHS = $(1))
$(eval TEST_SRCDIR = $(2))
$(eval TEST_DSTDIR = $(3))
$(eval TEST_NAME = $(4))
$(eval TEST_LIBS = $(5))
$(eval TEST_FLAGS = $(6))
$(eval TEST_ASM = $(7))
$(eval TEST_C = $(8))
$(eval TEST_ARGS = $(9))

# native bins
$(foreach ARCH,$(TEST_ARCHS),\
$(call NBUILD,$(ARCH),$(TEST_SRCDIR),$(TEST_DSTDIR),\
$(TEST_NAME),$(TEST_LIBS),$(TEST_FLAGS),$(TEST_ASM),$(TEST_C),$(TEST_ARGS)))

# translated bins
$(foreach mode,$(MODES),\
$(call SBT_TEST1,X86,$(TEST_DSTDIR),$(TEST_NAME),$(mode),\
$(TEST_LIBS),$(TEST_C),$(TEST_ARGS)))

.PHONY: $(TEST_NAME)
$(TEST_NAME): $(foreach ARCH,$(TEST_ARCHS),$($(ARCH)_PREFIX)-$(TEST_NAME)) \
				$(foreach mode,$(MODES),$(TEST_NAME)-$(mode))

test-$(TEST_NAME): $(foreach ARCH,$(TEST_ARCHS),run-$($(ARCH)_PREFIX)-$(TEST_NAME)) \
					$(foreach mode,$(MODES),test-$(TEST_NAME)-$(mode))

endef


### hello ###

$(eval $(call SBT_TEST,$(TEST_NARCHS),$(SRC_DIR),$(SBT_OUT_DIR),hello,\
$(NOLIBS),$(NOFLAGS),$(ASM),$(NOC)))

### argc/argv ###

$(eval $(call SBT_TEST,$(TEST_NARCHS),$(SRC_DIR),$(SBT_OUT_DIR),argv,\
$(NOLIBS),$(NOFLAGS),$(NOASM),$(C),one two three))

### mm (matrix multiply) ###

DIM := 5
$(eval $(call SBT_TEST,$(TEST_NARCHS),$(SRC_DIR),$(SBT_OUT_DIR),mm,\
$(NOLIBS),-DROWS=$(DIM),$(NOASM),$(C)))

### tests targets ###

.PHONY: tests
tests: x86-test-syscall $(TESTS)

.PHONY: run-tests
run-tests: tests run-x86-test-syscall $(foreach test,$(TESTS),test-$(test))


### RV32 Translator unit tests ###

UTESTS := load-store alu-ops branch fence system m

$(eval SBT_NAT_OBJS = $(SBT_NAT_OBJS) COUNTERS)

$(foreach test,$(UTESTS),\
$(eval $(call SBT_TEST,$(UTEST_NARCHS),$(SRC_DIR),$(SBT_OUT_DIR),$(test),\
$(NOLIBS),$(NOFLAGS),$(ASM),$(C))))

.PHONY: utests
utests: $(UTESTS)

# XXX removed system from utests to run (need MSR access and performance
# counters support (not always available in VMs))
run-utests: utests $(foreach test,$(filter-out system,$(UTESTS)),test-$(test))
	@echo "All unit tests passed!"

###

.PHONY: alltests
alltests: tests utests

.PHONY: run-alltests
run-alltests: alltests run-tests run-utests

clean:
	rm -rf $(SBT_OUT_DIR)

####
#### QEMU tests
####
#
#QEMU_TESTS_BUILD := $(BUILD_DIR)/riscv-qemu-tests
#OUT_DIR := $(QEMU_TESTS_BUILD)/rv32i/
#
#RV32_TESTS := \
#    add addi and andi \
#    aiupc \
#    beq bge bgeu blt bltu bne \
#    jal jalr \
#    lui lb lbu lhu lw sb sw \
#    or ori \
#    sll slli slt slti sltiu sltu sra srai srl srli \
#    sub xor xori
#RV32_TESTS_FAILING :=
#RV32_TESTS_MISSING := csrrw csrrs csrrc csrrwi csrrsi csrrci \
#    ecall ebreak fence fence.i lh sh
#
#RV32_TESTS_TARGETS := $(addprefix rv32-x86-,$(RV32_TESTS))
#
#define TEST2
#$(eval TEST2_RV32  = $(2))
#$(eval TEST2_TRANS = rv32-x86-$(2))
#
#.PHONY: test-$(2)
#test-$(2):
#	$(RV32_RUN) $(1)$(TEST2_RV32)
#	$(1)$(TEST2_TRANS); rc=$$$$?; \
#	if [ "$$$$rc" -ne 0 ]; then \
#		echo $$$$rc; \
#		false; \
#	fi
#
#endef
#
#$(foreach test,$(RV32_TESTS),\
#    $(eval $(call TEST2,$(OUT_DIR),$(test))))
#
#$(foreach test,$(RV32_TESTS),\
#    $(eval $(call TRANSLATE_ASM,X86,$(OUT_DIR),$(OUT_DIR),$(test),rv32-x86-$(test))))
#
#.PHONY: rv32tests_status
#rv32tests_status:
#	@echo passing: `echo $(RV32_TESTS) | wc -w`
#	@echo $(RV32_TESTS)
#	@echo failing: `echo $(RV32_TESTS_FAILING) | wc -w`
#	@echo $(RV32_TESTS_FAILING)
#	@echo missing: `echo $(RV32_TESTS_MISSING) | wc -w`
#	@echo $(RV32_TESTS_MISSING)
#	@echo total: \
#		`echo $(RV32_TESTS) $(RV32_TESTS_FAILING) $(RV32_TESTS_MISSING) | wc -w`
#
#.PHONY: rv32tests
#rv32tests:
#	$(MAKE) $(RV32_TESTS_TARGETS) $(foreach test,$(RV32_TESTS),test-$(test))
