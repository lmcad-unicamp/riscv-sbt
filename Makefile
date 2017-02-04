MAKE_OPTS := -j9

# build type for LLVM and SBT (Release or Debug)
# WARNING: using Release LLVM builds with Debug SBT CAN cause problems!
BUILD_TYPE := Debug

ifeq ($(TOPDIR),)
$(error "TOPDIR not set. Please run 'source setenv.sh' first.")
endif

DIR_TOOLCHAIN := $(TOPDIR)/toolchain
DIR_TOOLCHAIN_X86 := $(DIR_TOOLCHAIN)/x86

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
	SBT

all: \
	sbt \
	riscv-isa-sim \
	riscv-pk-32

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
#	sbt


###
### rules
###

ALL_FILES := find \! -type d | sed "s@^\./@@; /^pkg\//d;" | sort
DIFF_FILTER := grep "^< " | sed "s/^< //"
NEW_FILES := bash -c \
             'diff <($(ALL_FILES)) <(cat pkg/all.files) | $(DIFF_FILTER)'

define UPDATE_FILES
	cd $(DIR_TOOLCHAIN) && \
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
	if [ ! -f $$(DIR_TOOLCHAIN)/pkg/all.files ]; then \
		mkdir -p $$(DIR_TOOLCHAIN)/pkg; \
		touch $$(DIR_TOOLCHAIN)/pkg/all.files; \
	fi
	cd $$(DIR_TOOLCHAIN) && \
	$$(NEW_FILES) > pkg/$$($(1)_ALIAS).files && \
	$(call UPDATE_FILES,$(1))

# install only when build OUTPUT changes
$$($(1)_TOOLCHAIN): $$($(1)_OUT)
	$(MAKE) $$($(1)_ALIAS)-install
	touch $$@
endef

# build and install
define RULE_BUILD_AND_INSTALL =
$$($(1)_OUT) $$($(1)_TOOLCHAIN): $$($(1)_MAKEFILE)
	$(MAKE) -C $$($(1)_BUILD) $(MAKE_OPTS)
endef

# alias to invoke to build and install target
define RULE_ALIAS =
.PHONY: $$($(1)_ALIAS)
$$($(1)_ALIAS): $$($(1)_TOOLCHAIN)
endef

# clean
define RULE_CLEAN =
$$($(1)_ALIAS)-clean:
	cd $(DIR_TOOLCHAIN) && \
		rm -f `cat pkg/$$($(1)_ALIAS).files` && \
		rm -f pkg/$$($(1)_ALIAS).files && \
		$(ALL_FILES) > pkg/all.files
	rm -rf $$($(1)_BUILD)
endef

# all rules above
define RULE_ALL =
$(call RULE_MAKEFILE,$(1))
ifneq ($$($(1)_BUILD_AND_INSTALL),)
$(call RULE_BUILD_AND_INSTALL,$(1))
else
$(call RULE_BUILD,$(1))
$(call RULE_INSTALL,$(1))
endif
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
BINUTILS32_TOOLCHAIN := $(DIR_TOOLCHAIN)/bin/riscv32-unknown-elf-ld
BINUTILS32_CONFIGURE := $(TOPDIR)/riscv-binutils-gdb/configure \
                      --target=riscv32-unknown-elf \
                      --prefix=$(DIR_TOOLCHAIN) \
                      --disable-werror
BINUTILS32_ALIAS := riscv-binutils-gdb-32

# 64 bit

BINUTILS64_BUILD := $(TOPDIR)/build/riscv-binutils-gdb/64
BINUTILS64_MAKEFILE := $(BINUTILS64_BUILD)/Makefile
BINUTILS64_OUT := $(BINUTILS64_BUILD)/ld/ld-new
BINUTILS64_TOOLCHAIN := $(DIR_TOOLCHAIN)/bin/riscv64-unknown-elf-ld
BINUTILS64_CONFIGURE := $(TOPDIR)/riscv-binutils-gdb/configure \
                      --target=riscv64-unknown-elf \
                      --prefix=$(DIR_TOOLCHAIN) \
                      --disable-werror
