### rules ###

# DEBUG_RULES := 1

# constants
NOLIBS :=
NOFLAGS :=
ASM := ASM
NOASM :=
C := C
NOC :=

# .c -> .bc
define _C2BC
$(eval C2BC_SRCDIR = $(2))
$(eval C2BC_DSTDIR = $(3))
$(eval C2BC_IN = $(4))
$(eval C2BC_OUT = $(5))
$(eval C2BC_FLAGS = $(6))
$(eval C2BC_CLANG_FLAGS = $($(1)_CLANG_FLAGS))

$(if $(DEBUG_RULES),$(warning "C2BC(SRCDIR=$(C2BC_SRCDIR),\
DSTDIR=$(C2BC_DSTDIR),\
IN=$(C2BC_IN),\
OUT=$(C2BC_OUT),\
FLAGS=$(C2BC_FLAGS))"),)

$(C2BC_DSTDIR)$(C2BC_OUT).bc: $(C2BC_SRCDIR)$(C2BC_IN).c \
							| $(if $(subst ./,,$(C2BC_DSTDIR)),$(C2BC_DSTDIR),)
	@echo $$@: $$<
	$($(1)_CLANG) $(C2BC_CLANG_FLAGS) $(C2BC_FLAGS) \
		$(EMITLLVM) $$< -o $$@

endef


# *.bc -> .bc
define _LLLINK
$(eval LLLINK_DIR = $(2))
$(eval LLLINK_INS = $(3))
$(eval LLLINK_OUT = $(4))

$(if $(DEBUG_RULES),$(warning "LLLINK($(1),\
DIR=$(LLLINK_DIR),\
INS=$(LLLINK_INS),\
OUT=$(LLLINK_OUT))"),)

$(LLLINK_DIR)$(LLLINK_OUT).bc: $(foreach bc,$(LLLINK_INS),$(LLLINK_DIR)$(bc).bc)
	@echo $$@: $$^
	$(LLVMLINK) $$^ -o $$@

endef


# .bc -> .ll
define _DIS
$(eval DIS_DIR = $(2))
$(eval DIS_MOD = $(3))

$(if $(DEBUG_RULES),$(warning "DIS($(1),\
DIR=$(DIS_DIR),\
MOD=$(DIS_MOD))"),)

$(DIS_DIR)$(DIS_MOD).ll: $(DIS_DIR)$(DIS_MOD).bc
	@echo $$@: $$^
	$(LLVMDIS) $$^ -o $$@

endef


# opt
define _OPT
$(eval OPT_DIR = $(2))
$(eval OPT_IN = $(3))
$(eval OPT_OUT = $(4))

$(if $(DEBUG_RULES),$(warning "OPT($(1),\
DIR=$(OPT_DIR),\
IN=$(OPT_IN),\
OUT=$(OPT_OUT))"),)

$(OPT_DIR)$(OPT_OUT).bc: $(OPT_DIR)$(OPT_IN).bc $(OPT_DIR)$(OPT_IN).ll
	@echo $$@: $$<
	$(LLVMOPT) $(LLVMOPT_FLAGS) $$< -o $$@

endef


# .c -> .bc && .bc -> .ll
define _C2BCNDIS
$(call _C2BC,$(1),$(2),$(3),$(4),$(5),$(6))
$(call _DIS,$(1),$(3),$(5))
endef


# path -> dir/prefix-file
# 1: prefix
# 2: path
define _ADD_PREFIX
$(dir $(2))$(1)-$(notdir $(2))
endef


# *.c -> *.bc -> .bc
# (C2LBC: C to linked BC)
define _C2LBC
$(eval C2LBC_PREFIX = $($(1)_PREFIX))
$(eval C2LBC_SRCDIR = $(2))
$(eval C2LBC_DSTDIR = $(3))
$(eval C2LBC_INS = $(4))
$(eval C2LBC_OUT = $(5))
$(eval C2LBC_LIBS = $(6))
$(eval C2LBC_FLAGS = $(7))

$(if $(DEBUG_RULES),$(warning "C2LBC($(1),\
SRCDIR=$(C2LBC_SRCDIR),\
DSTDIR=$(C2LBC_DSTDIR),\
INS=$(C2LBC_INS),\
OUT=$(C2LBC_OUT),\
LIBS=$(C2LBC_LIBS),\
FLAGS=$(C2LBC_FLAGS))"),)

