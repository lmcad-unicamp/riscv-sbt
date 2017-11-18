### rules ###

# DEBUG_RULES := 1

# .c -> .bc
define C2BC
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
							$(if $(subst ./,,$(C2BC_DSTDIR)),$(C2BC_DSTDIR),)
	@echo $$@: $$<
	$($(1)_CLANG) $(C2BC_CLANG_FLAGS) $(BUILD_CFLAGS) $(C2BC_FLAGS) \
		$(EMITLLVM) $$< -o $$@

endef


# .c -> .o
define C2O
$(eval C2O_SRCDIR = $(2))
$(eval C2O_DSTDIR = $(3))
$(eval C2O_IN = $(4))
$(eval C2O_OUT = $(5))

$(if $(DEBUG_RULES),$(warning "C2O(SRCDIR=$(C2O_SRCDIR),\
DSTDIR=$(C2O_DSTDIR),\
IN=$(C2O_IN),\
OUT=$(C2O_OUT))"),)

$(C2O_DSTDIR)$(C2O_OUT).o: $(GCC_C2O_SRCDIR)$(GCC_C2O_IN).c
	@echo $$@: $$<
	$($(1)_GCC) $(GCC_CFLAGS) $(BUILD_CFLAGS) -c $$< -o $$@

endef


# .c -> .o
define GCC_C2O
$(eval C2O_SRCDIR = $(2))
$(eval C2O_DSTDIR = $(3))
$(eval C2O_IN = $(4))
$(eval C2O_OUT = $(5))

$(C2O_DSTDIR)$(C2O_OUT).o: $(C2O_SRCDIR)$(C2O_IN).c
	@echo $$@: $$<
	$($(1)_GCC) $(GCC_CFLAGS) $(BUILD_CFLAGS) -c $$< -o $$@

endef


# *.bc -> .bc
define LLLINK
$(eval LLLINK_DIR = $(2))
$(eval LLLINK_INS = $(3))
$(eval LLLINK_OUT = $(4))

$(LLLINK_DIR)$(LLLINK_OUT).bc: $(foreach bc,$(LLLINK_INS),$(LLLINK_SRCDIR)$(bc).bc)
	@echo $$@: $$^
	$(LLVMLINK) $$^ -o $$@

endef


# .bc -> .ll
define DIS
$(eval DIS_DIR = $(2))
$(eval DIS_MOD = $(3))

$(DIS_DIR)$(DIS_MOD).ll: $(DIS_DIR)$(DIS_MOD).bc
	@echo $$@: $$^
	$(LLVMDIS) $$^ -o $$@

endef


# opt
define OPT
$(eval OPT_DIR = $(2))
$(eval OPT_IN = $(3))
$(eval OPT_OUT = $(4))

$(OPT_DIR)$(OPT_OUT).bc: $(OPT_DIR)$(OPT_IN).bc $(OPT_DIR)$(OPT_IN).ll
	@echo $$@: $$<
	$(LLVMOPT) $(LLVMOPT_FLAGS) $$< -o $$@

endef


# .bc -> .s
define BC2S
$(eval BC2S_DIR = $(2))
$(eval BC2S_IN = $(3))
$(eval BC2S_OUT = $(4))

$(BC2S_DIR)$(BC2S_OUT).s: $(BC2S_DIR)$(BC2S_IN).bc $(BC2S_DIR)$(BC2S_IN).ll
	@echo $$@: $$<
	$(LLC) $(LLC_FLAGS) $($(1)_LLC_FLAGS) $$< -o $$@
ifeq ($(1),RV32)
	sed -i "1i.option norelax" $$@
endif

endef


# .s -> .o
define S2O
$(eval S2O_SRCDIR = $(2))
$(eval S2O_DSTDIR = $(3))
$(eval S2O_MOD = $(4))

$(if $(DEBUG_RULES),$(warning "S2O(SRCDIR=$(S2O_SRCDIR),\
DSTDIR=$(S2O_DSTDIR),\
MOD=$(S2O_MOD))"),)

$(S2O_DSTDIR)$(S2O_MOD).o: $(S2O_SRCDIR)$(S2O_MOD).s
	@echo $$@: $$<
	$$($(1)_AS) -o $$@ -c $$<