BINUTILS64_ALIAS := riscv-binutils-gdb-64

# x86

BINUTILS_X86_BUILD := $(TOPDIR)/build/x86-binutils-gdb
BINUTILS_X86_MAKEFILE := $(BINUTILS_X86_BUILD)/Makefile
BINUTILS_X86_OUT := $(BINUTILS_X86_BUILD)/ld/ld-new
BINUTILS_X86_TOOLCHAIN := $(DIR_TOOLCHAIN_X86)/bin/ld
BINUTILS_X86_CONFIGURE := $(TOPDIR)/riscv-binutils-gdb/configure \
                          --prefix=$(DIR_TOOLCHAIN_X86) \
                          --enable-multilib
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
NEWLIB_GCC32_TOOLCHAIN := $(DIR_TOOLCHAIN)/bin/riscv32-unknown-elf-gcc
NEWLIB_GCC32_CONFIGURE := $(TOPDIR)/riscv-newlib-gcc/configure \
                 --target=riscv32-unknown-elf \
                 --prefix=$(DIR_TOOLCHAIN) \
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
NEWLIB_GCC64_TOOLCHAIN := $(DIR_TOOLCHAIN)/bin/riscv64-unknown-elf-gcc
NEWLIB_GCC64_CONFIGURE := $(TOPDIR)/riscv-newlib-gcc/configure \
                 --target=riscv64-unknown-elf \
                 --prefix=$(DIR_TOOLCHAIN) \
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

###
### riscv-fesvr
###

FESVR_BUILD := $(TOPDIR)/build/riscv-fesvr
FESVR_MAKEFILE := $(FESVR_BUILD)/Makefile
FESVR_OUT := $(FESVR_BUILD)/libfesvr.so
FESVR_TOOLCHAIN := $(DIR_TOOLCHAIN)/lib/libfesvr.so
FESVR_CONFIGURE := $(TOPDIR)/riscv-fesvr/configure \
                   --prefix=$(DIR_TOOLCHAIN)
FESVR_ALIAS := riscv-fesvr

# riscv-isa-sim
SIM_BUILD := $(TOPDIR)/build/riscv-isa-sim
SIM_MAKEFILE := $(SIM_BUILD)/Makefile
SIM_OUT := $(SIM_BUILD)/spike
SIM_TOOLCHAIN := $(DIR_TOOLCHAIN)/bin/spike
SIM_CONFIGURE := $(TOPDIR)/riscv-isa-sim/configure \
                 --prefix=$(DIR_TOOLCHAIN) \
                 --with-fesvr=$(DIR_TOOLCHAIN) \
                 --with-isa=RV32IMAFDC
SIM_ALIAS := riscv-isa-sim
SIM_DEPS := $(FESVR_TOOLCHAIN)

###
### riscv-pk
###

# 32 bit

PK32_BUILD := $(TOPDIR)/build/riscv-pk/32
PK32_MAKEFILE := $(PK32_BUILD)/Makefile
PK32_OUT := $(PK32_BUILD)/pk
PK32_TOOLCHAIN := $(DIR_TOOLCHAIN)/riscv32-unknown-elf/bin/pk
PK32_CONFIGURE := $(TOPDIR)/riscv-pk/configure \
                  --prefix=$(DIR_TOOLCHAIN) \
                  --host=riscv32-unknown-elf \
                  --enable-32bit
PK32_ALIAS := riscv-pk-32

# 64 bit

PK64_BUILD := $(TOPDIR)/build/riscv-pk/64
PK64_MAKEFILE := $(PK64_BUILD)/Makefile
PK64_OUT := $(PK64_BUILD)/pk
PK64_TOOLCHAIN := $(DIR_TOOLCHAIN)/riscv64-unknown-elf/bin/pk
PK64_CONFIGURE := $(TOPDIR)/riscv-pk/configure \
                  --prefix=$(DIR_TOOLCHAIN) \
                  --host=riscv64-unknown-elf
