include $(TOPDIR)/make/config.mk

### config ####

CXXFLAGS = -m32 -Wall -Werror -O0 -g -std=c++11 -pedantic
LDFLAGS  = -m32

BINS := elf \
	rv32-hello x86-hello \
	x86-test-syscall \
	rv32-x86-hello \
	x86-mm rv32-mm rv32-x86-mm

all: elf x86-test-syscall

include $(TOPDIR)/make/rules.mk

### elf ###

$(eval $(call NBUILD,elf))

### hello ###

$(eval $(call BUILD,RV32,rv32-hello,,,save-run.out))
$(eval $(call BUILD,X86,x86-hello,,,save-run.out))
$(eval $(call AS,X86,x86-dummy))

rv32-x86-hello.bc: rv32-hello.o x86-dummy.o
	riscv-sbt -o $@ $<

rv32-x86-hello.s: rv32-x86-hello.bc
	llc -O3 -o $@ -march x86 $<

X86_LIBS = $(X86_SYSCALL_O) $(X86_RVSC_O) x86-dummy.o
CLEAN_EXTRA = rv32-x86-hello.s
$(eval $(call BUILD,X86,rv32-x86-hello,,,save-run.out))
X86_LIBS =
CLEAN_EXTRA =

test-hello: run-rv32-hello run-rv32-x86-hello
	diff rv32-hello.out rv32-x86-hello.out

### x86-test-syscall ###

X86_LIBS = $(X86_SYSCALL_O)
$(eval $(call ASNCLINK,X86,x86-test-syscall))
X86_LIBS =

### mm (matrix multiply) ###

# CFLAGS = -Wall -Werror -O0
$(eval $(call BUILD,X86,x86-mm,CLANG,mm,save-run.out))
$(eval $(call BUILD,RV32,rv32-mm,CLANG,mm,save-run.out))
$(eval $(call TRANSLATE_C,RV32,X86,mm))

TESTS := hello mm
RV32_TESTS := $(foreach test,$(TESTS),rv32-$(test))
RV32_X86_TESTS := $(foreach test,$(TESTS),rv32-x86-$(test))

tests: $(RV32_TESTS) $(RV32_X86_TESTS)

# XXX removed mm from tests to run
run-tests: tests $(foreach test,$(filter-out mm,$(TESTS)),test-$(test))
	@echo "All tests passed!"

#clean-tests:
#	rm -f $(foreach test,$(RV32_TESTS),rv32-$(test).out)

### RV32 Translator unit tests ###

UTESTS := load-store alu-ops branch fence system m
RV32_UTESTS := $(foreach test,$(UTESTS),rv32-$(test))
RV32_X86_UTESTS := $(foreach test,$(UTESTS),rv32-x86-$(test))

$(eval $(foreach test,$(RV32_UTESTS),\
  $(call ASNCLINK,RV32,$(test),,save-run.out)))

$(eval $(foreach test,$(UTESTS),\
  $(call TRANSLATE_C,RV32,X86,$(test))))

utests: $(RV32_UTESTS) $(RV32_X86_UTESTS)

# XXX removed system from utests to run (need MSR access and performance
# counters support (not always available in VMs))
run-utests: utests $(foreach test,$(filter-out system,$(UTESTS)),test-$(test))
	@echo "All unit tests passed!"


#$(warning clean-rv32-x86: $(foreach test,$(RV32_X86_UTESTS),clean-$(test)))

clean-utests: $(foreach test,$(RV32_UTESTS),clean-$(test)) \
  $(foreach test,$(RV32_X86_UTESTS),clean-$(test))

###

alltests: utests tests x86-test-syscall

run-alltests: alltests run-utests run-tests
	./x86-test-syscall

clean: $(foreach prog,$(BINS),clean-$(prog)) clean-utests
	rm -f x86-dummy.o
