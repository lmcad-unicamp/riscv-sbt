ifeq ($(TOPDIR),)
$(error "TOPDIR not set. Please run 'source setenv.sh' first.")
endif

include $(TOPDIR)/config.mk

# build type for LLVM and SBT (Release or Debug)
# WARNING: using Release LLVM builds with Debug SBT CAN cause problems!
ifeq ($(BUILD_TYPE),Debug)
  SBT := sbt-debug
else
  SBT := sbt-release
endif

ALL := \
	BINUTILS32 \
	NEWLIB_GCC32 \
	FESVR \
	SIM \
	PK32 \
	BINUTILS64 \
	NEWLIB_GCC64 \
	PK64 \
	LLVM_DEBUG \
	LLVM_RELEASE \
	SBT_DEBUG \
	SBT_RELEASE \
	BINUTILS_X86 \
	NEWLIB_GCC_X86

all: \
	$(SBT) \
	riscv-isa-sim \
	riscv-pk-32 \
	x86-newlib-gcc

#
# all targets
#
#all: \
#	riscv-binutils-gdb-32 \
#	riscv-newlib-gcc-32 \
#	riscv-fesvr \
#	riscv-isa-sim \
#	riscv-pk-32 \
#	riscv-binutils-gdb-64 \
#	riscv-newlib-gcc-64 \
#	riscv-pk-64 \
#	llvm-debug \
#	llvm-release \
#	sbt-debug \
#	sbt-release \
#	x86-binutils-gdb \
#	x86-newlib-gcc


###
### rules
###

include $(TOPDIR)/rules.mk


ALL_FILES := find \! -type d | sed "s@^\./@@; /^pkg\//d;" | sort
DIFF_FILTER := grep "^< " | sed "s/^< //"
NEW_FILES := bash -c \
             'diff <($(ALL_FILES)) <(cat pkg/all.files) | $(DIFF_FILTER)'

define UPDATE_FILES
	cd $(TOOLCHAIN) && \
	cat pkg/$$($(1)_ALIAS).files >> pkg/all.files && \
	sort pkg/all.files -o pkg/all.files
endef

# gen MAKEFILE
define RULE_MAKEFILE =
$$($(1)_MAKEFILE): $$($(1)_DEPS)
	mkdir -p $$($(1)_BUILD)
	cd $$($(1)_BUILD) && \
	$$($(1)_CONFIGURE)
	touch $$@
endef

# build
define RULE_BUILD =
# unconditional build
.PHONY: $$($(1)_ALIAS)-build
$$($(1)_ALIAS)-build:
	$(MAKE) -C $$($(1)_BUILD) $$($(1)_MAKE_FLAGS) $(MAKE_OPTS)

# serial build
.PHONY: $$($(1)_ALIAS)-build1
$$($(1)_ALIAS)-build1:
	$(MAKE) -C $$($(1)_BUILD)

# build only when MAKEFILE changes
$$($(1)_OUT): $$($(1)_MAKEFILE)
	$(MAKE) $$($(1)_ALIAS)-build
	touch $$@
endef

# install
define RULE_INSTALL =
# unconditional install
.PHONY: $$($(1)_ALIAS)-install
$$($(1)_ALIAS)-install:
	$(MAKE) -C $$($(1)_BUILD) install
	echo "Updating file list..."
	if [ ! -f $$(TOOLCHAIN)/pkg/all.files ]; then \
		mkdir -p $$(TOOLCHAIN)/pkg; \
		touch $$(TOOLCHAIN)/pkg/all.files; \
	fi
	cd $$(TOOLCHAIN) && \
	$$(NEW_FILES) > pkg/$$($(1)_ALIAS).files && \
	$(call UPDATE_FILES,$(1))

# install only when build OUTPUT changes
$$($(1)_TOOLCHAIN): $$($(1)_OUT)
	$(MAKE) $$($(1)_ALIAS)-install
	touch $$@
endef

# alias to invoke to build and install target
define RULE_ALIAS =
.PHONY: $$($(1)_ALIAS)
$$($(1)_ALIAS): $$($(1)_TOOLCHAIN)
endef

