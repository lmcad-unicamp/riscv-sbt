ifeq ($(TOPDIR),)
$(error "TOPDIR not set. Please run '. scripts/env.sh' first.")
endif

include $(TOPDIR)/make/config.mk

ALL :=

all: riscv-gnu-toolchain

include $(TOPDIR)/make/rules.mk
include $(TOPDIR)/make/build_pkg.mk
include $(TOPDIR)/make/riscv-gnu-toolchain.mk

###
### generate all rules
###

#$(foreach prog,$(ALL),$(eval $(call RULE_ALL,$(prog))))

