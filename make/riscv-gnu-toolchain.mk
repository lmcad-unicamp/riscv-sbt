###
### riscv-gnu-toolchain
###

RISCV_GNU_TOOLCHAIN_PREFIX := $(TOOLCHAIN_RELEASE)/opt/riscv

RISCV_GNU_TOOLCHAIN_BUILD := $(BUILD_DIR)/riscv-gnu-toolchain/newlib32
RISCV_GNU_TOOLCHAIN_DEPS := $(RISCV_GNU_TOOLCHAIN_BUILD)

$(RISCV_GNU_TOOLCHAIN_BUILD):
	mkdir -p $(BUILD_DIR)/riscv-gnu-toolchain
	cp -a $(SUBMODULES_DIR)/riscv-gnu-toolchain $@

RISCV_GNU_TOOLCHAIN_MAKEFILE := $(RISCV_GNU_TOOLCHAIN_BUILD)/Makefile
RISCV_GNU_TOOLCHAIN_CONFIGURE := ./configure \
                                 --prefix=$(RISCV_GNU_TOOLCHAIN_PREFIX) \
                                 --with-arch=rv32gc \
                                 --with-abi=ilp32d
RISCV_GNU_TOOLCHAIN_TOOLCHAIN := $(RISCV_GNU_TOOLCHAIN_PREFIX)/bin/riscv32-unknown-elf-gcc
RISCV_GNU_TOOLCHAIN_ALIAS := riscv-gnu-toolchain

$(eval $(call RULE_MAKEFILE,RISCV_GNU_TOOLCHAIN))

# build and install
$(RISCV_GNU_TOOLCHAIN_TOOLCHAIN): $(RISCV_GNU_TOOLCHAIN_MAKEFILE)
	@echo "*** $@ ***"
	$(MAKE) -C $(RISCV_GNU_TOOLCHAIN_BUILD) $(MAKE_OPTS)

$(eval $(call RULE_ALIAS,RISCV_GNU_TOOLCHAIN))
$(eval $(call RULE_CLEAN,RISCV_GNU_TOOLCHAIN))


###
### riscv-gnu-toolchain-linux
###

RISCV_GNU_TOOLCHAIN_LINUX_BUILD := $(BUILD_DIR)/riscv-gnu-toolchain/linux
RISCV_GNU_TOOLCHAIN_LINUX_DEPS := $(RISCV_GNU_TOOLCHAIN_LINUX_BUILD)

$(RISCV_GNU_TOOLCHAIN_LINUX_BUILD):
	mkdir -p $(BUILD_DIR)/riscv-gnu-toolchain
	cp -a $(SUBMODULES_DIR)/riscv-gnu-toolchain $@

RISCV_GNU_TOOLCHAIN_LINUX_MAKEFILE := $(RISCV_GNU_TOOLCHAIN_LINUX_BUILD)/Makefile
RISCV_GNU_TOOLCHAIN_LINUX_CONFIGURE := ./configure \
                                       --prefix=$(RISCV_GNU_TOOLCHAIN_PREFIX) \
                                       --enable-multilib
RISCV_GNU_TOOLCHAIN_LINUX_TOOLCHAIN := $(RISCV_GNU_TOOLCHAIN_PREFIX)/bin/riscv-gnu-linux-gcc
RISCV_GNU_TOOLCHAIN_LINUX_ALIAS := riscv-gnu-toolchain-linux

$(eval $(call RULE_MAKEFILE,RISCV_GNU_TOOLCHAIN_LINUX))

# build and install
$(RISCV_GNU_TOOLCHAIN_LINUX_TOOLCHAIN): $(RISCV_GNU_TOOLCHAIN_LINUX_MAKEFILE)
	@echo "*** $@ ***"
	$(MAKE) -C $(RISCV_GNU_TOOLCHAIN_LINUX_BUILD) $(MAKE_OPTS) linux

$(eval $(call RULE_ALIAS,RISCV_GNU_TOOLCHAIN_LINUX))
$(eval $(call RULE_CLEAN,RISCV_GNU_TOOLCHAIN_LINUX))
