### rules ###

# assembly program link
# 1: prefix
# 2: dir/
# 3: modules
# 4: output
define LINK
$(2)$(4): $(addprefix $(2),$(addsuffix .o,$(3)))
	cd $(2) && \
		$$($(1)_LD) -o $(4) $(addsuffix .o,$(3))
endef

# C program link
# 1: prefix (X86/RV32)
# 2: output dir
# 3: modules
# 4: output file
define CLINK
$(eval CL_OBJS = $(addsuffix .o,$(3)))

$(2)$(4): $(addprefix $(2),$(CL_OBJS))
	cd $(2) && \
	$$($(1)_LD) -o $$@ \
		$$($(1)_LD_FLAGS0) $(CL_OBJS) \
		$$($(1)_LD_FLAGS1)
endef


# CLEAN
# 1: dir
# 2: module
# 3: clean .s?
define CLEAN
clean-$(2):
	@echo $$@:
	cd $(1) && \
		rm -f $(2) $(2).o $(if $(3),$(2).s,) $(2).bc $(2).ll $(2).out
###
endef

# RUN
# 1: prefix
# 2: dir/
# 3: program
# 4: save output?
define RUN
run-$(3): $(2)$(3)
	@echo $$@:
  ifeq ($(4),)
	$$($(1)_RUN) $(2)$(3)
  else
	$$($(1)_RUN) $(2)$(3) | tee $(3).out
  endif
endef

# Assemble
# 1: prefix
# 2: output module
define AS
$(eval AS_FILE = $(MAKE_DIR)$(2))
$(AS_FILE).o: $(AS_FILE).s
	$$($(1)_AS) -o $$@ -c $$<
endef



# Assemble and link with C libs
# 1: prefix
# 2: output module
# 3: clean .s?
# 4: save run output?
define ASNCLINK
$(call AS,$(1),$(2))
$(call CLINK,$(1),$(2))
$(call RUN,$(1),$(2),$(4))
$(call CLEAN,$(2),$(3))
endef


# TRANSLATE BINARY
# 1: guest arch
# 2: host arch
# 3: module name, without extension and arch prefix
# 4: flag: no prefix in module
define TRANSLATE
$(eval TRANSLATE_FILE = $(MAKE_DIR)$($(1)_PREFIX)-$($(2)_PREFIX)-$(3))
$(eval TRANSLATE_GUEST = $(MAKE_DIR)$(if $(4),$(3),$($(1)_PREFIX)-$(3)))

# .o -> .bc
$(TRANSLATE_FILE).bc: $(TRANSLATE_GUEST).o
	riscv-sbt $(SBTFLAGS) -o $$@ $$<
	$(LLVM_INSTALL_DIR)/bin/llvm-dis -o $(TRANSLATE_FILE).ll $$@

# .bc -> .s
$(TRANSLATE_FILE).s: $(TRANSLATE_FILE).bc $(TRANSLATE_FILE).ll
	$(LLVM_INSTALL_DIR)/bin/opt -O3 $(TRANSLATE_FILE).bc -o $(TRANSLATE_FILE).opt.bc
	$(LLVM_INSTALL_DIR)/bin/llvm-dis -o $(TRANSLATE_FILE).opt.ll $(TRANSLATE_FILE).opt.bc
	llc -o $$@ -march $($(2)_MARCH) $(TRANSLATE_FILE).opt.bc
endef


# TRANSLATE RUN target
# 1: guest arch
# 2: host arch
# 3: module name, without extension and arch prefix
define TRANSLATE_RUN
$(eval TRANSLATE_RUN_FILE = $(MAKE_DIR)$($(1)_PREFIX)-$($(2)_PREFIX)-$(3))
$(eval TRANSLATE_RUN_GUEST = $($(1)_PREFIX)-$(3))

test-$(3): run-$(TRANSLATE_RUN_GUEST) run-$(TRANSLATE_RUN_FILE)
	diff $(TRANSLATE_RUN_GUEST).out $(TRANSLATE_RUN_FILE).out
endef


# TRANSLATE from C
# 1: guest arch
# 2: host arch
# 3: module name, without extension and arch prefix
define TRANSLATE_C
$(eval TRANSLATE_C_FILE = $($(1)_PREFIX)-$($(2)_PREFIX)-$(3))

$(call TRANSLATE,$(1),$(2),$(3),)

$(eval $(2)_LIBS = $($(2)_SYSCALL_O) $($(2)_COUNTERS_O))
$(call ASNCLINK,$(2),$(TRANSLATE_C_FILE),clean.s,save-run.out)
$(eval $(2)_LIBS =)

$(call TRANSLATE_RUN,$(1),$(2),$(3))
endef