# clean
define RULE_CLEAN =
$$($(1)_ALIAS)-clean:
	cd $(TOOLCHAIN) && \
		if [ -f pkg/$$($(1)_ALIAS).files ]; then \
			rm -f `cat pkg/$$($(1)_ALIAS).files` && \
			rm -f pkg/$$($(1)_ALIAS).files && \
			$(ALL_FILES) > pkg/all.files; \
		fi || true
	rm -rf $$($(1)_BUILD)
endef

# all rules above
define RULE_ALL =
$(call RULE_MAKEFILE,$(1))
$(call RULE_BUILD,$(1))
$(call RULE_INSTALL,$(1))
$(call RULE_ALIAS,$(1))
$(call RULE_CLEAN,$(1))
endef

###
### riscv-binutils-gdb
###

# 32 bit

BINUTILS32_BUILD := $(TOPDIR)/build/riscv-binutils-gdb/32
BINUTILS32_MAKEFILE := $(BINUTILS32_BUILD)/Makefile
BINUTILS32_OUT := $(BINUTILS32_BUILD)/ld/ld-new
BINUTILS32_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/$(RV32_TRIPLE)-ld
BINUTILS32_CONFIGURE := $(TOPDIR)/riscv-binutils-gdb/configure \
                      --target=$(RV32_TRIPLE) \
                      --prefix=$(TOOLCHAIN_RELEASE) \
                      --disable-werror
BINUTILS32_ALIAS := riscv-binutils-gdb-32

# 64 bit

BINUTILS64_BUILD := $(TOPDIR)/build/riscv-binutils-gdb/64
BINUTILS64_MAKEFILE := $(BINUTILS64_BUILD)/Makefile
BINUTILS64_OUT := $(BINUTILS64_BUILD)/ld/ld-new
BINUTILS64_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/$(RV64_TRIPLE)-ld
BINUTILS64_CONFIGURE := $(TOPDIR)/riscv-binutils-gdb/configure \
                      --target=$(RV64_TRIPLE) \
                      --prefix=$(TOOLCHAIN_RELEASE) \
                      --disable-werror
BINUTILS64_ALIAS := riscv-binutils-gdb-64

# x86

BINUTILS_X86_BUILD := $(TOPDIR)/build/x86-binutils-gdb
BINUTILS_X86_MAKEFILE := $(BINUTILS_X86_BUILD)/Makefile
BINUTILS_X86_OUT := $(BINUTILS_X86_BUILD)/ld/ld-new
BINUTILS_X86_TOOLCHAIN := $(TOOLCHAIN_X86)/bin/$(X86_TRIPLE)-ld
BINUTILS_X86_CONFIGURE := $(TOPDIR)/riscv-binutils-gdb/configure \
                          --target=$(X86_TRIPLE) \
                          --prefix=$(TOOLCHAIN_X86)
BINUTILS_X86_ALIAS := x86-binutils-gdb

###
### riscv-newlib-gcc
###

SRC_NEWLIB_GCC := $(TOPDIR)/riscv-newlib-gcc
$(SRC_NEWLIB_GCC):
	cp -a $(TOPDIR)/riscv-gcc $@.tmp
	cp -a $(TOPDIR)/riscv-newlib/. $@.tmp
	cp -a $(TOPDIR)/riscv-gcc/include/. $@.tmp/include
	mv $@.tmp $@

# 32 bit

NEWLIB_GCC32_BUILD := $(TOPDIR)/build/riscv-newlib-gcc/32
NEWLIB_GCC32_MAKEFILE := $(NEWLIB_GCC32_BUILD)/Makefile
NEWLIB_GCC32_OUT := $(NEWLIB_GCC32_BUILD)/gcc/xgcc
NEWLIB_GCC32_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/$(RV32_TRIPLE)-gcc
NEWLIB_GCC32_CONFIGURE := $(TOPDIR)/riscv-newlib-gcc/configure \
                 --target=$(RV32_TRIPLE) \
                 --prefix=$(TOOLCHAIN_RELEASE) \
                 --without-headers \
                 --disable-shared \
                 --disable-threads \
                 --enable-languages=c,c++ \
                 --with-system-zlib \
                 --enable-tls \
                 --with-newlib \
                 --disable-libmudflap \
                 --disable-libssp \
                 --disable-libquadmath \
                 --disable-libgomp \
                 --disable-nls \
                 --enable-multilib=no \
                 --enable-checking=yes \
                 --with-abi=ilp32d \
                 --with-arch=rv32g
