include $(TOPDIR)/make/config.mk

BINS  := rv32-hello rv32-gcc-hello rv32-clang-hello x86-clang-hello

all:  $(BINS)

include $(TOPDIR)/make/rules.mk


RV32_GCC := $(RV32_TRIPLE)-gcc
X86_GCC  := $(X86_64_TRIPLE)-gcc -m32

# 1: prefix (X86/RV32)
# 2: source dir/
# 3: out dir/
# 4: input file (.c)
# 5: output file (.s)
define GCC_C2S
$(3)$(5).s: $(2)$(4).c
	cd $(3) && \
		$$($(1)_GCC) -S $(2)$(4).c -o $(5).s
endef

$(eval $(call BUILDS,RV32,./,./,rv32-hello,rv32-hello))
$(eval $(call CLEAN,./,rv32-hello))

$(eval $(call GCC_C2S,RV32,./,./,hello,rv32-gcc-hello))
$(eval $(call CBUILDS,RV32,./,./,rv32-gcc-hello,rv32-gcc-hello))
$(eval $(call CLEAN,./,rv32-gcc-hello,clean.s))

$(eval $(call BUILD,RV32,./,./,hello,rv32-clang-hello))
$(eval $(call CLEAN,./,rv32-clang-hello,clean.s))

$(eval $(call BUILD,X86,./,./,hello,x86-clang-hello))
$(eval $(call CLEAN,./,x86-clang-hello,clean.s))

# run/clean

run: $(foreach prog,$(BINS),run-$(prog))

clean: $(foreach prog,$(BINS),clean-$(prog))


### extra
#EXTRA := rv64-hello rv64-gcc-hello rv64-clang-hello \
#         rv32-gcc-linux-hello rv64-gcc-linux-hello
#
#extra: $(EXTRA)
#
#$(eval $(call BUILD,RV64,rv64-hello))
#$(eval $(call BUILD,RV64,rv64-gcc-hello,GCC,hello))
#$(eval $(call BUILD,RV64,rv64-clang-hello,CLANG,hello))
#
## linux 32
#
#M32    := -march=rv32g -mabi=ilp32d
#CFLAGS := -Wall -Werror -O2
#
#rv32-gcc-linux-hello: rv32-gcc-linux-hello.o
#	$(RV64_LINUX_TRIPLE)-gcc $(M32) -static -o $@ $<
#
#rv32-gcc-linux-hello.o: hello.c
#	$(RV64_LINUX_TRIPLE)-gcc $(M32) $(CFLAGS) -o $@ -c $<
#
#clean-rv32-gcc-linux-hello:
#	rm -f rv32-gcc-linux-hello rv32-gcc-linux-hello.o
#
#SPIKE32 := spike $(PK32)
#run-rv32-gcc-linux-hello:
#	$(SPIKE32) rv32-gcc-linux-hello
#
## linux 64
#
#rv64-gcc-linux-hello: rv64-gcc-linux-hello.o
#	$(RV64_LINUX_TRIPLE)-gcc -static -o $@ $<
#
#rv64-gcc-linux-hello.o: hello.c
#	$(RV64_LINUX_TRIPLE)-gcc $(CFLAGS) -o $@ -c $<
#
#clean-rv64-gcc-linux-hello:
#	rm -f rv64-gcc-linux-hello rv64-gcc-linux-hello.o
#
#SPIKE64 := spike --isa=RV64IMAFD pk
#run-rv64-gcc-linux-hello:
#	$(SPIKE64) rv64-gcc-linux-hello
#
#run-extra: $(foreach prog,$(EXTRA),run-$(prog))
#clean-extra: $(foreach prog,$(EXTRA),clean-$(prog))
