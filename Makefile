ifeq ($(TOPDIR),)
$(error "TOPDIR not set. Please run '. scripts/env.sh' first.")
endif

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
		./build.py --get-srcs && \
		./build.py --build all

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

BIN := argv

test-prep:
	$(MAKE) -C $(TOPDIR)/test/sbt $(BIN)

.PHONY: test
test: sbt-force
	riscv-sbt -x -regs=globals /mnt/ssd/riscv-sbt/build/test/sbt/rv32-$(BIN).o -o /mnt/ssd/riscv-sbt/build/test/sbt/rv32-x86-$(BIN)-globals.bc >/mnt/ssd/riscv-sbt/junk/rv32-x86-$(BIN)-globals.log 2>&1

.PHONY: dbg
dbg:
	gdb --args riscv-sbt -x ...