PK64_ALIAS := riscv-pk-64

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
LLVM_DEBUG_TOOLCHAIN := $(DIR_TOOLCHAIN)/bin/clang
LLVM_DEBUG_CONFIGURE := \
    $(CMAKE) -DCMAKE_BUILD_TYPE=Debug \
             -DLLVM_TARGETS_TO_BUILD="ARM;RISCV;X86" \
             -DCMAKE_INSTALL_PREFIX=$(DIR_TOOLCHAIN) $(TOPDIR)/llvm
LLVM_DEBUG_ALIAS := llvm-debug
CLANG_LINK := $(TOPDIR)/llvm/tools/clang
LLVM_DEBUG_DEPS := $(NEWLIB_GCC32_TOOLCHAIN) $(CMAKE) $(CLANG_LINK)

$(CLANG_LINK):
	ln -sf $(TOPDIR)/clang $@

# release

LLVM_RELEASE_BUILD := $(TOPDIR)/build/llvm/release
LLVM_RELEASE_MAKEFILE := $(LLVM_RELEASE_BUILD)/Makefile
LLVM_RELEASE_OUT := $(LLVM_RELEASE_BUILD)/bin/clang
LLVM_RELEASE_TOOLCHAIN := $(DIR_TOOLCHAIN)/release/bin/clang
LLVM_RELEASE_CONFIGURE := \
    $(CMAKE) -DCMAKE_BUILD_TYPE=Release \
             -DLLVM_TARGETS_TO_BUILD="ARM;RISCV;X86" \
             -DCMAKE_INSTALL_PREFIX=$(DIR_TOOLCHAIN)/release $(TOPDIR)/llvm
LLVM_RELEASE_ALIAS := llvm-release
LLVM_RELEASE_DEPS := $(NEWLIB_GCC32_TOOLCHAIN) $(CMAKE) $(CLANG_LINK)

###
### sbt
###

SBT_BUILD := $(TOPDIR)/build/sbt
SBT_MAKEFILE := $(SBT_BUILD)/Makefile
SBT_OUT := $(SBT_BUILD)/riscv-sbt
SBT_TOOLCHAIN := $(DIR_TOOLCHAIN)/bin/riscv-sbt
SBT_CONFIGURE := \
    $(CMAKE) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
             -DCMAKE_INSTALL_PREFIX=$(DIR_TOOLCHAIN) $(TOPDIR)/sbt
SBT_ALIAS := sbt
ifeq ($(BUILD_TYPE),Debug)
SBT_DEPS := $(LLVM_DEBUG_TOOLCHAIN)
else
SBT_DEPS := $(LLVM_RELEASE_TOOLCHAIN)
endif

# generate all rules
$(foreach prog,$(ALL),$(eval $(call RULE_ALL,$(prog))))

# clean all
clean: $(foreach prog,$(ALL),$($(prog)_ALIAS)-clean)
	rm -rf $(DIR_TOOLCHAIN)

###
### TEST targets
###

all_files:
	cd $(DIR_TOOLCHAIN) && \
	$(ALL_FILES)

new_files:
	cd $(DIR_TOOLCHAIN) && \
	$(NEW_FILES)

$(eval UPDATE_FILE := $(call UPDATE_FILES,$(PKG)))
update_files:
	f="$(DIR_TOOLCHAIN)/pkg/$($(PKG)_ALIAS).files" && \
	if [ ! -f "$$f" ]; then \
		echo "Invalid PKG: \"$(PKG)\""; \
		exit 1; \
	fi
	$(UPDATE_FILE)

.PHONY: test
test:
	$(MAKE) sbt-build sbt-install
	rm -f $(TOPDIR)/sbt/test/rv-x86-main.bc
	$(MAKE) -C $(TOPDIR)/sbt/test
