ifeq ($(TOPDIR),)
$(error "TOPDIR not set. Please run '. scripts/env.sh' first.")
endif

include $(TOPDIR)/make/config.mk

### apply lowrisc patches

all: sbt

.PHONY: sbt
sbt:
	@echo TODO

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

### docke image

docker-img:
	$(MAKE) -C docker

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(TOOLCHAIN)/*

###

#include $(TOPDIR)/make/build_pkg.mk
#include $(TOPDIR)/make/riscv-gnu-toolchain.mk
#include $(TOPDIR)/make/llvm.mk
#include $(TOPDIR)/make/spike.mk
#include $(TOPDIR)/make/qemu.mk
#include $(TOPDIR)/make/sbt.mk
#include $(TOPDIR)/make/dbg.mk