NEWLIB_GCC32_MAKE_FLAGS := inhibit-libc=true
NEWLIB_GCC32_ALIAS := riscv-newlib-gcc-32
NEWLIB_GCC32_DEPS := $(BINUTILS32_TOOLCHAIN) $(SRC_NEWLIB_GCC)

# 64 bit

NEWLIB_GCC64_BUILD := $(TOPDIR)/build/riscv-newlib-gcc/64
NEWLIB_GCC64_MAKEFILE := $(NEWLIB_GCC64_BUILD)/Makefile
NEWLIB_GCC64_OUT := $(NEWLIB_GCC64_BUILD)/gcc/xgcc
NEWLIB_GCC64_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/$(RV64_TRIPLE)-gcc
NEWLIB_GCC64_CONFIGURE := $(TOPDIR)/riscv-newlib-gcc/configure \
                 --target=$(RV64_TRIPLE) \
                 --prefix=$(TOOLCHAIN_RELEASE) \
                 --without-headers \
                 --disable-shared \
                 --disable-threads \
                 --enable-languages=c,c++ \
                 --with-system-zlib \
                 --enable-tls \
                 --with-newlib \
                 --disable-libmudflap \
                 --disable-libssp \
                 --disable-libquadmath \
                 --disable-libgomp \
                 --disable-nls \
                 --enable-multilib=no \
                 --enable-checking=yes \
                 --with-abi=lp64d \
                 --with-arch=rv64g
NEWLIB_GCC64_MAKE_FLAGS := inhibit-libc=true
NEWLIB_GCC64_ALIAS := riscv-newlib-gcc-64
NEWLIB_GCC64_DEPS := $(BINUTILS64_TOOLCHAIN) $(SRC_NEWLIB_GCC)

# x86

NEWLIB_GCC_X86_BUILD := $(TOPDIR)/build/x86-newlib-gcc
NEWLIB_GCC_X86_MAKEFILE := $(NEWLIB_GCC_X86_BUILD)/Makefile
NEWLIB_GCC_X86_OUT := $(NEWLIB_GCC_X86_BUILD)/gcc/xgcc
NEWLIB_GCC_X86_TOOLCHAIN := $(TOOLCHAIN_X86)/bin/$(X86_TRIPLE)-gcc
NEWLIB_GCC_X86_CONFIGURE := $(TOPDIR)/riscv-newlib-gcc/configure \
                 --target=$(X86_TRIPLE) \
                 --prefix=$(TOOLCHAIN_X86) \
                 --without-headers \
                 --disable-shared \
                 --disable-threads \
                 --enable-languages=c,c++ \
                 --with-system-zlib \
                 --enable-tls \
                 --with-newlib \
                 --disable-libmudflap \
                 --disable-libssp \
                 --disable-libquadmath \
                 --disable-libgomp \
                 --disable-nls \
                 --enable-multilib=no
NEWLIB_GCC_X86_ALIAS := x86-newlib-gcc
NEWLIB_GCC_X86_DEPS := $(BINUTILS_X86_TOOLCHAIN)

###
### riscv-fesvr
###

FESVR_BUILD := $(TOPDIR)/build/riscv-fesvr
FESVR_MAKEFILE := $(FESVR_BUILD)/Makefile
FESVR_OUT := $(FESVR_BUILD)/libfesvr.so
FESVR_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/lib/libfesvr.so
FESVR_CONFIGURE := $(TOPDIR)/riscv-fesvr/configure \
                   --prefix=$(TOOLCHAIN_RELEASE)
