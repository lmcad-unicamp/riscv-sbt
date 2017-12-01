include $(TOPDIR)/make/config.mk

BINS  := rv32-hello-asm rv32-hello #x86-hello rv32-linux-hello

all:  $(BINS)

SRCDIR := $(shell pwd)
DSTDIR := $(BUILD_DIR)/test


###

define TGT
$(eval TGT_FLAGS = $(1))
$(eval TGT_ARCH = $(2))
$(eval TGT_OUT = $(3))
$(eval TGT_INS = $(4))

$(DSTDIR)/$(TGT_OUT):
	$(BUILD_PY) $(TGT_FLAGS) build $(TGT_ARCH) $(SRCDIR) $(DSTDIR) $(TGT_OUT) $(TGT_INS)

.PHONY: $(TGT_OUT)
$(TGT_OUT): $(DSTDIR)/$(TGT_OUT)

$(TGT_OUT)-run:
	$(BUILD_PY) run $(TGT_ARCH) $(DSTDIR) $(TGT_OUT)

$(TGT_OUT)-clean:
	$(BUILD_PY) $(TGT_FLAGS) clean $(DSTDIR) $(TGT_OUT)

endef

###

$(eval $(call TGT,--asm,rv32,rv32-hello-asm,rv32-hello))

$(eval $(call TGT,,rv32,rv32-hello,hello))

#$(eval $(call BUILD,X86,$(SRCDIR),$(DSTDIR),hello,x86-hello,\
#$(NOLIBS),$(NOFLAGS),$(NOASM),$(C)))

#$(eval $(call BUILD,RV32_LINUX,$(SRCDIR),$(DSTDIR),hello,rv32-linux-hello,\
#$(NOLIBS),$(NOFLAGS),$(NOASM),$(C)))

run: $(foreach bin,$(BINS),$(bin)-run)

clean: $(foreach bin,$(BINS),$(bin)-clean)
