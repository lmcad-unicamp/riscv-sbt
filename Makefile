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

### patch llvm (lowrisc) ###
## 1- update llvm and clang with upstream:
# cd $TOPDIR/submodules/llvm
# git checkout master
# git pull
# git fetch upstream
# git merge upstream/master
# git push
# # rename the lowrisc branch
# git branch -m lowrisc lowrisc-old
# # repeat for clang
#
# 2- checkout the target revision of llvm+clang:
# - Check it at https://github.com/lowRISC/riscv-llvm
# - Look for REV=123456
# - Use git log and look for git-svn-id and find the commit that corresponds to
#   REV, or the closest one not greater than REV
# git checkout -b lowrisc <REV-commit>
# # repeat for clang
#
# 3- update lowrisc-llvm
# cd $TOPDIR/submodules/lowrisc-llvm
# git pull
#
# 4- patch-llvm
# cd $TOPDIR
# make patch-llvm
#
# 5- commit and push the patched llvm/clang branches
# git add lib test
# git commit
# # LLVM patched with lowrisc/riscv-llvm
# # @<SHA>
# git push origin -d lowrisc
# git push -u origin lowrisc
#
# 6- add the updated submodules
# git add submodules/clang
# git add submodules/llvm
# git add submodules/lowrisc-llvm

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

test-prep:
	$(MAKE) -C mibench stringsearch-clean stringsearch

.PHONY: test
test: sbt-force
	riscv-sbt -x -regs=globals  -stack-size=131072 /mnt/ssd/riscv-sbt/build/mibench/office/stringsearch/rv32-stringsearch.o -o /mnt/ssd/riscv-sbt/build/mibench/office/stringsearch/rv32-x86-stringsearch-globals.bc >/mnt/ssd/riscv-sbt/junk/rv32-x86-stringsearch-globals.log 2>&1

.PHONY: dbg
dbg:
	gdb --args riscv-sbt -x -regs=globals  -stack-size=131072 /mnt/ssd/riscv-sbt/build/mibench/office/stringsearch/rv32-stringsearch.o -o /mnt/ssd/riscv-sbt/build/mibench/office/stringsearch/rv32-x86-stringsearch-globals.bc
