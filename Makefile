ifeq ($(TOPDIR),)
$(error "TOPDIR not set. Please run '. scripts/env.sh' first.")
endif

include $(TOPDIR)/make/config.mk

ALL := LLVM

all: riscv-gnu-toolchain

include $(TOPDIR)/make/rules.mk
include $(TOPDIR)/make/build_pkg.mk
include $(TOPDIR)/make/riscv-gnu-toolchain.mk

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

###
### generate all rules
###

#$(foreach prog,$(ALL),$(eval $(call RULE_ALL,$(prog))))

