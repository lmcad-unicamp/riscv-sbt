ifeq ($(TOPDIR),)
$(error "TOPDIR not set. Please run 'source scripts/setenv.sh' first.")
endif

include $(TOPDIR)/make/config.mk

# build type for LLVM and SBT (Release or Debug)
# WARNING: using Release LLVM builds with Debug SBT CAN cause problems!
ifeq ($(BUILD_TYPE),Debug)
  SBT := sbt-debug
else
  SBT := sbt-release
endif

ALL := \
	BINUTILS32 \
	NEWLIB_GCC32 \
	FESVR \
	SIM \
	PK32 \
	BINUTILS64 \
	NEWLIB_GCC64 \
	PK64 \
	LLVM_DEBUG \
	LLVM_RELEASE \
	SBT_DEBUG \
	SBT_RELEASE \
	BINUTILS_X86 \
	NEWLIB_GCC_X86 \
	RVEMU \
	QEMU \
	QEMU_USER \
	QEMU_TESTS \
	BINUTILS_LINUX \
	LINUX_GCC \
	LINUX

all: \
	$(SBT) \
	riscv-isa-sim \
	riscv-pk-32 \
	x86-newlib-gcc

gcc64: riscv-newlib-gcc-64
gcc-linux: riscv-linux-gcc

extra: \
	gcc64 \
	gcc-linux \
	riscvemu \
	riscv-linux \
	riscv-pk-64 \
	qemu \
	qemu-user

###
### rules
###

include $(TOPDIR)/make/rules.mk


ALL_FILES := find \! -type d | sed "s@^\./@@; /^pkg\//d;" | sort
DIFF_FILTER := grep "^< " | sed "s/^< //"
NEW_FILES := bash -c \
             'diff <($(ALL_FILES)) <(cat pkg/all.files) | $(DIFF_FILTER)'

define UPDATE_FILES
	cd $(TOOLCHAIN) && \
	cat pkg/$$($(1)_ALIAS).files >> pkg/all.files && \
	sort pkg/all.files -o pkg/all.files
endef

# gen MAKEFILE
define RULE_MAKEFILE =
$$($(1)_MAKEFILE): $$($(1)_DEPS)
	@echo "*** $$@: $$? ***"
	if [ ! -d "$$($(1)_BUILD)" ]; then mkdir -p $$($(1)_BUILD); fi
	cd $$($(1)_BUILD) && \
	$$($(1)_CONFIGURE)
	touch $$@
endef

# build
define RULE_BUILD =
# unconditional build
.PHONY: $$($(1)_ALIAS)-build
$$($(1)_ALIAS)-build:
	@echo "*** $$@ ***"
	$(MAKE) -C $$($(1)_BUILD) $$($(1)_MAKE_FLAGS) $(MAKE_OPTS)

# serial build
.PHONY: $$($(1)_ALIAS)-build1
$$($(1)_ALIAS)-build1:
	$(MAKE) -C $$($(1)_BUILD)

# build only when MAKEFILE changes
$$($(1)_OUT): $$($(1)_MAKEFILE)
	$(MAKE) $$($(1)_ALIAS)-build
	touch $$@
endef

# install
define RULE_INSTALL =
# unconditional install
.PHONY: $$($(1)_ALIAS)-install
$$($(1)_ALIAS)-install:
	@echo "*** $$@ ***"
ifeq ($$($(1)_INSTALL),)
	$(MAKE) -C $$($(1)_BUILD) install
else
	$$($(1)_INSTALL)
endif
ifneq ($$($(1)_POSTINSTALL),)
	$$($(1)_POSTINSTALL)
endif
	echo "Updating file list..."
	if [ -f $$(TOOLCHAIN)/pkg/$$($(1)_ALIAS).files ]; then \
		echo "Skipped: file list already exists"; \
	else \
		if [ ! -f $$(TOOLCHAIN)/pkg/all.files ]; then \
			mkdir -p $$(TOOLCHAIN)/pkg; \
			touch $$(TOOLCHAIN)/pkg/all.files; \
		fi; \
		cd $$(TOOLCHAIN) && \
		$$(NEW_FILES) > pkg/$$($(1)_ALIAS).files && \
		$(call UPDATE_FILES,$(1)); \
	fi

