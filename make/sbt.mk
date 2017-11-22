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
             -DCMAKE_INSTALL_PREFIX=$(TOOLCHAIN_DEBUG) \
             $(TOPDIR)/sbt
SBT_DEBUG_ALIAS := sbt-debug
SBT_DEBUG_DEPS := $(LOWRISC_LLVM_DEBUG_TOOLCHAIN)

.PHONY: sbt
sbt: $(SBT)-build $(SBT)-install

