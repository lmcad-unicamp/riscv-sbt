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

almost-alltests:
	cd $(TOPDIR)/test/sbt && ./genmake.py && make clean almost-alltests

alltests:
	cd $(TOPDIR)/test/sbt && ./genmake.py && make clean alltests

### dbg ###

.PHONY: test
test:
	valgrind --leak-check=yes riscv-sbt -debug -regs=globals -commented-asm \
		-a2s /mnt/ssd/riscv-sbt/build/test/sbt/rv32-mm.a2s \
		/mnt/ssd/riscv-sbt/build/test/sbt/rv32-mm.o \
		-o /mnt/ssd/riscv-sbt/build/test/sbt/rv32-x86-mm-globals.bc \
		>/mnt/ssd/riscv-sbt/junk/rv32-x86-mm-globals.log 2>&1