# install only when build OUTPUT changes
$$($(1)_TOOLCHAIN): $$($(1)_OUT)
	$(MAKE) $$($(1)_ALIAS)-install
	touch $$@
endef

# alias to invoke to build and install target
define RULE_ALIAS =
.PHONY: $$($(1)_ALIAS)
$$($(1)_ALIAS): $$($(1)_TOOLCHAIN)
endef

# clean
define RULE_CLEAN =
$$($(1)_ALIAS)-clean:
	cd $(TOOLCHAIN) && \
		if [ -f pkg/$$($(1)_ALIAS).files ]; then \
			rm -f `cat pkg/$$($(1)_ALIAS).files` && \
			rm -f pkg/$$($(1)_ALIAS).files && \
			$(ALL_FILES) > pkg/all.files; \
		fi || true
	rm -rf $$($(1)_BUILD)
endef

# all rules above
define RULE_ALL =
$(call RULE_MAKEFILE,$(1))
$(call RULE_BUILD,$(1))
$(call RULE_INSTALL,$(1))
$(call RULE_ALIAS,$(1))
$(call RULE_CLEAN,$(1))
endef

###
### riscv-binutils-gdb
###

# 32 bit

BINUTILS32_BUILD := $(BUILD_DIR)/riscv-binutils-gdb/32
BINUTILS32_MAKEFILE := $(BINUTILS32_BUILD)/Makefile
BINUTILS32_OUT := $(BINUTILS32_BUILD)/ld/ld-new
BINUTILS32_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/$(RV32_TRIPLE)-ld
BINUTILS32_CONFIGURE := $(SUBMODULES_DIR)/riscv-binutils-gdb/configure \
                      --target=$(RV32_TRIPLE) \
                      --prefix=$(TOOLCHAIN_RELEASE) \
                      --disable-werror
BINUTILS32_ALIAS := riscv-binutils-gdb-32

# 64 bit

BINUTILS64_BUILD := $(BUILD_DIR)/riscv-binutils-gdb/64
BINUTILS64_MAKEFILE := $(BINUTILS64_BUILD)/Makefile
BINUTILS64_OUT := $(BINUTILS64_BUILD)/ld/ld-new
BINUTILS64_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/$(RV64_TRIPLE)-ld
BINUTILS64_CONFIGURE := $(SUBMODULES_DIR)/riscv-binutils-gdb/configure \
                      --target=$(RV64_TRIPLE) \
                      --prefix=$(TOOLCHAIN_RELEASE) \
                      --disable-werror
BINUTILS64_ALIAS := riscv-binutils-gdb-64

# x86

BINUTILS_X86_BUILD := $(BUILD_DIR)/x86-binutils-gdb
BINUTILS_X86_MAKEFILE := $(BINUTILS_X86_BUILD)/Makefile
BINUTILS_X86_OUT := $(BINUTILS_X86_BUILD)/ld/ld-new
BINUTILS_X86_TOOLCHAIN := $(TOOLCHAIN_X86)/bin/$(X86_TRIPLE)-ld
BINUTILS_X86_CONFIGURE := $(SUBMODULES_DIR)/riscv-binutils-gdb/configure \
                          --target=$(X86_TRIPLE) \
                          --prefix=$(TOOLCHAIN_X86)
BINUTILS_X86_ALIAS := x86-binutils-gdb

# linux

SYSROOT := $(TOOLCHAIN_RELEASE)/sysroot

BINUTILS_LINUX_BUILD := $(BUILD_DIR)/riscv-binutils-gdb/linux
BINUTILS_LINUX_MAKEFILE := $(BINUTILS_LINUX_BUILD)/Makefile
BINUTILS_LINUX_OUT := $(BINUTILS_LINUX_BUILD)/ld/ld-new
BINUTILS_LINUX_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/$(RV64_LINUX_TRIPLE)-ld
BINUTILS_LINUX_CONFIGURE := $(SUBMODULES_DIR)/riscv-binutils-gdb/configure \
                      --target=$(RV64_LINUX_TRIPLE) \
                      --prefix=$(TOOLCHAIN_RELEASE) \
                      --with-sysroot=$(SYSROOT) \
                      --enable-multilib \
                      --disable-werror \
                      --disable-nls
BINUTILS_LINUX_ALIAS := riscv-binutils-gdb-linux