FESVR_ALIAS := riscv-fesvr

# riscv-isa-sim
SIM_BUILD := $(TOPDIR)/build/riscv-isa-sim
SIM_MAKEFILE := $(SIM_BUILD)/Makefile
SIM_OUT := $(SIM_BUILD)/spike
SIM_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/spike
SIM_CONFIGURE := $(TOPDIR)/riscv-isa-sim/configure \
                 --prefix=$(TOOLCHAIN_RELEASE) \
                 --with-fesvr=$(TOOLCHAIN_RELEASE) \
                 --with-isa=RV32IMAFDC
SIM_ALIAS := riscv-isa-sim
SIM_DEPS := $(FESVR_TOOLCHAIN)

###
### riscv-pk
###

# 32 bit

PK_PATCHED := $(TOPDIR)/riscv-pk/.patched
$(PK_PATCHED): $(TOPDIR)/riscv-pk-32-bit-build-fix.patch
	cd $(TOPDIR)/riscv-pk && patch < $<
	@touch $@

PK32_BUILD := $(TOPDIR)/build/riscv-pk/32
PK32_MAKEFILE := $(PK32_BUILD)/Makefile
PK32_OUT := $(PK32_BUILD)/pk
PK32_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/$(RV32_TRIPLE)/bin/pk
PK32_CONFIGURE := $(TOPDIR)/riscv-pk/configure \
                  --prefix=$(TOOLCHAIN_RELEASE) \
                  --host=$(RV32_TRIPLE) \
                  --enable-32bit
PK32_ALIAS := riscv-pk-32
PK32_DEPS := $(PK_PATCHED)

# 64 bit

PK64_BUILD := $(TOPDIR)/build/riscv-pk/64
PK64_MAKEFILE := $(PK64_BUILD)/Makefile
PK64_OUT := $(PK64_BUILD)/pk
PK64_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/$(RV64_TRIPLE)/bin/pk
PK64_CONFIGURE := $(TOPDIR)/riscv-pk/configure \
                  --prefix=$(TOOLCHAIN_RELEASE) \
                  --host=$(RV64_TRIPLE)
PK64_ALIAS := riscv-pk-64
PK64_DEPS := $(PK_PATCHED)

###
### cmake
###

CMAKE_URL := http://www.cmake.org/files/v3.5/cmake-3.5.2-Linux-x86_64.tar.gz
CMAKE_ROOT := $(TOPDIR)/cmake
CMAKE_PKG := $(CMAKE_ROOT)/cmake-3.5.2-Linux-x86_64.tar.gz
CMAKE := $(CMAKE_ROOT)/bin/cmake

$(CMAKE_PKG):
	mkdir -p $(CMAKE_ROOT)
	cd $(CMAKE_ROOT) && \
	wget $(CMAKE_URL)

$(CMAKE): $(CMAKE_PKG)
	tar --strip-components=1 -xvf $(CMAKE_PKG) -C $(CMAKE_ROOT)
	touch $(CMAKE)

.PHONY: cmake
cmake: $(CMAKE)

###
### llvm
###

# debug

LLVM_DEBUG_BUILD := $(TOPDIR)/build/llvm/debug
LLVM_DEBUG_MAKEFILE := $(LLVM_DEBUG_BUILD)/Makefile
LLVM_DEBUG_OUT := $(LLVM_DEBUG_BUILD)/bin/clang
LLVM_DEBUG_TOOLCHAIN := $(TOOLCHAIN_DEBUG)/bin/clang
LLVM_DEBUG_CONFIGURE := \
    $(CMAKE) -DCMAKE_BUILD_TYPE=Debug \
             -DLLVM_TARGETS_TO_BUILD="ARM;RISCV;X86" \
             -DCMAKE_INSTALL_PREFIX=$(TOOLCHAIN_DEBUG) $(TOPDIR)/llvm
