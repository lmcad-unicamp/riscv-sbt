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
		$(LINUX_GCC_STAGE1_OUT)/libexec/gcc/$(RV64_LINUX_TRIPLE)/7.1.1/as
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

