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
--asm,--syscall))

### hello ###

TNARCHS := rv32 x86

$(eval $(call UTEST,$(TNARCHS),hello,$(SRCDIR),$(DSTDIR),\
--asm -C,,--save-output,,add-arch-prefix))

### argc/argv ###

ARGS   := one two three

$(eval $(call UTEST,$(TNARCHS),argv,$(SRCDIR),$(DSTDIR),,,$(ARGS) --save-output))

### mm (matrix multiply) ###

DIM    := 4
LFLAGS := --cflags="-DROWS=$(DIM)"
RFLAGS := --save-output

$(eval $(call UTEST,$(TNARCHS),mm,$(SRCDIR),$(DSTDIR),,$(LFLAGS),$(RFLAGS)))

### tests targets ###

.PHONY: tests
tests: x86-syscall-test $(TESTS)

.PHONY: tests-run
tests-run: tests x86-syscall-test-run $(foreach test,$(TESTS),$(test)-test)


### RV32 Translator unit tests ###

UTNARCHS := rv32
UTESTS   := load-store alu-ops branch fence system m

$(foreach test,$(UTESTS),$(eval \
$(call UTEST,$(UTNARCHS),$(test),$(SRCDIR),$(DSTDIR),\
--asm,--counters,--save-output,,add-arch-prefix)))

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