$(if $(subst 1,,$(words $(C2LBC_INS))),\
$(foreach c,$(C2LBC_INS),\
$(call _C2BCNDIS,$(1),$(C2LBC_SRCDIR),$(C2LBC_DSTDIR),$(c),\
$(call _ADD_PREFIX,$(C2LBC_PREFIX),$(c)_bc1),$(C2LBC_FLAGS))),\
$(call _C2BC,$(1),$(C2LBC_SRCDIR),$(C2LBC_DSTDIR),\
$(C2LBC_INS),$(C2LBC_OUT),$(C2LBC_FLAGS)))

$(if $(subst 1,,$(words $(C2LBC_INS))),\
$(call _LLLINK,$(1),$(C2LBC_DSTDIR),\
$(foreach bc,$(C2LBC_INS),\
$(call _ADD_PREFIX,$(C2LBC_PREFIX),$(bc)_bc1)),\
$(C2LBC_OUT)))

$(call _DIS,$(1),$(C2LBC_DSTDIR),$(C2LBC_OUT))
endef


# .bc -> .s
define _BC2S
$(eval BC2S_DIR = $(2))
$(eval BC2S_IN = $(3))
$(eval BC2S_OUT = $(4))

$(if $(DEBUG_RULES),$(warning "BC2S($(1),\
DIR=$(BC2S_DIR),\
IN=$(BC2S_IN),\
OUT=$(BC2S_OUT))"),)

$(BC2S_DIR)$(BC2S_OUT).s: $(BC2S_DIR)$(BC2S_IN).bc $(BC2S_DIR)$(BC2S_IN).ll
	@echo $$@: $$<
	$(LLC) $(LLC_FLAGS) $($(1)_LLC_FLAGS) $$< -o $$@
ifeq ($(1),RV32)
	sed -i "1i.option norelax" $$@
endif

endef


# opt; dis; opt; dis; .bc -> .s
define _OPTNBC2S
$(eval OPTNBC2S_DSTDIR = $(3))
$(eval OPTNBC2S_OUT = $(5))

$(if $(DEBUG_RULES),$(warning "OPTNBC2S(DSTDIR=$(OPTNBC2S_DSTDIR),\
OUT=$(OPTNBC2S_OUT))"),)

$(call _OPT,$(1),$(OPTNBC2S_DSTDIR),$(OPTNBC2S_OUT),$(OPTNBC2S_OUT).opt)
$(call _DIS,$(1),$(OPTNBC2S_DSTDIR),$(OPTNBC2S_OUT).opt)
$(call _OPT,$(1),$(OPTNBC2S_DSTDIR),$(OPTNBC2S_OUT).opt,$(OPTNBC2S_OUT).opt2)
$(call _DIS,$(1),$(OPTNBC2S_DSTDIR),$(OPTNBC2S_OUT).opt2)
$(call _BC2S,$(1),$(OPTNBC2S_DSTDIR),$(OPTNBC2S_OUT).opt2,$(OPTNBC2S_OUT))
endef


# *.c -> .s
define _C2S
$(eval C2S_SRCDIR = $(2))
$(eval C2S_DSTDIR = $(3))
$(eval C2S_INS = $(4))
$(eval C2S_OUT = $(5))
$(eval C2S_LIBS = $(6))
$(eval C2S_FLAGS = $(7))

$(if $(DEBUG_RULES),$(warning "C2S(SRCDIR=$(C2S_SRCDIR),\
DSTDIR=$(C2S_DSTDIR),\
INS=$(C2S_INS),\
OUT=$(C2S_OUT),\
LIBS=$(C2S_LIBS),\
FLAGS=$(C2S_FLAGS))"),)

$(call _C2LBC,$(1),$(C2S_SRCDIR),$(C2S_DSTDIR),\
$(C2S_INS),$(C2S_OUT),$(C2S_LIBS),$(C2S_FLAGS))
$(call _OPTNBC2S,$(1),$(C2S_SRCDIR),$(C2S_DSTDIR),\
$(C2S_INS),$(C2S_OUT),$(C2S_LIBS))
endef


