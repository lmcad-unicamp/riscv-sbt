# lowrisc-llvm-debug

LLVM_COMMON_CMAKE_OPTS := \
    -DLLVM_TARGETS_TO_BUILD="ARM;X86"

LOWRISC_LLVM_DEBUG_BUILD := $(BUILD_DIR)/lowrisc-llvm/debug
LOWRISC_LLVM_DEBUG_MAKEFILE := $(LOWRISC_LLVM_DEBUG_BUILD)/build.ninja
LOWRISC_LLVM_DEBUG_OUT := $(LOWRISC_LLVM_DEBUG_BUILD)/bin/clang
LOWRISC_LLVM_DEBUG_INSTALL_DIR := $(TOOLCHAIN_DEBUG)/lowrisc-llvm
LOWRISC_LLVM_DEBUG_TOOLCHAIN := $(LOWRISC_LLVM_DEBUG_INSTALL_DIR)/bin/clang
LOWRISC_LLVM_DEBUG_CONFIGURE := \
    $(CMAKE) \
             -G Ninja \
             $(LLVM_COMMON_CMAKE_OPTS) \
             -DCMAKE_BUILD_TYPE=Debug \
             -DBUILD_SHARED_LIBS=True \
             -DLLVM_USE_SPLIT_DWARF=True \
             -DLLVM_OPTIMIZED_TABLEGEN=True \
             -DLLVM_BUILD_TESTS=True \
             -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="RISCV" \
             -DCMAKE_INSTALL_PREFIX=$(LOWRISC_LLVM_DEBUG_INSTALL_DIR) \
             $(SUBMODULES_DIR)/llvm
LOWRISC_LLVM_DEBUG_ALIAS := lowrisc-llvm-debug
CLANG_LINK := $(SUBMODULES_DIR)/llvm/tools/clang
LOWRISC_LLVM_DEBUG_DEPS := $(RISCV_GNU_TOOLCHAIN_TOOLCHAIN) $(CLANG_LINK)

LOWRISC_LLVM_DEBUG_POSTINSTALL := \
  SRC=$(LOWRISC_LLVM_DEBUG_BUILD)/lib/Target/RISCV && \
  DST=$(LOWRISC_LLVM_DEBUG_INSTALL_DIR)/include/llvm/Target/RISCV && \
  mkdir -p $$DST && \
  (for f in RISCVGenInstrInfo.inc RISCVGenRegisterInfo.inc; do \
    cp $$SRC/$$f $$DST/$$f || exit 1; \
  done)

$(CLANG_LINK):
	ln -sf $(SUBMODULES_DIR)/clang $@


###
### ninja build
###
define RULE_NINJA_BUILD =
# unconditional build
.PHONY: $$($(1)_ALIAS)-build
$$($(1)_ALIAS)-build:
	@echo "*** $$@ ***"
	$(CMAKE) --build $$($(1)_BUILD) -- $(MAKE_OPTS)

#### build only when MAKEFILE changes
$$($(1)_OUT): $$($(1)_MAKEFILE)
	$(MAKE) $$($(1)_ALIAS)-build
	touch $$@

endef

###
### ninja install
###
define RULE_NINJA_INSTALL =
# unconditional install
.PHONY: $$($(1)_ALIAS)-install
$$($(1)_ALIAS)-install:
	@echo "*** $$@ ***"
ifeq ($$($(1)_INSTALL),)
	cd $$($(1)_BUILD) && ninja install
else
	$$($(1)_INSTALL)
endif
ifneq ($$($(1)_POSTINSTALL),)
	$$($(1)_POSTINSTALL)
endif
	$(MAKE) $$($(1)_ALIAS)-update-filelist

### install only when build OUTPUT changes
$$($(1)_TOOLCHAIN): $$($(1)_OUT)
	$(MAKE) $$($(1)_ALIAS)-install
	touch $$@

endef


define RULE_ALL_NINJA =
$(call RULE_MAKEFILE,$(1))
$(call RULE_NINJA_BUILD,$(1))
$(call RULE_UPDATE_FILELIST,$(1))
$(call RULE_NINJA_INSTALL,$(1))
$(call RULE_ALIAS,$(1))
$(call RULE_CLEAN,$(1))
endef

$(eval $(call RULE_ALL_NINJA,LOWRISC_LLVM_DEBUG))
