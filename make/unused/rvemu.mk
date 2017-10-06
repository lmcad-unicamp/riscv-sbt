###
### riscvemu
###

RVEMU_PKG_FILE := riscvemu-2017-01-12.tar.gz
RVEMU_URL   := http://www.bellard.org/riscvemu/$(RVEMU_PKG_FILE)
RVEMU_PKG := $(REMOTE_DIR)/$(RVEMU_PKG_FILE)
RVEMU_SRC := $(BUILD_DIR)/riscvemu-2017-01-12

$(RVEMU_PKG):
	mkdir -p $(dir $@)
	wget $(RVEMU_URL) -O $@

$(RVEMU_SRC)/extracted: $(RVEMU_PKG)
	tar -C $(BUILD_DIR) -xvf $(RVEMU_PKG)
	touch $@

RVEMU_BUILD := $(RVEMU_SRC)
RVEMU_PKG_LINUX_FILE := diskimage-linux-riscv64-2017-01-07.tar.gz
RVEMU_LINUX_URL  := http://www.bellard.org/riscvemu/$(RVEMU_PKG_LINUX_FILE)
RVEMU_LINUX_PKG  := $(REMOTE_DIR)/$(RVEMU_PKG_LINUX_FILE)
RVEMU_LINUX_SRC  := $(RVEMU_BUILD)/diskimage-linux-riscv64-2017-01-07

RVEMU_MAKEFILE := $(RVEMU_BUILD)/Makefile
RVEMU_OUT := $(RVEMU_BUILD)/riscvemu
RVEMU_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/riscvemu
RVEMU_CONFIGURE := true
RVEMU_INSTALL   := cd $(RVEMU_BUILD) && \
  cp riscvemu riscvemu32 riscvemu64 riscvemu128 $(TOOLCHAIN_RELEASE)/bin/ && \
  DIR=$(TOOLCHAIN_RELEASE)/share/riscvemu && \
  mkdir -p $$DIR && \
  cp $(RVEMU_LINUX_SRC)/bbl.bin $(RVEMU_LINUX_SRC)/root.bin $$DIR && \
  F=$(TOOLCHAIN_RELEASE)/bin/riscvemu64_linux && \
  echo "riscvemu $$DIR/bbl.bin $$DIR/root.bin" > $$F && \
  chmod +x $$F
RVEMU_ALIAS := riscvemu
RVEMU_DEPS := $(RVEMU_SRC)/extracted $(RVEMU_LINUX_SRC)/extracted

$(RVEMU_LINUX_PKG):
	mkdir -p $(dir $@)
	wget $(RVEMU_LINUX_URL) -O $@

$(RVEMU_LINUX_SRC)/extracted: $(RVEMU_LINUX_PKG)
	mkdir -p $(RVEMU_BUILD)
	tar -C $(RVEMU_BUILD) -xvf $(RVEMU_LINUX_PKG)
	touch $@

