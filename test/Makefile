include $(TOPDIR)/make/config.mk

BINS  := rv32-hello-asm rv32-hello x86-hello rv32-linux-hello

all:  $(BINS)

include $(TOPDIR)/make/rules.mk

SRCDIR := $(shell pwd)/
DSTDIR := $(BUILD_DIR)/test/

$(DSTDIR):
	mkdir -p $@

$(eval $(call BUILD,RV32,$(SRCDIR),$(DSTDIR),rv32-hello,rv32-hello-asm,\
$(NOLIBS),$(NOFLAGS),$(ASM),$(NOC)))

$(eval $(call BUILD,RV32,$(SRCDIR),$(DSTDIR),hello,rv32-hello,\
$(NOLIBS),$(NOFLAGS),$(NOASM),$(C)))

$(eval $(call BUILD,X86,$(SRCDIR),$(DSTDIR),hello,x86-hello,\
$(NOLIBS),$(NOFLAGS),$(NOASM),$(C)))

$(eval $(call BUILD,RV32_LINUX,$(SRCDIR),$(DSTDIR),hello,rv32-linux-hello,\
$(NOLIBS),$(NOFLAGS),$(NOASM),$(C)))

# run/clean

run: $(foreach prog,$(BINS),run-$(prog))

clean: $(foreach prog,$(BINS),clean-$(prog))