BINUTILS_LINUX_POSTINSTALL := \
  mkdir -p $(SYSROOT)/usr/ && \
  cp -a $(SUBMODULES_DIR)/riscv-gnu-toolchain/linux-headers/include $(SYSROOT)/usr/


###
### riscv-newlib-gcc
###

SRC_NEWLIB_GCC := $(BUILD_DIR)/riscv-newlib-gcc
$(SRC_NEWLIB_GCC)/stamp:
	$(eval $@_DIR := $(shell dirname $@))
	cp -a $(SUBMODULES_DIR)/riscv-gcc $($@_DIR).tmp
	cp -a $(SUBMODULES_DIR)/riscv-newlib/. $($@_DIR).tmp
	cp -a $(SUBMODULES_DIR)/riscv-gcc/include/. $($@_DIR).tmp/include
	mv $($@_DIR).tmp $($@_DIR) && touch $@

# 32 bit

NEWLIB_GCC32_BUILD := $(BUILD_DIR)/riscv-newlib-gcc/32
NEWLIB_GCC32_MAKEFILE := $(NEWLIB_GCC32_BUILD)/Makefile
NEWLIB_GCC32_OUT := $(NEWLIB_GCC32_BUILD)/gcc/xgcc
NEWLIB_GCC32_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/$(RV32_TRIPLE)-gcc
NEWLIB_GCC32_CONFIGURE := $(BUILD_DIR)/riscv-newlib-gcc/configure \
                 --target=$(RV32_TRIPLE) \
                 --prefix=$(TOOLCHAIN_RELEASE) \
                 --without-headers \
                 --disable-shared \
                 --disable-threads \
                 --enable-languages=c,c++ \
                 --with-system-zlib \
                 --enable-tls \
                 --with-newlib \
                 --disable-libmudflap \
                 --disable-libssp \
                 --disable-libquadmath \
                 --disable-libgomp \
                 --disable-nls \
                 --enable-multilib=no \
                 --enable-checking=yes \
                 --with-abi=ilp32d \
                 --with-arch=rv32g
NEWLIB_GCC32_MAKE_FLAGS := inhibit-libc=true
NEWLIB_GCC32_ALIAS := riscv-newlib-gcc-32
NEWLIB_GCC32_DEPS := $(BINUTILS32_TOOLCHAIN) $(SRC_NEWLIB_GCC)/stamp

# 64 bit

NEWLIB_GCC64_BUILD := $(BUILD_DIR)/riscv-newlib-gcc/64
NEWLIB_GCC64_MAKEFILE := $(NEWLIB_GCC64_BUILD)/Makefile
NEWLIB_GCC64_OUT := $(NEWLIB_GCC64_BUILD)/gcc/xgcc
NEWLIB_GCC64_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/$(RV64_TRIPLE)-gcc
NEWLIB_GCC64_CONFIGURE := $(BUILD_DIR)/riscv-newlib-gcc/configure \
                 --target=$(RV64_TRIPLE) \
                 --prefix=$(TOOLCHAIN_RELEASE) \
                 --without-headers \
                 --disable-shared \
                 --disable-threads \
                 --enable-languages=c,c++ \
                 --with-system-zlib \
                 --enable-tls \
                 --with-newlib \
                 --disable-libmudflap \
                 --disable-libssp \
                 --disable-libquadmath \
                 --disable-libgomp \
                 --disable-nls \
                 --enable-multilib=no \
                 --enable-checking=yes \
                 --with-abi=lp64d \
                 --with-arch=rv64g
NEWLIB_GCC64_MAKE_FLAGS := inhibit-libc=true
NEWLIB_GCC64_ALIAS := riscv-newlib-gcc-64
NEWLIB_GCC64_DEPS := $(BINUTILS64_TOOLCHAIN) $(SRC_NEWLIB_GCC)/stamp

# x86

