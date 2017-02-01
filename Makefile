MAKE_OPTS := -j9

# build type for LLVM and SBT (Release or Debug)
# WARNING: using Release LLVM builds with Debug SBT CAN cause problems!
BUILD_TYPE := Debug
LLVM_BUILD_TYPE := $(BUILD_TYPE)

ifeq ($(TOPDIR),)
$(error "TOPDIR not set. Please run 'source setenv.sh' first.")
endif

DIR_TOOLCHAIN := $(TOPDIR)/toolchain
DIR_TOOLCHAIN_X86 := $(DIR_TOOLCHAIN)/x86

ALL := BINUTILS NEWLIB_GCC FESVR SIM PK32 PK64 #LLVM SBT
all: riscv-newlib-gcc #riscv-isa-sim riscv-pk


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

# riscv
BINUTILS_BUILD := $(TOPDIR)/build/riscv-binutils-gdb
BINUTILS_MAKEFILE := $(BINUTILS_BUILD)/Makefile
BINUTILS_OUT := $(BINUTILS_BUILD)/ld/ld-new
BINUTILS_TOOLCHAIN := $(DIR_TOOLCHAIN)/bin/riscv64-unknown-elf-ld
BINUTILS_CONFIGURE := $(TOPDIR)/riscv-binutils-gdb/configure \
                      --target=riscv64-unknown-elf \
                      --prefix=$(DIR_TOOLCHAIN) \
                      --disable-werror
BINUTILS_ALIAS := riscv-binutils-gdb

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

NEWLIB_GCC_BUILD := $(TOPDIR)/build/riscv-newlib-gcc
NEWLIB_GCC_MAKEFILE := $(NEWLIB_GCC_BUILD)/Makefile
NEWLIB_GCC_OUT := $(NEWLIB_GCC_BUILD)/gcc/xgcc
NEWLIB_GCC_TOOLCHAIN := $(DIR_TOOLCHAIN)/bin/riscv64-unknown-elf-gcc
NEWLIB_GCC_CONFIGURE := $(TOPDIR)/riscv-newlib-gcc/configure \
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
                 --enable-multilib \
                 --enable-checking=yes \
                 --with-abi=lp64d \
                 --with-arch=rv64g
NEWLIB_GCC_MAKE_FLAGS := inhibit-libc=true
NEWLIB_GCC_ALIAS := riscv-newlib-gcc
NEWLIB_GCC_DEPS := $(BINUTILS_TOOLCHAIN) $(SRC_NEWLIB_GCC)


# riscv-fesvr
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
                 --with-fesvr=$(DIR_TOOLCHAIN)
SIM_ALIAS := riscv-isa-sim
SIM_DEPS := $(FESVR_TOOLCHAIN)

###
### riscv-pk
###

PK_HOST := riscv64-unknown-elf
PK_BUILD := $(TOPDIR)/build/riscv-pk

PK32_BUILD := $(PK_BUILD)/32
PK32_MAKEFILE := $(PK32_BUILD)/Makefile
PK32_OUT := $(PK32_BUILD)/pk
PK32_TOOLCHAIN := $(DIR_TOOLCHAIN)/riscv32-unknown-elf/bin/pk
PK32_CONFIGURE := $(TOPDIR)/riscv-pk/configure \
                  --prefix=$(DIR_TOOLCHAIN) \
                  --host=$(PK_HOST) \
                  --enable-32bit
PK32_ALIAS := riscv-pk32

PK64_BUILD := $(PK_BUILD)/64
PK64_MAKEFILE := $(PK64_BUILD)/Makefile
PK64_OUT := $(PK64_BUILD)/pk
PK64_TOOLCHAIN := $(DIR_TOOLCHAIN)/riscv64-unknown-elf/bin/pk
PK64_CONFIGURE := $(TOPDIR)/riscv-pk/configure \
                  --prefix=$(DIR_TOOLCHAIN) \
                  --host=$(PK_HOST)
PK64_ALIAS := riscv-pk64

# alias for both pk32 && pk64
.PHONY: riscv-pk
riscv-pk: $(PK32_TOOLCHAIN) $(PK64_TOOLCHAIN)

# clean for both pk32 && pk64
riscv-pk-clean:
	rm -rf $(PK_BUILD)

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

LLVM_BUILD := $(TOPDIR)/build-llvm
LLVM_MAKEFILE := $(LLVM_BUILD)/Makefile
LLVM_OUT := $(LLVM_BUILD)/bin/clang
LLVM_TOOLCHAIN := $(DIR_TOOLCHAIN)/bin/clang
LLVM_CONFIGURE := \
    $(CMAKE) -DCMAKE_BUILD_TYPE=$(LLVM_BUILD_TYPE) \
             -DLLVM_TARGETS_TO_BUILD="ARM;RISCV;X86" \
             -DCMAKE_INSTALL_PREFIX=$(DIR_TOOLCHAIN) ../llvm
LLVM_ALIAS := llvm
CLANG_LINK := $(TOPDIR)/llvm/tools/clang
LLVM_DEPS := $(NEWLIB_GCC_TOOLCHAIN) $(CMAKE) $(CLANG_LINK)

$(CLANG_LINK):
	ln -sf $(TOPDIR)/clang $@

###
### sbt
###

SBT_BUILD := $(TOPDIR)/sbt/build
SBT_MAKEFILE := $(SBT_BUILD)/Makefile
SBT_OUT := $(SBT_BUILD)/riscv-sbt
SBT_TOOLCHAIN := $(DIR_TOOLCHAIN)/bin/riscv-sbt
SBT_CONFIGURE := \
    $(CMAKE) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
             -DCMAKE_INSTALL_PREFIX=$(DIR_TOOLCHAIN) ..
SBT_ALIAS := sbt
SBT_DEPS := $(LLVM_TOOLCHAIN)

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
