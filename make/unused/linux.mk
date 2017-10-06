###
### linux
###

LINUX_PKG_FILE := linux-4.6.2.tar.xz
LINUX_URL := https://cdn.kernel.org/pub/linux/kernel/v4.x/$(LINUX_PKG_FILE)
LINUX_PKG := $(REMOTE_DIR)/$(LINUX_PKG_FILE)
LINUX_DIR := $(TOPDIR)/riscv-linux
LINUX_SRC := $(BUILD_DIR)/linux-4.6.2

LINUX_BUILD := $(LINUX_SRC)
LINUX_MAKEFILE := $(LINUX_BUILD)/Makefile
LINUX_OUT := $(LINUX_BUILD)/vmlinux
LINUX_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/$(RV64_TRIPLE)/bin/vmlinux
LINUX_CONFIGURE := cd $(LINUX_BUILD) && \
                   $(MAKE) ARCH=riscv defconfig && \
                   cp $(LINUX_DIR)/riscv.config .config
LINUX_MAKE_FLAGS := ARCH=riscv vmlinux
LINUX_INSTALL := cp $(LINUX_OUT) $(LINUX_TOOLCHAIN)
LINUX_ALIAS := riscv-linux
LINUX_DEPS  := $(RVEMU_TOOLCHAIN) $(LINUX_SRC)/stamps/initramfs

$(LINUX_PKG):
	mkdir -p $(dir $@)
	wget $(LINUX_URL) -O $@

$(LINUX_SRC)/stamps/source: $(LINUX_PKG)
	tar -C $(BUILD_DIR) -xvf $(LINUX_PKG)
	cd $(LINUX_SRC) && \
	git init && \
	git remote add -t master origin https://github.com/riscv/riscv-linux.git && \
	git fetch && \
	git checkout -f -t origin/master
	mkdir -p $(dir $@) && touch $@

$(LINUX_SRC)/stamps/initramfs: $(LINUX_SRC)/stamps/source
	mkdir -p $(LINUX_BUILD)/rootfs
	cd $(LINUX_BUILD) && \
	sudo rm -rf initramfs && \
	sudo mount -o loop $(ROOT_FS) rootfs && \
	sudo cp -a rootfs initramfs && \
	sudo umount rootfs && \
	sudo cp $(LINUX_DIR)/init.sh initramfs/init && \
	sudo rm -rf initramfs/lost+found && \
	mkdir -p $(dir $@) && touch $@

