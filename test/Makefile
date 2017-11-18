include $(TOPDIR)/make/config.mk

BINS  := rv32-hello rv32-gcc-hello rv32-clang-hello x86-clang-hello \
         rv32-linux-hello

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

# hello: asm
$(eval $(call BUILDS,RV32,./,./,rv32-hello,rv32-hello))
$(eval $(call CLEAN,./,rv32-hello))

# hello: gcc
$(eval $(call GCC_C2S,RV32,./,./,hello,rv32-gcc-hello))
$(eval $(call CBUILDS,RV32,./,./,rv32-gcc-hello,rv32-gcc-hello))
$(eval $(call CLEAN,./,rv32-gcc-hello,clean.s))

# hello: clang rv32
$(eval $(call BUILD,RV32,./,./,hello,rv32-clang-hello))
$(eval $(call CLEAN,./,rv32-clang-hello,clean.s))

# hello: clang x86
$(eval $(call BUILD,X86,./,./,hello,x86-clang-hello))
$(eval $(call CLEAN,./,x86-clang-hello,clean.s))

# hello: clang rv32-linux
$(eval $(call BUILD,RV32_LINUX,./,./,hello,rv32-linux-hello))
$(eval $(call CLEAN,./,rv32-linux-hello,clean.s))

# run/clean

run: $(foreach prog,$(BINS),run-$(prog))

clean: $(foreach prog,$(BINS),clean-$(prog))
