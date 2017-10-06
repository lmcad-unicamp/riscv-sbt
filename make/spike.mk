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

