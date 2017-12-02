include $(TOPDIR)/make/config.mk

### config ####

SRCDIR := $(shell pwd)
DSTDIR := $(BUILD_DIR)/sbt/$(BUILD_TYPE_DIR)/test

TESTS  := hello argv mm

all: elf tests

include $(TOPDIR)/make/rules.mk

### elf ###

CXX      = g++
CXXFLAGS = -m32 -Wall -Werror -g -std=c++11 -pedantic
LDFLAGS  = -m32

$(DSTDIR):
	mkdir -p $@

$(DSTDIR)/elf.o: elf.cpp elf.hpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(DSTDIR)/elf: $(DSTDIR) $(DSTDIR)/elf.o
	$(CXX) $(LDFLAGS) -o $@ $(DSTDIR)/elf.o

.PHONY: elf
elf: $(DSTDIR)/elf

### x86-syscall-test ###

$(eval $(call TGT,x86,$(SRCDIR),$(DSTDIR),x86-syscall-test,x86-syscall-test,\
--asm,--libs="$(X86_SYSCALL_O)"))

###

define TEST
$(eval TEST_NAME = $(1))
$(eval TEST_TGTS = $(2))
$(eval TEST_DIR = $(3))

.PHONY: $(TEST_NAME)
$(TEST_NAME): $(TEST_TGTS)
.PHONY: $(TEST_NAME)-run
$(TEST_NAME)-run: $(addsuffix -run,$(TEST_TGTS))

$(eval T1 = $(word 1,$(TEST_TGTS)))
$(eval T2 = $(word 2,$(TEST_TGTS)))
$(eval T3G = $(word 3,$(TEST_TGTS))-globals)
$(eval T3L = $(word 3,$(TEST_TGTS))-locals)

.PHONY: $(TEST_NAME)-test
$(TEST_NAME)-test: $(TEST_NAME)-run
	diff $(TEST_DIR)/$(T1).out $(TEST_DIR)/$(T3G).out
	diff $(TEST_DIR)/$(T2).out $(TEST_DIR)/$(T3G).out
	diff $(TEST_DIR)/$(T1).out $(TEST_DIR)/$(T3L).out
	diff $(TEST_DIR)/$(T2).out $(TEST_DIR)/$(T3L).out

endef

### hello ###

GFLAGS := --asm -C
LFLAGS :=
RFLAGS := --save-output

$(eval $(call TGT,rv32,$(SRCDIR),$(DSTDIR),rv32-hello,rv32-hello,\
$(GFLAGS),$(LFLAGS),$(RFLAGS)))
$(eval $(call TGT,x86,$(SRCDIR),$(DSTDIR),x86-hello,x86-hello,\
$(GFLAGS),$(LFLAGS),$(RFLAGS)))
$(eval $(call TRANSLATE,x86,$(DSTDIR),rv32-hello,rv32-x86-hello,\
-C,$(RFLAGS)))
$(eval $(call TEST,hello,rv32-hello x86-hello rv32-x86-hello,$(DSTDIR)))

### argc/argv ###

ARGS   := one two three
GFLAGS :=
RFLAGS := $(ARGS) --save-output

$(eval $(call TGT,rv32,$(SRCDIR),$(DSTDIR),argv,rv32-argv,\
$(GFLAGS),$(LFLAGS),$(RFLAGS)))
$(eval $(call TGT,x86,$(SRCDIR),$(DSTDIR),argv,x86-argv,\
$(GFLAGS),$(LFLAGS),$(RFLAGS)))
$(eval $(call TRANSLATE,x86,$(DSTDIR),rv32-argv,rv32-x86-argv,\
$(GFLAGS),$(RFLAGS)))
$(eval $(call TEST,argv,rv32-argv x86-argv rv32-x86-argv,$(DSTDIR)))

### mm (matrix multiply) ###

DIM    := 4
LFLAGS := --cflags="-DROWS=$(DIM)"
RFLAGS := --save-output

$(eval $(call TGT,rv32,$(SRCDIR),$(DSTDIR),mm,rv32-mm,\
$(GFLAGS),$(LFLAGS),$(RFLAGS)))
$(eval $(call TGT,x86,$(SRCDIR),$(DSTDIR),mm,x86-mm,\
$(GFLAGS),$(LFLAGS),$(RFLAGS)))
$(eval $(call TRANSLATE,x86,$(DSTDIR),rv32-mm,rv32-x86-mm,\
$(GFLAGS),$(RFLAGS)))
$(eval $(call TEST,mm,rv32-mm x86-mm rv32-x86-mm,$(DSTDIR)))

### tests targets ###

.PHONY: tests
tests: x86-syscall-test $(TESTS)

.PHONY: tests-run
tests-run: tests x86-syscall-test-run $(foreach test,$(TESTS),$(test)-test)


### RV32 Translator unit tests ###

UTESTS := load-store alu-ops branch fence system m

$(eval SBT_NAT_OBJS = $(SBT_NAT_OBJS) COUNTERS)

$(foreach test,$(UTESTS),\
$(eval $(call SBT_TEST,$(UTEST_NARCHS),$(SRC_DIR),$(SBT_OUT_DIR),\
$(test),$(test),$(NOLIBS),$(NOFLAGS),$(ASM),$(C))))

.PHONY: utests
utests: $(UTESTS)

# NOTE removed system from utests to run (need MSR access and performance
# counters support (not always available in VMs))
utests-run: utests $(foreach test,$(filter-out system,$(UTESTS)),$(test)-test)
	@echo "All unit tests passed!"

###

.PHONY: alltests
alltests: tests utests

.PHONY: alltests-run
alltests-run: alltests tests-run utests-run

clean:
	rm -rf $(DSTDIR)