endef


# *.o -> bin
define LINK
$(eval LINK_DIR = $(2))
$(eval LINK_OBJS = $(3))
$(eval LINK_OUT = $(4))
$(eval LINK_LIBS = $(5))

$(LINK_DIR)$(LINK_OUT): $(addprefix $(LINK_DIR),$(addsuffix .o,$(LINK_OBJS)))
	@echo $$@: $$^
	$($(1)_LD) -o $$@ $$^ $(LINK_LIBS)

endef


# *.o -> cbin
define CLINK
$(eval CL_SRCDIR = $(2))
$(eval CL_DSTDIR = $(3))
$(eval CL_MODS = $(4))
$(eval CL_OUT = $(5))
$(eval CL_LIBS = $(6))
$(eval CL_OBJS = $(addsuffix .o,$(CL_MODS)))

$(CL_DSTDIR)$(CL_OUT): $(addprefix $(CL_SRCDIR),$(CL_OBJS))
	@echo $$@: $$^
	$($(1)_GCC) -o $$@ $$^ $(CL_LIBS)

endef


# *.o -> cbin
define GCC_LINK
$(eval CL_SRCDIR = $(2))
$(eval CL_DSTDIR = $(3))
$(eval CL_MODS = $(4))
$(eval CL_OUT = $(5))
$(eval CL_LIBS = $(6))
$(eval CL_OBJS = $(addsuffix .o,$(CL_MODS)))

$(CL_DSTDIR)$(CL_OUT): $(addprefix $(CL_SRCDIR),$(CL_OBJS))
	@echo $$@: $$^
	$($(1)_GCC) $(GCC_CFLAGS) -o $$@ $$^ $(CL_LIBS) -lm

endef


# *.s -> cbin
define CLINKS
$(eval CL_SRCDIR = $(2))
$(eval CL_DSTDIR = $(3))
$(eval CL_INS = $(4))
$(eval CL_OUT = $(5))
$(eval CL_LIBS = $(6))
$(eval CL_SRCS = $(addsuffix .s,$(CL_INS)))

$(if $(DEBUG_RULES),$(warning "CLINKS(SRCDIR=$(CL_SRCDIR),\
DSTDIR=$(CL_DSTDIR),\
INS=$(CL_INS),\
OUT=$(CL_OUT),\
LIBS=$(CL_LIBS))"),)

$(CL_DSTDIR)$(CL_OUT): $(addprefix $(CL_SRCDIR),$(CL_SRCS))
	@echo $$@: $$^
	$($(1)_GCC) $(GCC_CFLAGS) -o $$@ $$^ $(CL_LIBS) -lm

endef


# clean
define CLEAN
$(eval CLEAN_DIR = $(1))
$(eval CLEAN_MOD = $(2))
$(eval CLEAN_S = $(3))

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
define RUN
$(eval RUN_DIR = $(2))
$(eval RUN_PROG = $(3))
$(eval RUN_SAVE = $(4))

run-$(RUN_PROG): $(RUN_DIR)$(RUN_PROG)
	@echo $$@:
  ifeq ($(RUN_SAVE),)
	$($(1)_RUN) $(RUN_DIR)$(RUN_PROG)
  else
	$($(1)_RUN) $(RUN_DIR)$(RUN_PROG) | tee $(RUN_DIR)$(RUN_PROG).out
  endif

endef


# alias
define ALIAS
$(eval ALIAS_DIR = $(1))
$(eval ALIAS_BIN = $(2))

ifneq ($(ALIAS_DIR),./)
.PHONY: $(ALIAS_BIN)
$(ALIAS_BIN): $(ALIAS_DIR) $(ALIAS_DIR)$(ALIAS_BIN)
endif

endef


# .c -> .bc && .bc -> .ll
define _C2BCNDIS
$(call C2BC,$(1),$(2),$(3),$(4),$(5),$(6))
$(call DIS,$(1),$(3),$(5))
endef


# path -> dir/prefix-file
# 1: prefix
# 2: path
define _ADD_PREFIX
$(dir $(2))$(1)-$(notdir $(2))
endef