NEWLIB_GCC_X86_BUILD := $(BUILD_DIR)/x86-newlib-gcc
NEWLIB_GCC_X86_MAKEFILE := $(NEWLIB_GCC_X86_BUILD)/Makefile
NEWLIB_GCC_X86_OUT := $(NEWLIB_GCC_X86_BUILD)/gcc/xgcc
NEWLIB_GCC_X86_TOOLCHAIN := $(TOOLCHAIN_X86)/bin/$(X86_TRIPLE)-gcc
NEWLIB_GCC_X86_CONFIGURE := $(BUILD_DIR)/riscv-newlib-gcc/configure \
                 --target=$(X86_TRIPLE) \
                 --prefix=$(TOOLCHAIN_X86) \
                 --without-headers \
                 --disable-shared \
                 --disable-threads \
                 --enable-languages=c,c++ \
                 --with-system-zlib \
                 --enable-tls \
                 --with-newlib \
                 --disable-libmudflap \
                 --disable-libssp \
                 --disable-libquadmath \
                 --disable-libgomp \
                 --disable-nls \
                 --enable-multilib=no
NEWLIB_GCC_X86_ALIAS := x86-newlib-gcc
NEWLIB_GCC_X86_DEPS := $(BINUTILS_X86_TOOLCHAIN)

###
### riscv-linux-gcc
###

# multilib

LINUX_GCC_BASE  := $(BUILD_DIR)/riscv-linux-gcc
LINUX_GCC_STAGE2 := $(LINUX_GCC_BASE)/stage2
LINUX_GCC_BUILD  := $(LINUX_GCC_STAGE2)
LINUX_GCC_MAKEFILE := $(LINUX_GCC_BUILD)/Makefile
LINUX_GCC_OUT := $(LINUX_GCC_BUILD)/gcc/xgcc
LINUX_GCC_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/$(RV64_LINUX_TRIPLE)-gcc
LINUX_GCC_CONFIGURE := \
    cd $(LINUX_GCC_BUILD) && \
    $(SUBMODULES_DIR)/riscv-gcc/configure \
    --target=$(RV64_LINUX_TRIPLE) \
    --prefix=$(TOOLCHAIN_RELEASE) \
    --with-sysroot=$(SYSROOT) \
    --with-system-zlib \
    --enable-shared \
    --enable-tls \
    --enable-languages=c,c++ \
    --disable-libmudflap \
    --disable-libssp \
    --disable-libquadmath \
    --disable-nls \
    --disable-bootstrap \
    --enable-checking=yes \
    --enable-multilib \
    --with-abi=lp64d \
    --with-arch=rv64g
LINUX_GCC_ALIAS := riscv-linux-gcc

MULTILIB_NAMES := rv32ima-ilp32 rv32imafd-ilp32d rv64ima-lp64 rv64imafd-lp64d
LINUX_GCC_STAMPS := $(LINUX_GCC_BASE)/stamps
LINUX_GCC_DEPS := \
  $(addprefix $(LINUX_GCC_STAMPS)/build-glibc-linux-,$(MULTILIB_NAMES)) \
  $(LINUX_GCC_STAMPS)/build-glibc-linux-headers

LINUX_GCC_POSTINSTALL := \
  cp -a $(TOOLCHAIN_RELEASE)/$(RV64_LINUX_TRIPLE)/lib* $(SYSROOT)

#
# stage1
#

LINUX_GCC_STAGE1 := $(LINUX_GCC_BASE)/stage1
LINUX_GCC_STAGE1_OUT := $(LINUX_GCC_STAGE1)/output
$(LINUX_GCC_STAMPS)/build-gcc-linux-stage1: $(BINUTILS_LINUX_TOOLCHAIN)
	mkdir -p $(LINUX_GCC_STAGE1)
	cd $(LINUX_GCC_STAGE1) && \
	$(SUBMODULES_DIR)/riscv-gcc/configure \
		--target=$(RV64_LINUX_TRIPLE) \
		--prefix=$(LINUX_GCC_STAGE1_OUT) \
		--with-sysroot=$(SYSROOT) \
		--with-newlib \
		--without-headers \
		--disable-shared \
		--disable-threads \
		--with-system-zlib \
		--enable-tls \
		--enable-languages=c \
		--disable-libatomic \
		--disable-libmudflap \
		--disable-libssp \
		--disable-libquadmath \
		--disable-libgomp \
		--disable-nls \
		--disable-bootstrap \
		--enable-checking=yes \
		--enable-multilib \
		--with-abi=lp64d \
		--with-arch=rv64g
	$(MAKE) $(MAKE_OPTS) -C $(LINUX_GCC_STAGE1) inhibit-libc=true all-gcc
	$(MAKE) -C $(LINUX_GCC_STAGE1) inhibit-libc=true install-gcc
	$(MAKE) $(MAKE_OPTS) -C $(LINUX_GCC_STAGE1) inhibit-libc=true all-target-libgcc
	$(MAKE) -C $(LINUX_GCC_STAGE1) inhibit-libc=true install-target-libgcc
	ln -sf $(TOOLCHAIN_RELEASE)/bin/$(RV64_LINUX_TRIPLE)-as \
		$(LINUX_GCC_STAGE1_OUT)/libexec/gcc/$(RV64_LINUX_TRIPLE)/7.0.0/as
	mkdir -p $(dir $@) && touch $@