# TRANSLATE from ASM
# 1: guest arch
# 2: host arch
# 3: module name, without extension and arch prefix
# 4: flag: no prefix in module
define TRANSLATE_ASM
$(eval TRANSLATE_ASM_FILE = $($(1)_PREFIX)-$($(2)_PREFIX)-$(3))

$(call TRANSLATE,$(1),$(2),$(3),$(4))

$(eval $(2)_LIBS = $($(2)_SYSCALL_O) $($(2)_DUMMY_O))
$(eval $(call BUILD,$(2),$(TRANSLATE_ASM_FILE)))
$(eval $(2)_LIBS =)

$(call TRANSLATE_RUN,$(1),$(2),$(3))
endef


# NBUILD: native build
# 1: output module
define NBUILD
$(eval NBUILD_FILE = $(MAKE_DIR)$(1))

$(NBUILD_FILE).o: $(NBUILD_FILE).cpp $(NBUILD_FILE).hpp
	$(CXX) $(CXXFLAGS) -o $$@ -c $$<

$(NBUILD_FILE): $(NBUILD_FILE).o
	$(CXX) $(LDFLAGS) -o $$@ $$<

$(call RUN,X86,$(1))
$(call CLEAN,$(1),$(1))
endef

###
### MIBENCH
###

# 1: prefix (X86/RV32)
# 2: source dir/
# 3: out dir/
# 4: input file (.c)
# 5: output file (.bc)
define C2BC
$(3)$(5).bc: $(2)$(4).c
	cd $(3) && \
		$(CLANG) $$($(1)_CLANG_FLAGS) $(EMITLLVM) $(4).c -o $(5).bc
endef

# 1: prefix (X86/RV32)
# 2: dir/
# 3: module
define DIS
$(2)$(3).ll: $(2)$(3).bc
	cd $(2) && \
		$(DIS) $(3).bc -o $(3).ll
endef

### $(LLVMLINK) ${FILE1_bcs} -o $(BENCHNAME).bc

# OPT (x2 for native)
# 1: prefix (X86/RV32)
# 2: dir/
# 3: module
define OPT
$(2)$(3).opt.bc: $(2)$(3).bc $(2)$(3).ll
	cd $(2) && \
		$(OPT) $(OPT_FLAGS) $(3).bc -o $(3).opt.bc
endef

# 1: prefix (X86/RV32)
# 2: dir/
# 3: module
define BC2S
$(2)$(3).s: $(2)$(3).opt.bc $(2)$(3).opt.ll
	cd $(2) && \
		$(LLC) $(LLC_FLAGS) $($(1)_LLC_FLAGS) $(3).opt.bc -o $(3).s
endef

# 1: prefix (X86/RV32)
# 2: dir/
# 3: module (module.s -> module.o)
define S2O
$(2)$(3).o: $(2)$(3).s
	cd $(2) && \
		$$($(1)_AS) -o $(3).o -c $(3).s
endef

# 1: prefix (X86/RV32)
# 2: output dir
# 3: modules
# 4: output file
define CLINK2
$(eval CL2_OBJS = $(addsuffix .o,$(3)))

$(2)$(4): $(addprefix $(2),$(CL2_OBJS))
	cd $(2) && \
	$$($(1)_LD) -o $$@ \
		$$($(1)_LD_FLAGS0) $(CL2_OBJS) \
		$$($(1)_SYSCALL_O) \
		$($(1)_LIBS) \
		$$($(1)_LD_FLAGS1)
endef


# $(STATICBT) -target=$(ARCH) $(SBTOPT) -stacksize $(STACKSIZE) $(BENCHNAME)-oi.o -o=$(BENCHNAME)-oi.bc
# $(OPT) $(OPTFLAGS) $(BENCHNAME)-oi.bc -o $(BENCHNAME)-oi.bc

# TRANSLATE BINARY
# 1: host arch
# 2: dir
# 3: mod
define TRANSLATE2
$(2)$(3).bc: $(2)$(RV32_PREFIX)-$(3).o
	cd $(2) && \
		riscv-sbt $(SBTFLAGS) -o $(3).bc $(RV32_PREFIX)-$(3).o && \
		$(LLVM_INSTALL_DIR)/bin/llvm-dis -o $(3).ll $(3).bc

$(eval HOSTFILE = rv32-$$($(1)_PREFIX)-$(3))
$(2)$(HOSTFILE).s: $(2)$(3).bc $(2)$(3).ll
	cd $(2) && \
		$(OPT) -O3 $(3).bc -o $(3).opt.bc && \
		$(DIS) -o $(3).opt.ll $(3).opt.bc && \
		llc -o $(HOSTFILE).s -march $($(1)_MARCH) $(3).opt.bc

$(call S2O,$(1),$(2),$(2),$(HOSTFILE))

$(call CLINK2,$(1),$(2),$(HOSTFILE),$(HOSTFILE))
endef