# *.c -> *.bc -> .bc
define _BUILD2
$(eval BUILD2_PREFIX = $($(1)_PREFIX))
$(eval BUILD2_SRCDIR = $(2))
$(eval BUILD2_DSTDIR = $(3))
$(eval BUILD2_INS = $(4))
$(eval BUILD2_OUT = $(5))
$(eval BUILD2_LIBS = $(6))
$(eval BUILD2_FLAGS = $(7))

$(if $(DEBUG_RULES),$(warning "_BUILD2(SRCDIR=$(BUILD2_SRCDIR),\
DSTDIR=$(BUILD2_DSTDIR),\
INS=$(BUILD2_INS),\
OUT=$(BUILD2_OUT),\
LIBS=$(BUILD2_LIBS),\
FLAGS=$(BUILD2_FLAGS))"),)

ifeq ($(words $(BUILD2_INS)),1)
$(call C2BC,$(1),$(BUILD2_SRCDIR),$(BUILD2_DSTDIR),\
$(BUILD2_INS),$(BUILD2_OUT),$(BUILD2_FLAGS))
else
$(foreach c,$(BUILD2_INS),\
$(call _C2BCNDIS,$(1),$(BUILD2_SRCDIR),$(BUILD2_DSTDIR),$(c),\
$(call _ADD_PREFIX,$(BUILD2_PREFIX),$(c)_bc1),$(BUILD2_FLAGS)))

$(call LLLINK,$(1),$(BUILD2_DSTDIR),$(foreach bc,$(BUILD2_INS),\
$(call _ADD_PREFIX,$(BUILD2_PREFIX),$(bc)_bc1)),$(BUILD2_OUT))
endif

$(call DIS,$(1),$(BUILD2_DSTDIR),$(BUILD2_OUT))
endef


# opt; dis; opt; dis; .bc -> .s
define _BUILD3
$(eval BUILD3_DSTDIR = $(3))
$(eval BUILD3_OUT = $(5))

$(if $(DEBUG_RULES),$(warning "_BUILD3(DSTDIR=$(BUILD3_DSTDIR),\
OUT=$(BUILD3_OUT))"),)

$(call OPT,$(1),$(BUILD3_DSTDIR),$(BUILD3_OUT),$(BUILD3_OUT).opt)
$(call DIS,$(1),$(BUILD3_DSTDIR),$(BUILD3_OUT).opt)
$(call OPT,$(1),$(BUILD3_DSTDIR),$(BUILD3_OUT).opt,$(BUILD3_OUT).opt2)
$(call DIS,$(1),$(BUILD3_DSTDIR),$(BUILD3_OUT).opt2)
$(call BC2S,$(1),$(BUILD3_DSTDIR),$(BUILD3_OUT).opt2,$(BUILD3_OUT))
endef


# *.c -> .s
define _BUILD1
$(eval BUILD1_SRCDIR = $(2))
$(eval BUILD1_DSTDIR = $(3))
$(eval BUILD1_INS = $(4))
$(eval BUILD1_OUT = $(5))
$(eval BUILD1_LIBS = $(6))
$(eval BUILD1_FLAGS = $(7))

$(if $(DEBUG_RULES),$(warning "_BUILD1(SRCDIR=$(BUILD1_SRCDIR),\
DSTDIR=$(BUILD1_DSTDIR),\
INS=$(BUILD1_INS),\
OUT=$(BUILD1_OUT),\
LIBS=$(BUILD1_LIBS),\
FLAGS=$(BUILD1_FLAGS))"),)

$(call _BUILD2,$(1),$(BUILD1_SRCDIR),$(BUILD1_DSTDIR),\
$(BUILD1_INS),$(BUILD1_OUT),$(BUILD1_LIBS),$(BUILD1_FLAGS))
$(call _BUILD3,$(1),$(BUILD1_SRCDIR),$(BUILD1_DSTDIR),$(BUILD1_INS),$(BUILD1_OUT),$(BUILD1_LIBS))
endef


# .s -> bin
define BUILDS
$(eval BUILDS_SRCDIR = $(2))
$(eval BUILDS_DSTDIR = $(3))
$(eval BUILDS_INS = $(4))
$(eval BUILDS_OUT = $(5))
$(eval BUILDS_LIBS = $(6))