#
# glibc
#

LINUX_GLIBC := $(LINUX_GCC_BASE)/glibc
LINUX_GLIBC_HEADERS := $(LINUX_GCC_BASE)/glibc/headers
GLIBC_CC_FOR_TARGET := $(LINUX_GCC_STAGE1_OUT)/bin/$(RV64_LINUX_TRIPLE)-gcc

$(LINUX_GCC_STAMPS)/build-glibc-linux-%: \
    $(LINUX_GCC_STAMPS)/build-gcc-linux-stage1
	$(eval $@_ARCH := $(word 4,$(subst -, ,$(notdir $@))))
	$(eval $@_ABI := $(word 5,$(subst -, ,$(notdir $@))))
	$(eval $@_LIBDIRSUFFIX := $(if $($@_ABI),$(shell echo $($@_ARCH) | \
		sed 's/.*rv\([0-9]*\).*/\1/')/$($@_ABI),))
	$(eval $@_XLEN := $(if $($@_ABI),$(shell echo $($@_ARCH) | \
		sed 's/.*rv\([0-9]*\).*/\1/'),$(XLEN)))
	$(eval $@_CFLAGS := $(if $($@_ABI),-march=$($@_ARCH) -mabi=$($@_ABI),))
	$(eval $@_LIBDIROPTS := $(if $@_LIBDIRSUFFIX,--libdir=/usr/lib$($@_LIBDIRSUFFIX) libc_cv_slibdir=/lib$($@_LIBDIRSUFFIX) libc_cv_rtlddir=/lib,))
	$(eval $@_BUILDDIR := $(LINUX_GLIBC)/$($@_ARCH)-$($@_ABI))
	mkdir -p $($@_BUILDDIR)
	cd $($@_BUILDDIR) && \
		CC="$(GLIBC_CC_FOR_TARGET) $($@_CFLAGS)" \
		CFLAGS="-g -O2 $($@_CFLAGS)" \
		ASFLAGS="$($@_CFLAGS)" \
		$(SUBMODULES_DIR)/riscv-glibc/configure \
		--host=riscv$($@_XLEN)-unknown-linux-gnu \
		--prefix=/usr \
		--disable-werror \
		--enable-shared \
		--with-headers=$(SUBMODULES_DIR)/riscv-gnu-toolchain/linux-headers/include \
		--enable-multilib \
		--enable-kernel=3.0.0 \
		$($@_LIBDIROPTS)
	$(MAKE) $(MAKE_OPTS) -C $($@_BUILDDIR)
	+flock $(SYSROOT)/.lock $(MAKE) -C $($@_BUILDDIR) install install_root=$(SYSROOT)
	mkdir -p $(dir $@) && touch $@


$(LINUX_GCC_STAMPS)/build-glibc-linux-headers: \
    $(LINUX_GCC_STAMPS)/build-gcc-linux-stage1
	mkdir -p $(LINUX_GLIBC_HEADERS)
	cd $(LINUX_GLIBC_HEADERS) && \
	CC="$(LINUX_GCC_STAGE1_OUT)/bin/$(RV64_LINUX_TRIPLE)-gcc" \
	$(SUBMODULES_DIR)/riscv-glibc/configure \
		--host=$(RV64_LINUX_TRIPLE) \
		--prefix=$(SYSROOT)/usr \
		--enable-shared \
		--with-headers=$(SUBMODULES_DIR)/riscv-gnu-toolchain/linux-headers/include \
		--disable-multilib \
		--enable-kernel=3.0.0
	$(MAKE) -C $(LINUX_GLIBC_HEADERS) install-headers
	mkdir -p $(dir $@) && touch $@

###
### riscv-fesvr
###

