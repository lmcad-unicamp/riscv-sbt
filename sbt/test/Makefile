include $(TOPDIR)/make/config.mk

### config ####

SRC_DIR     := $(PWD)/
SBT_OUT_DIR := $(BUILD_DIR)/sbt/$(BUILD_TYPE_DIR)/test/

TESTS := hello

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

#$(eval $(call S2O,X86,$(SRC_DIR),$(SBT_OUT_DIR),x86-dummy))


### x86-test-syscall ###

$(eval $(call CBUILDS,X86,$(SRC_DIR),$(SBT_OUT_DIR),x86-test-syscall,x86-test-syscall,$(X86_SYSCALL_O)))


### mm (matrix multiply) ###

$(eval $(call BUILD,RV32,$(SRC_DIR),$(SBT_OUT_DIR),mm,rv32-mm))
$(eval $(call BUILD,X86,$(SRC_DIR),$(SBT_OUT_DIR),mm,x86-mm))
$(eval $(call TRANSLATE,X86,$(SRC_DIR),$(SBT_OUT_DIR),rv32-mm,rv32-x86-mm))

.PHONY: mm
mm: rv32-mm x86-mm rv32-x86-mm


#### RV32 Translator unit tests ###

UTESTS := load-store alu-ops branch fence system m
RV32_UTESTS := $(foreach test,$(UTESTS),rv32-$(test))
RV32_X86_UTESTS := $(foreach test,$(UTESTS),rv32-x86-$(test))

$(foreach test,$(RV32_UTESTS),\
    $(eval $(call CBUILDS,RV32,$(SRC_DIR),$(SBT_OUT_DIR),$(test),$(test))))

$(foreach test,$(UTESTS),\
    $(eval $(call TRANSLATE,X86,$(SRC_DIR),$(SBT_OUT_DIR),rv32-$(test),rv32-x86-$(test),$(X86_COUNTERS_O))))

$(foreach test,$(UTESTS),\
    $(eval $(call TEST,X86,$(SBT_OUT_DIR),$(test))))

.PHONY: utests
utests: $(RV32_UTESTS) $(RV32_X86_UTESTS)

# XXX removed system from utests to run (need MSR access and performance
# counters support (not always available in VMs))
.PHONY: run-utests
run-utests: utests $(foreach test,$(filter-out system,$(UTESTS)),test-$(test))
	@echo "All unit tests passed!"

###

$(foreach test,$(TESTS),\
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
