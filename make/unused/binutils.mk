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