FESVR_BUILD := $(BUILD_DIR)/riscv-fesvr
FESVR_MAKEFILE := $(FESVR_BUILD)/Makefile
FESVR_OUT := $(FESVR_BUILD)/libfesvr.so
FESVR_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/lib/libfesvr.so
FESVR_CONFIGURE := $(SUBMODULES_DIR)/riscv-fesvr/configure \
                   --prefix=$(TOOLCHAIN_RELEASE)
FESVR_ALIAS := riscv-fesvr

# riscv-isa-sim
SIM_BUILD := $(BUILD_DIR)/riscv-isa-sim
SIM_MAKEFILE := $(SIM_BUILD)/Makefile
SIM_OUT := $(SIM_BUILD)/spike
SIM_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/spike
SIM_CONFIGURE := $(SUBMODULES_DIR)/riscv-isa-sim/configure \
                 --prefix=$(TOOLCHAIN_RELEASE) \
                 --with-fesvr=$(TOOLCHAIN_RELEASE) \
                 --with-isa=RV32IMAFDC
SIM_ALIAS := riscv-isa-sim
SIM_DEPS := $(FESVR_TOOLCHAIN)

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

###
### riscv-pk
###

# 32 bit

PK_PATCHED := $(BUILD_DIR)/riscv-pk/.patched
$(PK_PATCHED): $(PATCHES_DIR)/riscv-pk-32-bit-build-fix.patch
	cd $(SUBMODULES_DIR)/riscv-pk && patch < $<
	mkdir -p $(dir $@) && touch $@

.PHONY: riscv-pk-unpatch
riscv-pk-unpatch:
	cd $(SUBMODULES_DIR)/riscv-pk && \
		git checkout configure configure.ac && \
		rm -f $(PK_PATCHED)

PK32_BUILD := $(BUILD_DIR)/riscv-pk/32
PK32_MAKEFILE := $(PK32_BUILD)/Makefile
PK32_OUT := $(PK32_BUILD)/pk
PK32_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/$(RV32_TRIPLE)/bin/pk
PK32_CONFIGURE := $(SUBMODULES_DIR)/riscv-pk/configure \
                  --prefix=$(TOOLCHAIN_RELEASE) \
                  --host=$(RV32_TRIPLE) \
                  --enable-32bit
PK32_ALIAS := riscv-pk-32
PK32_DEPS := $(PK_PATCHED)

# 64 bit

PK64_BUILD := $(BUILD_DIR)/riscv-pk/64
PK64_MAKEFILE := $(PK64_BUILD)/Makefile
PK64_OUT := $(PK64_BUILD)/pk
PK64_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/$(RV64_TRIPLE)/bin/pk
PK64_CONFIGURE := $(SUBMODULES_DIR)/riscv-pk/configure \
                  --prefix=$(TOOLCHAIN_RELEASE) \
                  --host=$(RV64_TRIPLE) \
                  --with-payload=$(LINUX_TOOLCHAIN)
PK64_ALIAS := riscv-pk-64
PK64_DEPS := $(PK_PATCHED) $(LINUX_TOOLCHAIN)

###
### cmake
###

CMAKE_PKG_FILE := cmake-3.5.2-Linux-x86_64.tar.gz
CMAKE_URL := http://www.cmake.org/files/v3.5/$(CMAKE_PKG_FILE)
CMAKE_PKG := $(REMOTE_DIR)/$(CMAKE_PKG_FILE)
CMAKE_BUILD := $(BUILD_DIR)/cmake
CMAKE := $(CMAKE_BUILD)/bin/cmake

$(CMAKE_PKG):
	mkdir -p $(dir $@)
	wget $(CMAKE_URL) -O $@

$(CMAKE): $(CMAKE_PKG)
	mkdir -p $(CMAKE_BUILD)
	tar --strip-components=1 -xvf $(CMAKE_PKG) -C $(CMAKE_BUILD)
	touch $(CMAKE)

.PHONY: cmake
cmake: $(CMAKE)

###
### llvm
###

# debug

