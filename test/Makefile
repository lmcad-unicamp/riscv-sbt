include $(TOPDIR)/config.mk

BINS := rv32-gcc-hello rv32-hello rv32-clang-hello x86-clang-hello

all: $(BINS)

include $(TOPDIR)/rules.mk

$(eval $(call BUILD,RV32,rv32-gcc-hello,GCC,hello))
$(eval $(call BUILD,RV64,rv64-gcc-hello,GCC,hello))
$(eval $(call BUILD,RV32,rv32-hello))
$(eval $(call BUILD,RV32,rv32-clang-hello,CLANG,hello))
$(eval $(call BUILD,X86,x86-clang-hello,CLANG,hello))

run: $(foreach prog,$(BINS),run-$(prog))
clean: $(foreach prog,$(BINS),clean-$(prog))