# .s -> .o
define _S2O
$(eval S2O_SRCDIR = $(2))
$(eval S2O_DSTDIR = $(3))
$(eval S2O_IN = $(4))
$(eval S2O_OUT = $(5))

$(if $(DEBUG_RULES),$(warning "S2O($(1),\
SRCDIR=$(S2O_SRCDIR),\
DSTDIR=$(S2O_DSTDIR),\
IN=$(S2O_IN),\
OUT=$(S2O_OUT))"),)

$(S2O_DSTDIR)$(S2O_OUT).o: $(S2O_SRCDIR)$(S2O_IN).s \
							| $(if $(subst ./,,$(S2O_DSTDIR)),$(S2O_DSTDIR),)
	@echo $$@: $$<
	$$($(1)_AS) -o $$@ -c $$<

endef


# *.s -> *.o
define _S2OS
$(eval S2OS_SRCDIR = $(2))
$(eval S2OS_DSTDIR = $(3))
$(eval S2OS_INS = $(4))
$(eval S2OS_OUT = $(5))

$(if $(DEBUG_RULES),$(warning "S2OS($(1),\
SRCDIR=$(S2OS_SRCDIR),\
DSTDIR=$(S2OS_DSTDIR),\
INS=$(S2OS_INS),\
OUT=$(S2OS_OUT))"),)

$(if $(subst 1,,$(words $(S2OS_INS))),\
$(foreach in,$(S2OS_INS),\
$(call _S2O,$(1),$(S2OS_SRCDIR),$(S2OS_DSTDIR),$(in))),\
$(call _S2O,$(1),$(S2OS_SRCDIR),$(S2OS_DSTDIR),$(S2OS_INS),$(S2OS_OUT)))
endef


# *.o -> bin
define _LINK
$(eval LINK_SRCDIR = $(2))
$(eval LINK_DSTDIR = $(3))
$(eval LINK_INS = $(4))
$(eval LINK_OUT = $(5))
$(eval LINK_LIBS = $(6))
$(eval LINK_C = $(7))
$(eval LINK_OBJS = $(addsuffix .o,$(LINK_INS)))

$(if $(DEBUG_RULES),$(warning "LINK($(1),\
SRCDIR=$(LINK_SRCDIR),\
DSTDIR=$(LINK_DSTDIR),\
INS=$(LINK_INS),\
OUT=$(LINK_OUT),\
LIBS=$(LINK_LIBS),\
C=$(LINK_C))"),)

$(LINK_DSTDIR)$(LINK_OUT): $(addprefix $(LINK_SRCDIR),$(LINK_OBJS))
	@echo $$@: $$^
	$(if $(LINK_C),$($(1)_GCC) $(GCC_CFLAGS),$($(1)_LD)) -o $$@ $$^ $(LINK_LIBS) \
		$(if $(LINK_C),-lm,)

endef


# *.s -> bin
define _SNLINK
$(eval SNLINK_SRCDIR = $(2))
$(eval SNLINK_DSTDIR = $(3))
$(eval SNLINK_INS = $(4))
$(eval SNLINK_OUT = $(5))
$(eval SNLINK_LIBS = $(6))
$(eval SNLINK_FLAGS = $(7))
$(eval SNLINK_C = $(8))

$(if $(DEBUG_RULES),$(warning "SNLINK($(1),\
SRCDIR=$(SNLINK_SRCDIR),\
DSTDIR=$(SNLINK_DSTDIR),\
INS=$(SNLINK_INS),\
OUT=$(SNLINK_OUT),\
LIBS=$(SNLINK_LIBS),\
FLAGS=$(SNLINK_FLAGS),\
C=$(SNLINK_C))"),)

$(call _S2OS,$(1),$(SNLINK_SRCDIR),$(SNLINK_DSTDIR),$(SNLINK_INS),$(SNLINK_OUT))

$(call _LINK,$(1),$(SNLINK_DSTDIR),$(SNLINK_DSTDIR),\
$(if $(subst 1,,$(words $(SNLINK_INS))),$(SNLINK_INS),$(SNLINK_OUT)),\
$(SNLINK_OUT),$(SNLINK_LIBS),$(SNLINK_C))
endef


