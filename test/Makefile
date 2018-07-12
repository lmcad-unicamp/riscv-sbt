ifeq ($(TOPDIR),)
$(error "TOPDIR not set. Please run '. scripts/env.sh' first.")
endif

BUILD_PY  := $(TOPDIR)/scripts/auto/build.py
RUN_PY    := $(TOPDIR)/scripts/auto/run.py
SRCDIR    := $(shell pwd)
DSTDIR    := $(TOPDIR)/build/test
ARM_DSTDIR := $(ARM_TOPDIR)/build/test
ADB_DSTDIR := $(ADB_TOPDIR)/build/test

ARCHS     := rv32-linux x86 arm
COMPILERS := clang gcc

BINS      := \
    $(foreach arch,$(ARCHS),$(foreach cc,$(COMPILERS),$(arch)-$(cc)-hello))

all:  $(BINS) rv32-linux-asm-hello
run:  $(addsuffix -run,$(BINS)) rv32-linux-asm-hello-run

clean:
	rm -rf $(DSTDIR)

### rules ###

$(DSTDIR)/rv32-linux-asm-hello: $(SRCDIR)/rv32-hello.s
	$(BUILD_PY) --arch rv32-linux --srcdir $(SRCDIR) --dstdir $(DSTDIR) \
		rv32-hello.s -o rv32-linux-asm-hello -C --no-relax

.PHONY: rv32-linux-asm-hello
rv32-linux-asm-hello: $(DSTDIR)/rv32-linux-asm-hello

.PHONY: rv32-linux-asm-hello-run
rv32-linux-asm-hello-run:
	$(RUN_PY) --arch rv32-linux --dir $(DSTDIR) rv32-linux-asm-hello


define BLDNRUN
$(eval A_ARCH = $(1))
$(eval A_CC   = $(2))
$(eval A_MOD  = $(3))

$(eval A_BIN = $(A_ARCH)-$(A_CC)-$(A_MOD))

$(DSTDIR)/$(A_BIN): $(SRCDIR)/$(A_MOD).c
	$(BUILD_PY) --arch $(A_ARCH) --srcdir $(SRCDIR) --dstdir $(DSTDIR) \
		--sbtobjs=runtime \
		--cc $(A_CC) $(A_MOD).c -o $(A_BIN)

.PHONY: $(A_BIN)
$(A_BIN): $(DSTDIR)/$(A_BIN)

$(A_BIN)-run:
	$(RUN_PY) --arch $(A_ARCH) --dir $(DSTDIR) $(A_BIN)

endef

$(eval $(foreach arch,$(ARCHS),$(foreach cc,$(COMPILERS),\
$(call BLDNRUN,$(arch),$(cc),hello))))

.PHONY: ssh-copy
ssh-copy:
	ssh $(ARM) mkdir -p $(ARM_DSTDIR)
	scp $(foreach cc,$(COMPILERS),$(DSTDIR)/arm-$(cc)-hello) $(ARM):$(ARM_DSTDIR)

.PHONY: adb-copy
adb-copy:
	$(ADB) shell mkdir -p $(ADB_DSTDIR)
	$(ADB) push $(foreach cc,$(COMPILERS),$(DSTDIR)/arm-$(cc)-hello) $(ADB_DSTDIR)
