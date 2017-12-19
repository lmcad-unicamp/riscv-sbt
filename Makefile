ifeq ($(TOPDIR),)
$(error "TOPDIR not set. Please run '. scripts/env.sh' first.")
endif

BUILDPKG_PY := $(TOPDIR)/scripts/auto/build_pkg.py

all: sbt riscv-gnu-toolchain-linux spike qemu

riscv-gnu-toolchain-newlib:
	$(BUILDPKG_PY) $@ $(MAKE_OPTS)
riscv-gnu-toolchain-linux:
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
		cd submodules/llvm && \
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

###

clean:
	rm -rf $(TOPDIR)/build
	rm -rf $(TOPDIR)/toolchain/*

lc:
	cat $(TOPDIR)/sbt/*.h $(TOPDIR)/sbt/*.cpp $(TOPDIR)/sbt/*.s | wc -l

almost-alltests:
	cd $(TOPDIR)/test/sbt && ./genmake.py && make clean almost-alltests

alltests:
	cd $(TOPDIR)/test/sbt && ./genmake.py && make clean alltests

### dbg ###

.PHONY: test
test: sbt
	$(MAKE) mibench-clean mibench MIBENCHS=crc32
	$(MAKE) -C mibench crc32-test

.PHONY: dbg
dbg:
	gdb /mnt/ssd/riscv-sbt/build/mibench/telecomm/CRC32/rv32-x86-crc32-globals $(TOPDIR)/mibench/core