# *.c -> bin
define _CNLINK
$(eval CNLINK_SRCDIR = $(2))
$(eval CNLINK_DSTDIR = $(3))
$(eval CNLINK_INS = $(4))
$(eval CNLINK_OUT = $(5))
$(eval CNLINK_LIBS = $(6))
$(eval CNLINK_FLAGS = $(7))
$(eval CNLINK_C = $(8))

$(if $(DEBUG_RULES),$(warning "CNLINK($(1),\
SRCDIR=$(CNLINK_SRCDIR),\
DSTDIR=$(CNLINK_DSTDIR),\
INS=$(CNLINK_INS),\
OUT=$(CNLINK_OUT),\
LIBS=$(CNLINK_LIBS),\
FLAGS=$(CNLINK_FLAGS),\
C=$(CNLINK_C))"),)

$(call _C2S,$(1),$(CNLINK_SRCDIR),$(CNLINK_DSTDIR),$(CNLINK_INS),\
$(CNLINK_OUT),$(CNLINK_LIBS),$(CNLINK_FLAGS))

$(call _S2O,$(1),$(CNLINK_DSTDIR),$(CNLINK_DSTDIR),\
$(CNLINK_OUT),$(CNLINK_OUT))

$(call _LINK,$(1),$(CNLINK_DSTDIR),$(CNLINK_DSTDIR),\
$(CNLINK_OUT),$(CNLINK_OUT),$(CNLINK_LIBS),$(CNLINK_C))
endef


# clean
define _CLEAN
$(eval CLEAN_DIR = $(1))
$(eval CLEAN_MOD = $(2))
$(eval CLEAN_S = $(3))

$(if $(DEBUG_RULES),$(warning "CLEAN(DIR=$(CLEAN_DIR),\
MOD=$(CLEAN_MOD),\
CLEAN_S=$(CLEAN_S))"),)

clean-$(CLEAN_MOD):
	@echo $$@:
	cd $(CLEAN_DIR) && \
		rm -f $(CLEAN_MOD) \
		$(CLEAN_MOD).o \
		$(if $(CLEAN_S),$(CLEAN_MOD).s,) \
		$(CLEAN_MOD).bc $(CLEAN_MOD).ll \
		$(CLEAN_MOD).opt.bc $(CLEAN_MOD).opt.ll \
		$(CLEAN_MOD).opt2.bc $(CLEAN_MOD).opt2.ll \
		$(CLEAN_MOD).out

endef


# run
define _RUN
$(eval RUN_DIR = $(2))
$(eval RUN_PROG = $(3))
$(eval RUN_SAVE = $(4))
$(eval RUN_ARGS = $(5))

$(if $(DEBUG_RULES),$(warning "RUN(DIR=$(RUN_DIR),\
PROG=$(RUN_PROG),\
SAVE=$(RUN_SAVE),\
ARGS=$(RUN_ARGS))"),)

run-$(RUN_PROG): $(RUN_DIR)$(RUN_PROG)
	@echo $$@:
	$(if $(RUN_SAVE),$(RUN_SH) -o $(RUN_DIR)$(RUN_PROG).out,) \
		$($(1)_RUN) $(RUN_DIR)$(RUN_PROG) $(RUN_ARGS)

endef


# alias
define _ALIAS
$(eval ALIAS_DIR = $(1))
$(eval ALIAS_BIN = $(2))

$(if $(DEBUG_RULES),$(warning "ALIAS(DIR=$(ALIAS_DIR),\
BIN=$(ALIAS_BIN))"),)

ifneq ($(ALIAS_DIR),./)
.PHONY: $(ALIAS_BIN)
$(ALIAS_BIN): $(ALIAS_DIR) $(ALIAS_DIR)$(ALIAS_BIN)
endif

endef


# *.c/s -> bin
define BUILD
$(eval BUILD_SRCDIR = $(2))
$(eval BUILD_DSTDIR = $(3))
$(eval BUILD_INS = $(4))
$(eval BUILD_OUT = $(5))
$(eval BUILD_LIBS = $(6))
$(eval BUILD_FLAGS = $(7))
$(eval BUILD_ASM = $(8))
$(eval BUILD_C = $(9))
$(eval BUILD_RUNARGS = $(10))

