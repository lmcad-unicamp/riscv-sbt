###
### QEMU
###

# user

QEMU_USER_BUILD := $(BUILD_DIR)/qemu-user
QEMU_USER_MAKEFILE := $(QEMU_USER_BUILD)/Makefile
QEMU_USER_OUT := $(QEMU_USER_BUILD)/qemu-riscv32
QEMU_USER_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/qemu-riscv32
QEMU_USER_CONFIGURE := \
    ln -sf $(SUBMODULES_DIR)/riscv-gnu-toolchain/riscv-qemu \
           $(SUBMODULES_DIR)/riscv-qemu && \
    $(SUBMODULES_DIR)/riscv-qemu/configure \
    --prefix=$(TOOLCHAIN_RELEASE) \
    --target-list=riscv64-linux-user,riscv32-linux-user
QEMU_USER_ALIAS := qemu-user

# system

QEMU_BUILD := $(BUILD_DIR)/qemu
QEMU_MAKEFILE := $(QEMU_BUILD)/Makefile
QEMU_OUT := $(QEMU_BUILD)/qemu-system-riscv32
QEMU_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/qemu-system-riscv32
QEMU_CONFIGURE := \
    ln -sf $(SUBMODULES_DIR)/riscv-gnu-toolchain/riscv-qemu \
           $(SUBMODULES_DIR)/riscv-qemu && \
    $(SUBMODULES_DIR)/riscv-qemu/configure \
    --prefix=$(TOOLCHAIN_RELEASE) \
    --target-list=riscv64-softmmu,riscv32-softmmu
QEMU_ALIAS := qemu
