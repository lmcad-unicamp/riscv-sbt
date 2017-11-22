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

$(eval ADD_ASM_SRC_PREFIX = 1)

### x86-test-syscall ###

$(eval $(call BUILD,X86,$(SRC_DIR),$(SBT_OUT_DIR),\
x86-test-syscall,x86-test-syscall,$(X86_SYSCALL_O),$(NOFLAGS),$(ASM),$(C)))

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

# NOTE removed system from utests to run (need MSR access and performance
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
