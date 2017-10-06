ifeq ($(TOPDIR),)
$(error "TOPDIR not set. Please run '. scripts/env.sh' first.")
endif

include $(TOPDIR)/make/config.mk

# ALL := LOWRISC_LLVM_DEBUG

all: lowrisc-llvm-debug

include $(TOPDIR)/make/rules.mk
include $(TOPDIR)/make/build_pkg.mk
include $(TOPDIR)/make/riscv-gnu-toolchain.mk
include $(TOPDIR)/make/llvm.mk

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

# $(foreach prog,$(ALL),$(eval $(call RULE_ALL,$(prog))))

