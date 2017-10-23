ifeq ($(TOPDIR),)
$(error "TOPDIR not set. Please run '. scripts/env.sh' first.")
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
	FESVR \
	PK32 \
	QEMU_TESTS \
	QEMU_USER \
	SBT_DEBUG \
	SIM \


all: \
	$(SBT) \
	riscv-isa-sim \
	riscv-pk-32 \
	qemu-user \


include $(TOPDIR)/make/rules.mk
include $(TOPDIR)/make/build_pkg.mk
include $(TOPDIR)/make/riscv-gnu-toolchain.mk
include $(TOPDIR)/make/llvm.mk
include $(TOPDIR)/make/spike.mk
include $(TOPDIR)/make/qemu.mk
include $(TOPDIR)/make/sbt.mk
include $(TOPDIR)/make/tests.mk
include $(TOPDIR)/make/dbg.mk

###
# apply lowrisc patches

patch-llvm:
	set -e && \
		cd $(SUBMODULES_DIR)/llvm && \
		for P in ../lowrisc-llvm/*.patch; do \
			echo $$P; \
			patch -p1 < $$P; \
		done && \
		for P in ../lowrisc-llvm/clang/*.patch; do \
			echo $$P; \
			patch -d tools/clang -p1 < $$P; \
		done

###

### MIBENCH ###

.PHONY: mibench
mibench: sbt
	rm -f log.txt
	$(LOG) $(MAKE) -C $(TOPDIR)/mibench $(MIBENCHS)

mibench-clean:
	$(MAKE) -C $(TOPDIR)/mibench clean


###
### generate all rules
###

$(foreach prog,$(ALL),$(eval $(call RULE_ALL,$(prog))))


# clean all
clean: $(foreach prog,$(ALL),$($(prog)_ALIAS)-clean) riscv-pk-unpatch
	rm -rf $(BUILD_DIR)
	rm -rf $(TOOLCHAIN)/*
