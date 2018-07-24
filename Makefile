TOPDIR ?= $(shell pwd)

BUILDPKG_PY := $(TOPDIR)/scripts/auto/build_pkg.py

all: sbt riscv-gnu-toolchain-linux spike qemu

riscv-gnu-toolchain-newlib:
	@$(BUILDPKG_PY) $@ $(MAKE_OPTS)
riscv-gnu-toolchain-linux:
	@$(BUILDPKG_PY) $@ $(MAKE_OPTS)
riscv-gnu-toolchain-newlib-gcc7:
	@$(BUILDPKG_PY) $@ $(MAKE_OPTS)
riscv-gnu-toolchain-linux-gcc7:
	@$(BUILDPKG_PY) $@ $(MAKE_OPTS)

llvm:
	@$(BUILDPKG_PY) $@ $(MAKE_OPTS)
llvm-gcc7:
	@$(BUILDPKG_PY) $@ $(MAKE_OPTS)

spike:
	@$(BUILDPKG_PY) $(MAKE_OPTS) riscv-isa-sim
	@$(BUILDPKG_PY) $(MAKE_OPTS) riscv-pk-32

qemu:
	@$(BUILDPKG_PY) $(MAKE_OPTS) qemu-user

.PHONY: sbt
sbt:
	@$(BUILDPKG_PY) $(MAKE_OPTS) $@
sbt-gcc7:
	@$(BUILDPKG_PY) $(MAKE_OPTS) $@

sbt-force:
	@$(BUILDPKG_PY) $(MAKE_OPTS) -f sbt
sbt-clean:
	@$(BUILDPKG_PY) $(MAKE_OPTS) --clean sbt

### docker image

docker-img:
	cd docker && \
		export TOPDIR=$(TOPDIR) PYTHONPATH=$(TOPDIR)/scripts && \
		./build.py --get-srcs && \
		./build.py --build all

docker-rdev:
	cd docker && \
		export TOPDIR=$(TOPDIR) PYTHONPATH=$(TOPDIR)/scripts && \
		./build.py --rdev

docker-xdev:
	cd docker && \
		export TOPDIR=$(TOPDIR) PYTHONPATH=$(TOPDIR)/scripts && \
		./build.py --xdev

docker-rgcc7:
	cd docker && \
		export TOPDIR=$(TOPDIR) PYTHONPATH=$(TOPDIR)/scripts && \
		./build.py --bind --rgcc7

docker-xgcc7:
	cd docker && \
		export TOPDIR=$(TOPDIR) PYTHONPATH=$(TOPDIR)/scripts && \
		./build.py --bind --xgcc7

###

clean:
	rm -rf $(TOPDIR)/build
	rm -rf $(TOPDIR)/toolchain/*

lc:
	cat $(TOPDIR)/sbt/*.h $(TOPDIR)/sbt/*.cpp $(TOPDIR)/sbt/*.s | wc -l

lc-py:
	cat $(TOPDIR)/scripts/auto/*.py \
		$(TOPDIR)/test/sbt/genmake.py \
		$(TOPDIR)/mibench/genmake.py \
		$(TOPDIR)/docker/build.py \
		| wc -l

almost-alltests:
	cd $(TOPDIR)/test/sbt && ./genmake.py && make clean almost-alltests

alltests:
	cd $(TOPDIR)/test/sbt && ./genmake.py && make clean alltests

### dbg ###

TEST := crc32
TEST_DIR := telecomm/CRC32
TEST_FILE := telecomm/adpcm/data/large.pcm

.PHONY: test
test:
	cd $(TOPDIR)/mibench && ./genmake.py
	make -C $(TOPDIR)/mibench $(TEST)-clean
	make -C $(TOPDIR)/mibench rv32-$(TEST) x86-$(TEST) rv32-x86-$(TEST)-globals rv32-x86-$(TEST)-locals
	$(TOPDIR)/scripts/auto/measure.py $(TOPDIR)/build/mibench/$(TEST_DIR) $(TEST) -n 1 \
		--args $(TOPDIR)/mibench/$(TEST_FILE)
