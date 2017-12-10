include $(TOPDIR)/make/config.mk

SRCDIR := $(shell pwd)
DSTDIR := $(BUILD_DIR)/test

BINS  := rv32-hello-asm rv32-hello x86-hello rv32-linux-hello

all:  $(BINS)
run:  $(addsuffix -run,$(BINS))

clean:
	rm -rf $(DSTDIR)

### rules ###

$(DSTDIR)/rv32-hello-asm: $(SRCDIR)/rv32-hello.s
	$(BUILD_PY) --arch rv32 --srcdir $(SRCDIR) --dstdir $(DSTDIR) \
		rv32-hello.s -o rv32-hello-asm -C

.PHONY: rv32-hello-asm
rv32-hello-asm: $(DSTDIR)/rv32-hello-asm

.PHONY: rv32-hello-asm-run
rv32-hello-asm-run:
	$(RUN_PY) --arch rv32 --dir $(DSTDIR) rv32-hello-asm


define BLDNRUN
$(eval A_ARCH = $(1))
$(eval A_MOD  = $(2))

$(eval A_BIN = $(A_ARCH)-$(A_MOD))

$(DSTDIR)/$(A_BIN): $(SRCDIR)/$(A_MOD).c
	$(BUILD_PY) --arch $(A_ARCH) --srcdir $(SRCDIR) --dstdir $(DSTDIR) \
		$(A_MOD).c -o $(A_BIN)

.PHONY: $(A_BIN)
$(A_BIN): $(DSTDIR)/$(A_BIN)

$(A_BIN)-run:
	$(RUN_PY) --arch $(A_ARCH) --dir $(DSTDIR) $(A_BIN)

endef

$(eval $(foreach ARCH,rv32 x86 rv32-linux,$(call BLDNRUN,$(ARCH),hello)))