$(call S2O,$(1),$(BUILDS_SRCDIR),$(BUILDS_DSTDIR),$(BUILDS_OUT))
$(call LINK,$(1),$(BUILDS_DSTDIR),$(BUILDS_OUT),$(BUILDS_OUT),$(BUILDS_LIBS))
$(call RUN,$(1),$(BUILDS_DSTDIR),$(BUILDS_OUT),save-run.out)
$(call ALIAS,$(BUILDS_DSTDIR),$(BUILDS_OUT))
endef


# .s -> cbin
define CBUILDS
$(eval CBUILDS_SRCDIR = $(2))
$(eval CBUILDS_DSTDIR = $(3))
$(eval CBUILDS_INS = $(4))
$(eval CBUILDS_OUT = $(5))
$(eval CBUILDS_LIBS = $(6))

$(if $(DEBUG_RULES),$(warning "CBUILDS(SRCDIR=$(CBUILDS_SRCDIR),\
DSTDIR=$(CBUILDS_DSTDIR),\
INS=$(CBUILDS_INS),\
OUT=$(CBUILDS_OUT),\
LIBS=$(CBUILDS_LIBS))"),)

$(call CLINKS,$(1),$(CBUILDS_SRCDIR),$(CBUILDS_DSTDIR),$(CBUILDS_OUT),$(CBUILDS_OUT),$(CBUILDS_LIBS))
$(call RUN,$(1),$(CBUILDS_DSTDIR),$(CBUILDS_OUT),save-run.out)
$(call ALIAS,$(CBUILDS_DSTDIR),$(CBUILDS_OUT))
endef


# *.c -> cbin
define BUILD
$(eval BUILD_SRCDIR = $(2))
$(eval BUILD_DSTDIR = $(3))
$(eval BUILD_INS = $(4))
$(eval BUILD_OUT = $(5))
$(eval BUILD_LIBS = $(6))
$(eval BUILD_FLAGS = $(7))

$(if $(DEBUG_RULES),$(warning "BUILD(SRCDIR=$(BUILD_SRCDIR),\
DSTDIR=$(BUILD_DSTDIR),\
INS=$(BUILD_INS),\
OUT=$(BUILD_OUT),\
LIBS=$(BUILD_LIBS),\
FLAGS=$(BUILD_FLAGS))"),)

$(call _BUILD1,$(1),$(BUILD_SRCDIR),$(BUILD_DSTDIR),$(BUILD_INS),$(BUILD_OUT),\
$(BUILD_LIBS),$(BUILD_FLAGS))
$(call CBUILDS,$(1),$(BUILD_DSTDIR),$(BUILD_DSTDIR),$(BUILD_OUT),$(BUILD_OUT),$(BUILD_LIBS))
endef


# *.c -> cbin
define GCC_BUILD
$(eval BUILD_PREFIX = $($(1)_PREFIX))
$(eval BUILD_SRCDIR = $(2))
$(eval BUILD_DSTDIR = $(3))
$(eval BUILD_INS = $(4))
$(eval BUILD_OUT = $(5))
$(eval BUILD_LIBS = $(6))

$(eval $(foreach c,$(BUILD_INS),\
$(call GCC_C2O,$(1),$(BUILD_SRCDIR),$(BUILD_DSTDIR),$(c),\
$(call _ADD_PREFIX,$(BUILD_PREFIX),$(c)))))

$(call GCC_LINK,$(1),$(BUILD_DSTDIR),$(BUILD_DSTDIR),\
$(foreach o,$(BUILD_INS),$(call _ADD_PREFIX,$(BUILD_PREFIX),$(o))),$(BUILD_OUT),)

$(call RUN,$(1),$(BUILD_DSTDIR),$(BUILD_OUT),save-run.out)
$(call ALIAS,$(BUILD_DSTDIR),$(BUILD_OUT))
endef


# *.c -> .o
# (optimize only one time)
define BUILDO
$(eval BUILD_PREFIX = $($(1)_PREFIX))
$(eval BUILD_SRCDIR = $(2))
$(eval BUILD_DSTDIR = $(3))
$(eval BUILD_INS = $(4))
$(eval BUILD_OUT = $(5))
$(eval BUILD_FLAGS = $(6))

