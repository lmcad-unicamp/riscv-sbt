ifeq ($(TOPDIR),)
$(error "TOPDIR not set. Please run '. scripts/env.sh' first.")
endif

include $(TOPDIR)/make/config.mk

### apply lowrisc patches

all: sbt spike qemu

riscv-gnu-toolchain:
	$(BUILDPKG_PY) $@ $(MAKE_OPTS)

llvm:
	$(BUILDPKG_PY) $@ $(MAKE_OPTS)

spike:
	$(BUILDPKG_PY) $(MAKE_OPTS) riscv-isa-sim
	$(BUILDPKG_PY) $(MAKE_OPTS) riscv-pk-32

qemu:
	$(BUILDPKG_PY) $(MAKE_OPTS) qemu-user

.PHONY: sbt
sbt:
	$(BUILDPKG_PY) $(MAKE_OPTS) sbt

### patch llvm (lowrisc) ###

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

### docker image

docker-img:
	$(MAKE) -C docker

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(TOOLCHAIN)/*
