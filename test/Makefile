include $(TOPDIR)/make/config.mk

BINS  := rv32-hello-asm rv32-hello x86-hello rv32-linux-hello

all:  $(BINS)

SRCDIR := $(shell pwd)
DSTDIR := $(BUILD_DIR)/test

include $(TOPDIR)/make/rules.mk

$(eval $(call TGT,rv32,$(SRCDIR),$(DSTDIR),rv32-hello,rv32-hello-asm,--asm -C))
$(eval $(call TGT,rv32,$(SRCDIR),$(DSTDIR),hello,rv32-hello))
$(eval $(call TGT,x86,$(SRCDIR),$(DSTDIR),hello,x86-hello))
$(eval $(call TGT,rv32-linux,$(SRCDIR),$(DSTDIR),hello,rv32-linux-hello))

run: $(foreach bin,$(BINS),$(bin)-run)

clean: $(foreach bin,$(BINS),$(bin)-clean)
