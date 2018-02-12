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

BIN := printf

test-prep:
	echo NOP

SRCDIR := $(TOPDIR)/mibench/automotive/basicmath
DSTDIR := $(TOPDIR)/build/mibench/automotive/basicmath

define COMPILE
$(DSTDIR)/$(1).bc: $(SRCDIR)/$(1).c
	clang -fno-rtti -fno-exceptions --target=riscv32 -D__riscv_xlen=32 \
		-isysroot /mnt/ssd/riscv-sbt/toolchain/release/opt/riscv/sysroot \
		-isystem /mnt/ssd/riscv-sbt/toolchain/release/opt/riscv/sysroot/usr/include \
		-emit-llvm -c -O1 -c $$< -o $$@
	llvm-dis $$@ -o $(DSTDIR)/$(1).ll

endef

MODS := cubic #basicmath_large rad2deg cubic isqrt

$(eval $(foreach mod,$(MODS),$(call COMPILE,$(mod))))

LLC := \
	llc -relocation-model=static -O1 -march=riscv32 -mattr=-a,-c,+m,+f,+d \
		$(DSTDIR)/cubic.bc -o $(DSTDIR)/cubic.s \
		-verify-machineinstrs -print-machineinstrs -debug-pass=Details -debug \
		-print-isel-input \
		# -print-before-all -print-after-all \
		# -view-dag-combine1-dags

$(DSTDIR)/basicmath: $(addsuffix .bc,$(addprefix $(DSTDIR)/,$(MODS)))
	#llvm-link $^ -o $(DSTDIR)/basicmath.bc
	#llvm-dis $(DSTDIR)/basicmath.bc -o $(DSTDIR)/basicmath.ll
	$(LLC) >& out.txt

.PHONY: test
test:
	rm -f $(DSTDIR)/*
	$(MAKE) $(DSTDIR)/basicmath

.PHONY: dbg
dbg:
	gdb --args $(LLC)