$(if $(DEBUG_RULES),$(warning "BUILD($(1),\
SRCDIR=$(BUILD_SRCDIR),\
DSTDIR=$(BUILD_DSTDIR),\
INS=$(BUILD_INS),\
OUT=$(BUILD_OUT),\
LIBS=$(BUILD_LIBS),\
FLAGS=$(BUILD_FLAGS),\
ASM=$(BUILD_ASM),\
C=$(BUILD_C),\
RUNARGS=$(BUILD_RUNARGS))"),)

# BUILDO: (optimize only one time)

# BUILDO4SBT:
# build object file for translation
# (don't use sysroot, use native header/libraries instead)

$(call $(if $(BUILD_ASM),_SNLINK,_CNLINK)\
,$(1),$(BUILD_SRCDIR),$(BUILD_DSTDIR),$(BUILD_INS),\
$(BUILD_OUT),$(BUILD_LIBS),$(BUILD_FLAGS),$(BUILD_C))

$(call _CLEAN,$(BUILD_DSTDIR),$(BUILD_OUT),$(if $(BUILD_C),,clean.s))
$(call _RUN,$(1),$(BUILD_DSTDIR),$(BUILD_OUT),save.out,$(BUILD_RUNARGS))
$(call _ALIAS,$(BUILD_DSTDIR),$(BUILD_OUT))
endef


# .o -> .bc
define _TRANSLATE_OBJ
$(eval TO_DIR = $(2))
$(eval TO_IN = $(3))
$(eval TO_OUT = $(4))
$(eval TO_FLAGS = $(5))

$(if $(DEBUG_RULES),$(warning "TRANSLATE_OBJ($(1),\
DIR=$(TO_DIR),\
IN=$(TO_IN),\
OUT=$(TO_OUT),\
FLAGS=$(TO_FLAGS))"),)

$(TO_DIR)$(TO_OUT).bc: $(TO_DIR)$(TO_IN).o
	@echo $$@: $$<
	riscv-sbt $(SBTFLAGS) $(TO_FLAGS) -o $$@ $$^ &> $(TOPDIR)/sbt.log

endef


# .o -> bin
define TRANSLATE
$(eval TRANSLATE_SRCDIR = $(2))
$(eval TRANSLATE_DSTDIR = $(3))
$(eval TRANSLATE_IN = $(4))
$(eval TRANSLATE_OUT = $(5))
$(eval TRANSLATE_LIBS = $(6))
$(eval TRANSLATE_FLAGS = $(7))
$(eval TRANSLATE_C = $(8))
$(eval TRANSLATE_RUNARGS = $(9))

$(eval TRANSLATE_NAT_OBJS = $(foreach obj,$(SBT_NAT_OBJS),$($(1)_$(obj)_O)))

$(if $(DEBUG_RULES),$(warning "TRANSLATE($(1),\
SRCDIR=$(TRANSLATE_SRCDIR),\
DSTDIR=$(TRANSLATE_DSTDIR),\
IN=$(TRANSLATE_IN),\
OUT=$(TRANSLATE_OUT),\
LIBS=$(TRANSLATE_LIBS),\
FLAGS=$(TRANSLATE_FLAGS),\
C=$(TRANSLATE_C),\
RUNARGS=$(TRANSLATE_RUNARGS))"),)

$(call _TRANSLATE_OBJ,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_IN),\
$(TRANSLATE_OUT),$(TRANSLATE_FLAGS) $(if $(TRANSLATE_C),,-dont-use-libc))

$(call _DIS,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_OUT))
$(call _OPT,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_OUT),$(TRANSLATE_OUT).opt)
$(call _DIS,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_OUT).opt)
$(call _BC2S,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_OUT).opt,$(TRANSLATE_OUT))

$(call BUILD,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_DSTDIR),\
$(TRANSLATE_OUT),$(TRANSLATE_OUT),\
$(TRANSLATE_LIBS) $(TRANSLATE_NAT_OBJS),$(NOFLAGS),\
$(ASM),$(TRANSLATE_C),$(TRANSLATE_RUNARGS))
endef