$(call C2BC,$(1),$(BUILD_SRCDIR),$(BUILD_DSTDIR),\
$(BUILD_INS),$(BUILD_OUT),$(BUILD_FLAGS))
$(call DIS,$(1),$(BUILD_DSTDIR),$(BUILD_OUT))
$(call OPT,$(1),$(BUILD_DSTDIR),$(BUILD_OUT),$(BUILD_OUT).opt)
$(call DIS,$(1),$(BUILD_DSTDIR),$(BUILD_OUT).opt)
$(call BC2S,$(1),$(BUILD_DSTDIR),$(BUILD_OUT).opt,$(BUILD_OUT))
$(call S2O,$(1),$(BUILD_DSTDIR),$(BUILD_DSTDIR),$(BUILD_OUT))
endef


# *.c -> .o
# build object file for translation
# (don't use sysroot, use native header/libraries instead)
define BUILDO4SBT
$(eval BUILDO4SBT_$(1)_CLANG_FLAGS = $($(1)_CLANG_FLAGS))
$(eval $(1)_CLANG_FLAGS = )
$(call BUILDO,$(1),$(2),$(3),$(4),$(5))
$(eval $(1)_CLANG_FLAGS = $(BUILDO4SBT_$(1)_CLANG_FLAGS))
endef


# .o -> .bc
define TRANSLATE_OBJ
$(eval TO_DIR = $(2))
$(eval TO_IN = $(3))
$(eval TO_OUT = $(4))

$(TO_DIR)$(TO_OUT).bc: $(TO_DIR)$(TO_IN).o
	riscv-sbt $(SBTFLAGS) -o $$@ $$^ &> $(TOPDIR)/sbt.log

endef


# .o -> .s
define _TRANSLATE1
$(eval TRANSLATE1_SRCDIR = $(2))
$(eval TRANSLATE1_DSTDIR = $(3))
$(eval TRANSLATE1_IN = $(4))
$(eval TRANSLATE1_OUT = $(5))
$(eval TRANSLATE1_LIBS = $(6))

$(call TRANSLATE_OBJ,$(1),$(TRANSLATE1_DSTDIR),$(TRANSLATE1_IN),$(TRANSLATE1_OUT))
$(call DIS,$(1),$(TRANSLATE1_DSTDIR),$(TRANSLATE1_OUT))
$(call OPT,$(1),$(TRANSLATE1_DSTDIR),$(TRANSLATE1_OUT),$(TRANSLATE1_OUT).opt)
$(call DIS,$(1),$(TRANSLATE1_DSTDIR),$(TRANSLATE1_OUT).opt)
$(call BC2S,$(1),$(TRANSLATE1_DSTDIR),$(TRANSLATE1_OUT).opt,$(TRANSLATE1_OUT))
endef


# .o -> cbin
define TRANSLATE
$(eval TRANSLATE_SRCDIR = $(2))
$(eval TRANSLATE_DSTDIR = $(3))
$(eval TRANSLATE_IN = $(4))
$(eval TRANSLATE_OUT = $(5))
$(eval TRANSLATE_LIBS = $(6))

$(call _TRANSLATE1,$(1),$(TRANSLATE_SRCDIR),$(TRANSLATE_DSTDIR),\
$(TRANSLATE_IN),$(TRANSLATE_OUT),$(TRANSLATE_LIBS))

$(call CBUILDS,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_DSTDIR),$(TRANSLATE_OUT),\
$(TRANSLATE_OUT),$(TRANSLATE_LIBS) $(X86_SYSCALL_O))
endef


# .o -> bin
define TRANSLATE_ASM
$(eval TRANSLATE_SRCDIR = $(2))
$(eval TRANSLATE_DSTDIR = $(3))
$(eval TRANSLATE_IN = $(4))
$(eval TRANSLATE_OUT = $(5))
$(eval TRANSLATE_LIBS = $(6))

$(call _TRANSLATE1,$(1),$(TRANSLATE_SRCDIR),$(TRANSLATE_DSTDIR),\
$(TRANSLATE_IN),$(TRANSLATE_OUT),$(TRANSLATE_LIBS))

$(call BUILDS,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_DSTDIR),$(TRANSLATE_OUT),\
$(TRANSLATE_OUT),$(TRANSLATE_LIBS) $(X86_SYSCALL_O))
endef

