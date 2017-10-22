### rules ###


# C2BC
# 1: arch
# 2: source dir/
# 3: out dir/
# 4: input (.c)
# 5: output (.bc)
define C2BC
$(eval C2BC_CLANG_FLAGS = $$($(1)_CLANG_FLAGS))
$(3)$(5).bc: $(2)$(4).c
	cd $(3) && \
		$$($(1)_CLANG) $(C2BC_CLANG_FLAGS) $(BUILD_CFLAGS) $(EMITLLVM) $(2)$(4).c -o $(5).bc

endef


# LLVM LINK
# 1: arch
# 2: dir/
# 3: inputs (.bc)
# 4: output (.bc)
define LLLINK
$(2)$(4).bc: $(foreach bc,$(3),$(2)$(bc).bc)
	cd $(2) && \
		$(LLVMLINK) $(foreach bc,$(3),$(bc).bc) -o $(4).bc

endef


define DIS
$(eval DIS_DIR = $(2))
$(eval DIS_MOD = $(3))

$(DIS_DIR)$(DIS_MOD).ll: $(DIS_DIR)$(DIS_MOD).bc
	$(LLVMDIS) $$^ -o $$@

endef


define OPT
$(eval OPT_DIR = $(2))
$(eval OPT_IN = $(3))
$(eval OPT_OUT = $(4))

$(OPT_DIR)$(OPT_OUT).bc: $(OPT_DIR)$(OPT_IN).bc $(OPT_DIR)$(OPT_IN).ll
	$(LLVMOPT) $(LLVMOPT_FLAGS) $$< -o $$@

endef


define BC2S
$(eval BC2S_DIR = $(2))
$(eval BC2S_IN = $(3))
$(eval BC2S_OUT = $(4))

$(BC2S_DIR)$(BC2S_OUT).s: $(BC2S_DIR)$(BC2S_IN).bc $(BC2S_DIR)$(BC2S_IN).ll
	$(LLC) $(LLC_FLAGS) $($(1)_LLC_FLAGS) $$< -o $$@

endef


define S2O
$(eval S2O_SRCDIR = $(2))
$(eval S2O_DSTDIR = $(3))
$(eval S2O_MOD = $(4))

$(S2O_DSTDIR)$(S2O_MOD).o: $(S2O_SRCDIR)$(S2O_MOD).s
	$$($(1)_AS) -o $$@ -c $$<

endef


# LINK
# 1: arch
# 2: dir/
# 3: objs
# 4: output
# 5: libs
define LINK
$(2)$(4): $(addprefix $(2),$(addsuffix .o,$(3)))
	cd $(2) && \
		$$($(1)_LD) -o $(4) $(addsuffix .o,$(3)) $(5)

endef


define CLINK
$(eval CL_SRCDIR = $(2))
$(eval CL_DSTDIR = $(3))
$(eval CL_MODS = $(4))
$(eval CL_OUT = $(5))
$(eval CL_LIBS = $(6))
$(eval CL_OBJS = $(addsuffix .o,$(CL_MODS)))

$(CL_DSTDIR)$(CL_OUT): $(addprefix $(CL_SRCDIR),$(CL_OBJS))
	$($(1)_GCC) -o $$@ $$^ $(CL_LIBS)

endef


define CLINKS
$(eval CL_SRCDIR = $(2))
$(eval CL_DSTDIR = $(3))
$(eval CL_OBJS = $(4))
$(eval CL_OUT = $(5))
$(eval CL_LIBS = $(6))
$(eval CL_SRCS = $(addsuffix .s,$(CL_OBJS)))

$(CL_DSTDIR)$(CL_OUT): $(addprefix $(CL_SRCDIR),$(CL_SRCS))
	$($(1)_GCC) $(GCC_CFLAGS) -o $$@ $$^ $(CL_LIBS) -lm

endef


# C2O
# 1: arch
# 2: source dir/
# 3: out dir/
# 4: input (.c)
# 5: output (.o)
define GCC_C2O
$(3)$(5).o: $(2)$(4).c
	cd $(3) && \
		$($(1)_GCC) $(GCC_CFLAGS) $(BUILD_CFLAGS) -c $(2)$(4).c -o $(5).o

endef


# GCC_LINK
# 1: arch
# 2: dir/
# 3: objs
# 4: output
# 5: libs
define GCC_LINK
$(eval CL_OBJS = $(addsuffix .o,$(3)))

$(2)$(4): $(addprefix $(2),$(CL_OBJS))
	cd $(2) && \
		$($(1)_GCC) $(GCC_CFLAGS) -o $(4) $(CL_OBJS) $(5) -lm

endef


# CLEAN
# 1: dir
# 2: module
# 3: clean .s?
define CLEAN
clean-$(2):
	@echo $$@:
	cd $(1) && \
		rm -f $(2) $(2).o $(if $(3),$(2).s,) $(2).bc $(2).ll \
			$(2).opt.bc $(2).opt.ll $(2).opt2.bc $(2).opt2.ll $(2).out

endef


# RUN
# 1: arch
# 2: dir/
# 3: program
# 4: save output?
define RUN
run-$(3): $(2)$(3)
	@echo $$@:
  ifeq ($(4),)
	$$($(1)_RUN) $(2)$(3)
  else
	$$($(1)_RUN) $(2)$(3) | tee $(2)$(3).out
  endif

endef


# ALIAS
# 1: out dir/
# 2: bin
define ALIAS
ifneq ($(1),./)
.PHONY: $(2)
$(2): $(1) $(1)$(2)
endif

endef


