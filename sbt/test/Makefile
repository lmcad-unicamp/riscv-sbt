include $(TOPDIR)/config.mk

BINS := elf \
	rv32-hello x86-hello \
	x86-test-syscall \
	rv32-x86-hello \
	x86-main rv32-main rv32-x86-main

RUN_BINS := rv32-hello x86-hello x86-test-syscall \
	rv32-x86-hello \
	x86-main rv32-main rv32-x86-main

all: $(BINS)

include $(TOPDIR)/rules.mk

CXXFLAGS = -m32 -Wall -Werror -O0 -g -std=c++11 -pedantic
LDFLAGS  = -m32
$(eval $(call NBUILD,elf))

$(eval $(call BUILD,RV32,rv32-hello))
$(eval $(call BUILD,X86,x86-hello))

X86_LIBS = $(X86_SYSCALL_O)
$(eval $(call BUILD,X86,x86-test-syscall))

rv32-x86-hello.bc: rv32-hello.o
	riscv-sbt -o $@ $<

rv32-x86-hello.s: rv32-x86-hello.bc
	llc -O3 -o $@ -march x86 $<

$(eval $(call BUILD,X86,rv32-x86-hello))

X86_LIBS =
$(eval $(call BUILD,X86,x86-main,CLANG,main))

$(eval $(call BUILD,RV32,rv32-main,CLANG,main))


### rv32-x86-main ###

rv32-x86-main.bc: rv32-main.o
	riscv-sbt -o $@ $<

rv32-x86-main.s: rv32-x86-main.bc
	llc -O3 -o $@ -march x86 $<

X86_LIBS = $(X86_SYSCALL_O)
$(eval $(call AS,X86,rv32-x86-main))
$(eval $(call CLINK,X86,rv32-x86-main))

clean-rv32-x86-main:
	rm -f rv32-x86-main*

run: $(foreach prog,$(RUN_BINS),run-$(prog))
clean: $(foreach prog,$(BINS),clean-$(prog))
	rm -f rv32-x86-hello.bc