LLVM_DEBUG_BUILD := $(BUILD_DIR)/llvm/debug
LLVM_DEBUG_MAKEFILE := $(LLVM_DEBUG_BUILD)/Makefile
LLVM_DEBUG_OUT := $(LLVM_DEBUG_BUILD)/bin/clang
LLVM_DEBUG_TOOLCHAIN := $(TOOLCHAIN_DEBUG)/bin/clang
LLVM_DEBUG_CONFIGURE := \
    $(CMAKE) -DCMAKE_BUILD_TYPE=Debug \
             -DLLVM_TARGETS_TO_BUILD="ARM;RISCV;X86" \
             -DBUILD_SHARED_LIBS=ON \
             -DCMAKE_INSTALL_PREFIX=$(TOOLCHAIN_DEBUG) $(SUBMODULES_DIR)/llvm
LLVM_DEBUG_ALIAS := llvm-debug
CLANG_LINK := $(SUBMODULES_DIR)/llvm/tools/clang
LLVM_DEBUG_DEPS := $(NEWLIB_GCC32_TOOLCHAIN) $(CMAKE) $(CLANG_LINK)

LLVM_DEBUG_POSTINSTALL := \
  SRC=$(LLVM_DEBUG_BUILD)/lib/Target/RISCVMaster && \
  DST=$(TOOLCHAIN_DEBUG)/include/llvm/Target/RISCVMaster && \
  mkdir -p $$DST && \
  (for f in RISCVMasterGenInstrInfo.inc RISCVMasterGenRegisterInfo.inc; do \
    cp $$SRC/$$f $$DST/$$f || exit 1; \
  done)

$(CLANG_LINK):
	ln -sf $(SUBMODULES_DIR)/clang $@

# release

LLVM_RELEASE_BUILD := $(BUILD_DIR)/llvm/release
LLVM_RELEASE_MAKEFILE := $(LLVM_RELEASE_BUILD)/Makefile
LLVM_RELEASE_OUT := $(LLVM_RELEASE_BUILD)/bin/clang
LLVM_RELEASE_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/clang
LLVM_RELEASE_CONFIGURE := \
    $(CMAKE) -DCMAKE_BUILD_TYPE=Release \
             -DLLVM_TARGETS_TO_BUILD="ARM;RISCV;X86" \
             -DCMAKE_INSTALL_PREFIX=$(TOOLCHAIN_RELEASE) $(SUBMODULES_DIR)/llvm
LLVM_RELEASE_ALIAS := llvm-release
LLVM_RELEASE_DEPS := $(NEWLIB_GCC32_TOOLCHAIN) $(CMAKE) $(CLANG_LINK)

LLVM_RELEASE_POSTINSTALL := \
  SRC=$(LLVM_RELEASE_BUILD)/lib/Target/RISCVMaster && \
  DST=$(TOOLCHAIN_RELEASE)/include/llvm/Target/RISCVMaster && \
  mkdir -p $$DST && \
  (for f in RISCVMasterGenInstrInfo.inc RISCVMasterGenRegisterInfo.inc; do \
    cp $$SRC/$$f $$DST/$$f || exit 1; \
  done)

###
### sbt
###

# debug

SBT_DEBUG_BUILD := $(BUILD_DIR)/sbt/debug
SBT_DEBUG_MAKEFILE := $(SBT_DEBUG_BUILD)/Makefile
SBT_DEBUG_OUT := $(SBT_DEBUG_BUILD)/riscv-sbt
SBT_DEBUG_TOOLCHAIN := $(TOOLCHAIN_DEBUG)/bin/riscv-sbt
SBT_DEBUG_CONFIGURE := \
    $(CMAKE) -DCMAKE_BUILD_TYPE=Debug \
             -DCMAKE_INSTALL_PREFIX=$(TOOLCHAIN_DEBUG) $(TOPDIR)/sbt
SBT_DEBUG_ALIAS := sbt-debug
SBT_DEBUG_DEPS := $(LLVM_DEBUG_TOOLCHAIN)

# release

SBT_RELEASE_BUILD := $(BUILD_DIR)/sbt/release
SBT_RELEASE_MAKEFILE := $(SBT_RELEASE_BUILD)/Makefile
SBT_RELEASE_OUT := $(SBT_RELEASE_BUILD)/riscv-sbt
SBT_RELEASE_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/riscv-sbt
SBT_RELEASE_CONFIGURE := \
    $(CMAKE) -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_INSTALL_PREFIX=$(TOOLCHAIN_RELEASE) $(TOPDIR)/sbt
SBT_RELEASE_ALIAS := sbt-release
SBT_RELEASE_DEPS := $(LLVM_RELEASE_TOOLCHAIN)

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

