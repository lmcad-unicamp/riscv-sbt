###
### QEMU
###

# needs: libglib2.0-dev

# user

QEMU_USER_BUILD := $(BUILD_DIR)/qemu-user
QEMU_USER_MAKEFILE := $(QEMU_USER_BUILD)/Makefile
QEMU_USER_OUT := $(QEMU_USER_BUILD)/qemu-riscv32
QEMU_USER_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/qemu-riscv32
QEMU_USER_CONFIGURE := $(SUBMODULES_DIR)/riscv-qemu/configure \
  --prefix=$(TOOLCHAIN_RELEASE) \
  --target-list=riscv64-linux-user,riscv32-linux-user
QEMU_USER_ALIAS := qemu-user

# system

QEMU_BUILD := $(BUILD_DIR)/qemu
QEMU_MAKEFILE := $(QEMU_BUILD)/Makefile
QEMU_OUT := $(QEMU_BUILD)/qemu-system-riscv32
QEMU_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/qemu-system-riscv32
QEMU_CONFIGURE := $(SUBMODULES_DIR)/riscv-qemu/configure \
  --prefix=$(TOOLCHAIN_RELEASE) \
  --target-list=riscv64-softmmu,riscv32-softmmu
QEMU_ALIAS := qemu

# tests

QEMU_TESTS_BUILD := $(BUILD_DIR)/riscv-qemu-tests
QEMU_TESTS_MAKEFILE := $(QEMU_TESTS_BUILD)/Makefile

.PHONY: qemu-tests-reset
qemu-tests-reset:
	rm -rf $(QEMU_TESTS_BUILD)

.PHONY: qemu-tests-prepare
qemu-tests-prepare: qemu-tests-reset
	cp -a $(PATCHES_DIR)/riscv-qemu-tests $(QEMU_TESTS_BUILD)

QEMU_TESTS_CONFIGURE := \
    $(MAKE) -C $(TOPDIR) qemu-tests-prepare
QEMU_TESTS_MAKE_FLAGS := all32
QEMU_TESTS_OUT := $(QEMU_TESTS_BUILD)/test1
QEMU_TESTS_TOOLCHAIN := $(QEMU_TESTS_BUILD)/rv32i/add.o
QEMU_TESTS_INSTALL := true
QEMU_TESTS_ALIAS := qemu-tests
QEMU_TESTS_DEPS := $(BINUTILS_LINUX)

