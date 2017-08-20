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
		$$($(1)_CLANG) $(C2BC_CLANG_FLAGS) $(EMITLLVM) $(2)$(4).c -o $(5).bc
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

# DIS
# 1: arch
# 2: dir/
# 3: module
define DIS
$(2)$(3).ll: $(2)$(3).bc
	cd $(2) && \
		$(LLVMDIS) $(3).bc -o $(3).ll
endef


# OPT
# 1: arch
# 2: dir/
# 3: input (.bc)
# 4: output (.bc)
define OPT
$(2)$(4).bc: $(2)$(3).bc $(2)$(3).ll
	cd $(2) && \
		$(LLVMOPT) $(LLVMOPT_FLAGS) $(3).bc -o $(4).bc
endef


# BC2S
# 1: arch
# 2: dir/
# 3: input (.bc)
# 4: output (.s)
define BC2S
$(2)$(4).s: $(2)$(3).bc $(2)$(3).ll
	cd $(2) && \
		$(LLC) $(LLC_FLAGS) $($(1)_LLC_FLAGS) $(3).bc -o $(4).s
endef


# C2O
# 1: arch
# 2: source dir/
# 3: out dir/
# 4: input (.c)
# 5: output (.o)
define C2O
$(3)$(5).o: $(2)$(4).c
	cd $(3) && \
		$($(1)_GCC) -static -O3 -fomit-frame-pointer -c $(2)$(4).c -o $(5).o

endef


# S2O
# 1: arch
# 2: source dir/
# 3: out dir/
# 4: module (module.s -> module.o)
define S2O
$(3)$(4).o: $(2)$(4).s
	cd $(3) && \
		$$($(1)_AS) -o $(4).o -c $(2)$(4).s
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


# CLINK
# 1: arch
# 2: dir/
# 3: objs
# 4: output
# 5: libs
define CLINK
$(eval CL_OBJS = $(addsuffix .o,$(3)))

$(2)$(4): $(addprefix $(2),$(CL_OBJS))
	cd $(2) && \
	$$($(1)_LD) -o $$@ \
		$$($(1)_LD_FLAGS0) $(CL_OBJS) $(5) \
		$$($(1)_LD_FLAGS1)
endef

define CLINKS
$(eval CL_SRCS = $(addsuffix .s,$(3)))

$(2)$(4): $(addprefix $(2),$(CL_SRCS))
	cd $(2) && \
		$($(1)_GCC) -static -O3 -o $$@ $(CL_SRCS) $(5)
endef

define GCC_LINK
$(eval CL_OBJS = $(addsuffix .o,$(3)))

$(warning $(2)$(4):)
$(2)$(4): $(addprefix $(2),$(CL_OBJS))
	cd $(2) && \
		$($(1)_GCC) -static -o $(4) $(CL_OBJS) $(5)

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
###
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

# multi C2BC and link
define BUILD2
$(eval BUILD2_PREFIX = $($(1)_PREFIX))

ifeq ($(words $(4)),1)
$(call C2BC,$(1),$(2),$(3),$(4),$(5))
else
$(foreach c,$(4),\
$(call C2BCNDIS,$(1),$(2),$(3),$(c),$(BUILD2_PREFIX)-$(c)_bc1))

$(call LLLINK,$(1),$(3),$(foreach bc,$(4),$(BUILD2_PREFIX)-$(bc)_bc1),$(5))
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

#define CBUILDS
#$(call S2O,$(1),$(2),$(3),$(5))
#$(call CLINK,$(1),$(3),$(5),$(5),$(6))
#$(call RUN,$(1),$(3),$(5),save-run.out)
#$(call ALIAS,$(3),$(5))
#endef

define CBUILDS
$(call CLINKS,$(1),$(3),$(5),$(5),$(6))
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
$(call C2O,$(1),$(2),$(3),$(c),$(BUILD_PREFIX)-$(c))))

$(call GCC_LINK,$(1),$(3),$(addprefix $(BUILD_PREFIX)-,$(4)),$(5),)
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


# TRANSLATE OBJ
# 1: host arch
# 2: dir
# 3: input (.o)
# 4: output (.bc)
define TRANSLATE_OBJ
$(2)$(4).bc: $(2)$(3).o
	cd $(2) && \
		riscv-sbt $(SBTFLAGS) -o $(4).bc $(3).o
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