###
### QEMU
###

# system

QEMU_BUILD := $(BUILD_DIR)/qemu
QEMU_MAKEFILE := $(QEMU_BUILD)/Makefile
QEMU_OUT := $(QEMU_BUILD)/qemu-system-riscv32
QEMU_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/qemu-system-riscv32
QEMU_CONFIGURE := $(SUBMODULES_DIR)/riscv-qemu/configure \
  --prefix=$(TOOLCHAIN_RELEASE) \
  --target-list=riscv64-softmmu,riscv32-softmmu
QEMU_ALIAS := qemu

# user

QEMU_USER_BUILD := $(BUILD_DIR)/qemu-user
QEMU_USER_MAKEFILE := $(QEMU_USER_BUILD)/Makefile
QEMU_USER_OUT := $(QEMU_USER_BUILD)/qemu-riscv32
QEMU_USER_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/qemu-riscv32
QEMU_USER_CONFIGURE := $(SUBMODULES_DIR)/riscv-qemu/configure \
  --prefix=$(TOOLCHAIN_RELEASE) \
  --target-list=riscv64-linux-user,riscv32-linux-user
QEMU_USER_ALIAS := qemu-user

# tests

QEMU_TESTS_BUILD := $(BUILD_DIR)/riscv-qemu-tests
QEMU_TESTS_MAKEFILE := $(QEMU_TESTS_BUILD)/Makefile
QEMU_TESTS_CONFIGURE := \
  rm -rf $(QEMU_TESTS_BUILD) && \
  cp -a $(SUBMODULES_DIR)/riscv-qemu-tests $(QEMU_TESTS_BUILD) && \
  sed -i "s/^AS = \$$(CROSS)as -m32/AS = \$$(CROSS)as -march=rv32g/" \
    $(QEMU_TESTS_BUILD)/common32.mk
QEMU_TESTS_MAKE_FLAGS := all32
QEMU_TESTS_OUT := $(QEMU_TESTS_BUILD)/test1
QEMU_TESTS_TOOLCHAIN := $(QEMU_TESTS_BUILD)/rv32i/add.o
QEMU_TESTS_INSTALL := true
QEMU_TESTS_ALIAS := qemu-tests

###
### generate all rules
###

$(foreach prog,$(ALL),$(eval $(call RULE_ALL,$(prog))))

# clean all
clean: $(foreach prog,$(ALL),$($(prog)_ALIAS)-clean)
	rm -rf $(BUILD_DIR)
	rm -rf $(TOOLCHAIN)/*

###
### TEST targets
###

all_files:
	cd $(TOOLCHAIN) && \
	$(ALL_FILES)

new_files:
	cd $(TOOLCHAIN) && \
	$(NEW_FILES)

$(eval UPDATE_FILE := $(call UPDATE_FILES,$(PKG)))
update_files:
	f="$(TOOLCHAIN)/pkg/$($(PKG)_ALIAS).files" && \
	if [ ! -f "$$f" ]; then \
		echo "Invalid PKG: \"$(PKG)\""; \
		exit 1; \
	fi
	$(UPDATE_FILE)

lc:
	cat $(TOPDIR)/sbt/*.h $(TOPDIR)/sbt/*.cpp $(TOPDIR)/sbt/*.s | wc -l

.PHONY: test
test:
	$(MAKE) $(SBT)-build1 $(SBT)-install
	$(MAKE) -C $(TOPDIR)/sbt/test tests run-tests

.PHONY: tests
tests:
	$(MAKE) -C $(TOPDIR)/test clean all run
	$(MAKE) -C $(TOPDIR)/sbt/test clean all run tests run-tests

.PHONY: rv32tests
rv32tests: $(QEMU_TESTS_TOOLCHAIN)
	$(MAKE) $(SBT)-build1 $(SBT)-install
	cd $(QEMU_TESTS_BUILD)/rv32i && \
		riscv-sbt add.o && \
		llc -O0 -o rv32-x86-add.s -march x86 x86-add.bc && \
		$(X86_AS) -o rv32-x86-add.o -c rv32-x86-add.s && \
		$(X86_LD) -o rv32-x86-add rv32-x86-add.o \
			$(X86_SYSCALL_O) $(X86_RVSC_O) $(TOPDIR)/sbt/test/x86-dummy.o