# TEST
# 1: host arch
# 2: dir/
# 3: module
define TEST
$(eval TEST_RV32 = $(RV32_PREFIX)-$(3))
$(eval TEST_TRANS = $(RV32_PREFIX)-$($(1)_PREFIX)-$(3))

test-$(3): run-$(TEST_RV32) run-$(TEST_TRANS)
	diff $(2)$(TEST_RV32).out $(2)$(TEST_TRANS).out

endef


# BUILD*
# 1: arch
# 2: source dir/
# 3: out dir/
# 4: input files (.c)
# 5: output file (bin)
# 6: libs

define C2BCNDIS
$(call C2BC,$(1),$(2),$(3),$(4),$(5))
$(call DIS,$(1),$(3),$(5))
endef

# 1: prefix
# 2: path
define ADD_PREFIX
$(dir $(2))$(1)-$(notdir $(2))
endef

# multi C2BC and link
define BUILD2
$(eval BUILD2_PREFIX = $($(1)_PREFIX))

ifeq ($(words $(4)),1)
$(call C2BC,$(1),$(2),$(3),$(4),$(5))
else
$(foreach c,$(4),\
$(call C2BCNDIS,$(1),$(2),$(3),$(c),$(call ADD_PREFIX,$(BUILD2_PREFIX),$(c)_bc1)))

$(call LLLINK,$(1),$(3),$(foreach bc,$(4),$(call ADD_PREFIX,$(BUILD2_PREFIX),$(bc)_bc1)),$(5))
endif

$(call DIS,$(1),$(3),$(5))
endef

# OPT/DIS x2 then BC2S
define BUILD3
$(call OPT,$(1),$(3),$(5),$(5).opt)
$(call DIS,$(1),$(3),$(5).opt)
$(call OPT,$(1),$(3),$(5).opt,$(5).opt2)
$(call DIS,$(1),$(3),$(5).opt2)
$(call BC2S,$(1),$(3),$(5).opt2,$(5))
endef

define BUILD1
$(call BUILD2,$(1),$(2),$(3),$(4),$(5),$(6))
$(call BUILD3,$(1),$(2),$(3),$(4),$(5),$(6))
endef

define BUILDS
$(call S2O,$(1),$(2),$(3),$(5))
$(call LINK,$(1),$(3),$(5),$(5),$(6))
$(call RUN,$(1),$(3),$(5),save-run.out)
$(call ALIAS,$(3),$(5))
endef

define CBUILDS
$(call CLINKS,$(1),$(2),$(3),$(5),$(5),$(6))
$(call RUN,$(1),$(3),$(5),save-run.out)
$(call ALIAS,$(3),$(5))
endef

define BUILD
$(call BUILD1,$(1),$(2),$(3),$(4),$(5),$(6))
$(call CBUILDS,$(1),$(3),$(3),$(5),$(5),$(6))
endef

define GCC_BUILD
$(eval BUILD_PREFIX = $($(1)_PREFIX))
$(eval $(foreach c,$(4),\
$(call GCC_C2O,$(1),$(2),$(3),$(c),$(call ADD_PREFIX,$(BUILD_PREFIX),$(c)))))

$(call GCC_LINK,$(1),$(3),\
$(foreach o,$(4),$(call ADD_PREFIX,$(BUILD_PREFIX),$(o))),$(5),)

$(call RUN,$(1),$(3),$(5),save-run.out)
$(call ALIAS,$(3),$(5))
endef

# build object file
# optimize only one time
define BUILDO
$(call C2BC,$(1),$(2),$(3),$(4),$(5))
$(call DIS,$(1),$(3),$(5))
$(call OPT,$(1),$(3),$(5),$(5).opt)
$(call DIS,$(1),$(3),$(5).opt)
$(call BC2S,$(1),$(3),$(5).opt,$(5))
$(call S2O,$(1),$(3),$(3),$(5))
endef


# build object file for translation
# (don't use sysroot, use native header/libraries instead)
define BUILDO4SBT
$(eval BUILDO4SBT_$(1)_CLANG_FLAGS = $($(1)_CLANG_FLAGS))
$(eval $(1)_CLANG_FLAGS = )
$(call BUILDO,$(1),$(2),$(3),$(4),$(5))
$(eval $(1)_CLANG_FLAGS = $(BUILDO4SBT_$(1)_CLANG_FLAGS))
endef


define TRANSLATE_OBJ
$(eval TO_DIR = $(2))
$(eval TO_IN = $(3))
$(eval TO_OUT = $(4))

$(TO_DIR)$(TO_OUT).bc: $(TO_DIR)$(TO_IN).o
	riscv-sbt $(SBTFLAGS) -o $$@ $$^ &> $(TOPDIR)/sbt.log

endef


# TRANSLATE C object file
# 1: arch
# 2: source dir/
# 3: out dir/
# 4: input file (.o)
# 5: output file (bin)
# 6: libs

define TRANSLATE1
$(call TRANSLATE_OBJ,$(1),$(3),$(4),$(5))
$(call DIS,$(1),$(3),$(5))
$(call OPT,$(1),$(3),$(5),$(5).opt)
$(call DIS,$(1),$(3),$(5).opt)
$(call BC2S,$(1),$(3),$(5).opt,$(5))
endef

define TRANSLATE
$(call TRANSLATE1,$(1),$(2),$(3),$(4),$(5),$(6))
$(call CBUILDS,$(1),$(3),$(3),$(5),$(5),$(6) $(X86_SYSCALL_O))
endef

define TRANSLATE_ASM
$(call TRANSLATE1,$(1),$(2),$(3),$(4),$(5),$(6))
$(call BUILDS,$(1),$(3),$(3),$(5),$(5),$(6) $(X86_SYSCALL_O))
endef

