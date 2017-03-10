include $(TOPDIR)/config.mk

### config ####

CXXFLAGS = -m32 -Wall -Werror -O0 -g -std=c++11 -pedantic
LDFLAGS  = -m32

BINS := elf \
	rv32-hello x86-hello \
	x86-test-syscall \
	rv32-x86-hello \
	x86-main rv32-main rv32-x86-main

RUN_BINS := $(filter-out elf,$(BINS))

all: $(BINS)

include $(TOPDIR)/rules.mk

### elf ###

$(eval $(call NBUILD,elf))

### hello ###

$(eval $(call BUILD,RV32,rv32-hello))
$(eval $(call BUILD,X86,x86-hello))

rv32-x86-hello.bc: rv32-hello.o
	riscv-sbt -o $@ $<

rv32-x86-hello.s: rv32-x86-hello.bc
	llc -O3 -o $@ -march x86 $<

X86_LIBS = $(X86_SYSCALL_O) $(X86_RVSC_O)
CLEAN_EXTRA = rv32-x86-hello.s
$(eval $(call BUILD,X86,rv32-x86-hello))
X86_LIBS =
CLEAN_EXTRA =

### x86-test-syscall ###

X86_LIBS = $(X86_SYSCALL_O)
$(eval $(call BUILD,X86,x86-test-syscall))
X86_LIBS =

### main ###

CFLAGS = -Wall -Werror -O0
$(eval $(call BUILD,X86,x86-main,CLANG,main))
$(eval $(call BUILD,RV32,rv32-main,CLANG,main))
$(eval $(call TRANSLATE,RV32,X86,main))

### RV32 Translator unit tests ###

TESTS := load-store alu-ops branch fence
RV32_TESTS := $(foreach test,$(TESTS),rv32-$(test))
RV32_X86_TESTS := $(foreach test,$(TESTS),rv32-x86-$(test))

$(eval $(foreach test,$(RV32_TESTS),$(call ASNCLINK,RV32,$(test),,save-run.out)))
$(eval $(foreach test,$(TESTS),$(call TRANSLATE,RV32,X86,$(test))))

tests: $(RV32_TESTS) $(RV32_X86_TESTS)

run-tests: tests $(foreach test,$(TESTS),test-$(test))
	@echo "All tests passed!"

clean-tests: $(foreach test,$(RV32_TESTS),clean-$(test)) \
  $(foreach test,$(RV32_X86_TESTS),clean-$(test))

###

run: $(foreach prog,$(RUN_BINS),run-$(prog))

clean: $(foreach prog,$(BINS),clean-$(prog)) clean-tests