LLVM_DEBUG_ALIAS := llvm-debug
CLANG_LINK := $(TOPDIR)/llvm/tools/clang
LLVM_DEBUG_DEPS := $(NEWLIB_GCC32_TOOLCHAIN) $(CMAKE) $(CLANG_LINK)

$(CLANG_LINK):
	ln -sf $(TOPDIR)/clang $@

# release

LLVM_RELEASE_BUILD := $(TOPDIR)/build/llvm/release
LLVM_RELEASE_MAKEFILE := $(LLVM_RELEASE_BUILD)/Makefile
LLVM_RELEASE_OUT := $(LLVM_RELEASE_BUILD)/bin/clang
LLVM_RELEASE_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/clang
LLVM_RELEASE_CONFIGURE := \
    $(CMAKE) -DCMAKE_BUILD_TYPE=Release \
             -DLLVM_TARGETS_TO_BUILD="ARM;RISCV;X86" \
             -DCMAKE_INSTALL_PREFIX=$(TOOLCHAIN_RELEASE) $(TOPDIR)/llvm
LLVM_RELEASE_ALIAS := llvm-release
LLVM_RELEASE_DEPS := $(NEWLIB_GCC32_TOOLCHAIN) $(CMAKE) $(CLANG_LINK)

###
### sbt
###

# debug

SBT_DEBUG_BUILD := $(TOPDIR)/build/sbt/debug
SBT_DEBUG_MAKEFILE := $(SBT_DEBUG_BUILD)/Makefile
SBT_DEBUG_OUT := $(SBT_DEBUG_BUILD)/riscv-sbt
SBT_DEBUG_TOOLCHAIN := $(TOOLCHAIN_DEBUG)/bin/riscv-sbt
SBT_DEBUG_CONFIGURE := \
    $(CMAKE) -DCMAKE_BUILD_TYPE=Debug \
             -DCMAKE_INSTALL_PREFIX=$(TOOLCHAIN_DEBUG) $(TOPDIR)/sbt
SBT_DEBUG_ALIAS := sbt-debug
SBT_DEBUG_DEPS := $(LLVM_DEBUG_TOOLCHAIN)

# release

SBT_RELEASE_BUILD := $(TOPDIR)/build/sbt/release
SBT_RELEASE_MAKEFILE := $(SBT_RELEASE_BUILD)/Makefile
SBT_RELEASE_OUT := $(SBT_RELEASE_BUILD)/riscv-sbt
SBT_RELEASE_TOOLCHAIN := $(TOOLCHAIN_RELEASE)/bin/riscv-sbt
SBT_RELEASE_CONFIGURE := \
    $(CMAKE) -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_INSTALL_PREFIX=$(TOOLCHAIN_RELEASE) $(TOPDIR)/sbt
SBT_RELEASE_ALIAS := sbt-release
SBT_RELEASE_DEPS := $(LLVM_RELEASE_TOOLCHAIN)

###
### generate all rules
###

$(foreach prog,$(ALL),$(eval $(call RULE_ALL,$(prog))))

# clean all
clean: $(foreach prog,$(ALL),$($(prog)_ALIAS)-clean)
	rm -rf $(TOOLCHAIN)

###
### TEST targets
###

all_files:
	cd $(TOOLCHAIN) && \
	$(ALL_FILES)

new_files:
	cd $(TOOLCHAIN) && \
	$(NEW_FILES)

$(eval UPDATE_FILE := $(call UPDATE_FILES,$(PKG)))
update_files:
	f="$(TOOLCHAIN)/pkg/$($(PKG)_ALIAS).files" && \
	if [ ! -f "$$f" ]; then \
		echo "Invalid PKG: \"$(PKG)\""; \
		exit 1; \
	fi
	$(UPDATE_FILE)

lc:
	cat $(TOPDIR)/sbt/*.h $(TOPDIR)/sbt/*.cpp | wc -l

.PHONY: test
test:
	$(MAKE) $(SBT)-build1 $(SBT)-install
	#rm -f $(TOPDIR)/sbt/test/rv32-x86-main.bc
	#$(MAKE) -C $(TOPDIR)/sbt/test rv32-x86-main
