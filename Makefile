TOPDIR ?= $(shell pwd)

BUILDPKG_PY := $(TOPDIR)/scripts/auto/build_pkg.py

all: sbt riscv-gnu-toolchain-linux spike qemu

riscv-gnu-toolchain-newlib:
	@$(BUILDPKG_PY) $@ $(MAKE_OPTS)
riscv-gnu-toolchain-linux:
	@$(BUILDPKG_PY) $@ $(MAKE_OPTS)

llvm:
	@$(BUILDPKG_PY) $@ $(MAKE_OPTS)

spike:
	@$(BUILDPKG_PY) $(MAKE_OPTS) riscv-isa-sim
	@$(BUILDPKG_PY) $(MAKE_OPTS) riscv-pk-32

qemu:
	@$(BUILDPKG_PY) $(MAKE_OPTS) qemu-user

.PHONY: sbt
sbt:
	@$(BUILDPKG_PY) $(MAKE_OPTS) sbt

sbt-force:
	@$(BUILDPKG_PY) $(MAKE_OPTS) -f sbt

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

.PHONY: test-prep
test-prep:
	cd $(TOPDIR)/mibench && ./genmake.py
	make -C $(TOPDIR)/mibench dijkstra-clean
	make -C $(TOPDIR)/mibench rv32-dijkstra x86-dijkstra rv32-x86-dijkstra-globals rv32-x86-dijkstra-locals

.PHONY: test
test:
	#/mnt/ssd/riscv-sbt/scripts/auto/measure.py /mnt/ssd/riscv-sbt/build/mibench/network/dijkstra dijkstra -n 1 --args /mnt/ssd/riscv-sbt/mibench/network/dijkstra/input.dat
	gdb --args /mnt/ssd/riscv-sbt/build/mibench/network/dijkstra/rv32-x86-dijkstra-globals /mnt/ssd/riscv-sbt/mibench/network/dijkstra/input.dat
