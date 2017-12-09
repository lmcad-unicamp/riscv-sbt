# rules constants
ARCHS    := rv32-linux x86
PREFIXES := rv32 x86


# build
define BLD
$(eval BLD_ARCH = $(1))
$(eval BLD_SRCDIR = $(2))
$(eval BLD_DSTDIR = $(3))
$(eval BLD_INS = $(4))
$(eval BLD_OUT = $(5))
$(eval BLD_GFLAGS = $(6))
$(eval BLD_LFLAGS = $(7))

$(eval BLD_SUFFIX = $(if $(findstring --asm,$(BLD_GFLAGS)),.s,.c))

# build
$(BLD_DSTDIR)/$(BLD_OUT): $(foreach in,$(BLD_INS),$(BLD_SRCDIR)/$(in)$(BLD_SUFFIX))
	$(BUILD_PY) $(BLD_GFLAGS) build $(BLD_LFLAGS) $(BLD_ARCH) \
		$(BLD_SRCDIR) $(BLD_DSTDIR) $(BLD_OUT) $(BLD_INS)

# alias
.PHONY: $(BLD_OUT)
$(BLD_OUT): $(BLD_DSTDIR)/$(BLD_OUT)

# clean
$(BLD_OUT)-clean:
	$(BUILD_PY) $(BLD_GFLAGS) clean $(BLD_DSTDIR) $(BLD_OUT)

endef


# build all targets alias
define BLDALL
$(eval BA_PREFIXES = $(subst -linux,,$(1)))
$(eval BA_NAME = $(2))
$(eval BA_DSTDIR = $(3))

.PHONY: $(BA_NAME)
$(BA_NAME): $(foreach PREFIX,$(BA_PREFIXES),$(BA_DSTDIR)/$(PREFIX)-$(BA_NAME)) \
            $(foreach MODE,$(MODES),$(BA_DSTDIR)/rv32-x86-$(BA_NAME)-$(MODE))

endef


# run
define RUN
$(eval RUN_ARCH = $(1))
$(eval RUN_DIR = $(2))
$(eval RUN_PROG = $(3))
$(eval RUN_FLAGS = $(4))
$(eval RUN_OUT = $(5))

#$(eval RUN_FLAGS = $(RUN_FLAGS) \
#$(if $(findstring --save-output,$(RUN_FLAGS)),-o $(RUN_OUT).out,))

.PHONY: $(RUN_OUT)-run
$(RUN_OUT)-run: $(RUN_PROG)
	$(BUILD_PY) run $(RUN_ARCH) $(RUN_DIR) $(RUN_PROG) $(RUN_FLAGS)

endef


# run all
define RUNALL
$(eval RA_PREFIXES = $(1))
$(eval RA_NAME = $(2))

.PHONY: $(RA_NAME)-run
$(RA_NAME)-run: $(foreach PREFIX,$(RA_PREFIXES),$(PREFIX)-$(RA_NAME)) \
                $(foreach MODE,$(MODES),rv32-x86-$(RA_NAME)-$(MODE))

endef


# build and run
define BLDNRUN
$(eval BR_ARCH = $(1))
$(eval BR_SRCDIR = $(2))
$(eval BR_DSTDIR = $(3))
$(eval BR_INS = $(4))
$(eval BR_OUT = $(5))
$(eval BR_GFLAGS = $(6))
$(eval BR_LFLAGS = $(7))
$(eval BR_RFLAGS = $(8))

$(call BLD,$(BR_ARCH),$(BR_SRCDIR),$(BR_DSTDIR),$(BR_INS),$(BR_OUT),\
$(BR_GFLAGS),$(BR_LFLAGS))

$(call RUN,$(BR_ARCH),$(BR_DSTDIR),$(BR_OUT),$(BR_RFLAGS),$(BR_OUT))
endef


# measure
define MEASURE
$(eval M_DIR = $(1))
$(eval M_PROG = $(2))
$(eval M_ARGS = $(3))
$(eval M_OUT = $(4))

.PHONY: $(M_OUT)-measure
$(M_OUT)-measure:
	$(MEASURE_PY) $(M_DIR) $(M_PROG) $(M_ARGS)

endef


# translate
define _TRANSLATE1
$(eval T1_ARCH = $(1))
$(eval T1_DIR = $(2))
$(eval T1_IN = $(3))
$(eval T1_OUT = $(4))
$(eval T1_GFLAGS = $(5))
$(eval T1_LFLAGS = $(6))
$(eval T1_TGFLAGS = $(7))
$(eval T1_TLFLAGS = $(8))

$(T1_DIR)/$(T1_OUT).s: $(T1_DIR)/$(T1_IN).o
	$(BUILD_PY) $(T1_TGFLAGS) translate $(T1_ARCH) $(T1_DIR) $(T1_DIR) \
		$(T1_OUT) $(T1_IN) $(T1_TLFLAGS)

$(call BLD,$(T1_ARCH),$(T1_DIR),$(T1_DIR),$(T1_OUT),$(T1_OUT),\
--asm $(T1_GFLAGS),--syscall $(T1_LFLAGS))

endef


define TRANSLATE
$(eval T_ARCH = $(1))
$(eval T_DIR = $(2))
$(eval T_IN = $(3))
$(eval T_OUT = $(4))
$(eval T_GFLAGS = $(5))
$(eval T_LFLAGS = $(6))
$(eval T_TLFLAGS = $(7))

$(eval T_TGFLAGS = $(filter-out --asm,$(T_GFLAGS)))

$(foreach mode,$(MODES),\
$(call _TRANSLATE1,$(T_ARCH),$(T_DIR),$(T_IN),$(T_OUT)-$(mode),\
$(T_GFLAGS),$(T_LFLAGS),\
$(T_TGFLAGS),-- -regs=$(mode) $(T_TLFLAGS)))

.PHONY: $(T_OUT)
$(T_OUT): $(foreach mode,$(MODES),$(T_OUT)-$(mode))

endef


# diff tests
define DIFF
diff $(1)/$(2).out $(1)/$(3).out
	
endef


# test
define TEST
$(eval TEST_PREFIXES = $(1))
$(eval TEST_NAME = $(2))
$(eval TEST_DIR = $(3))

.PHONY: $(TEST_NAME)-test
$(TEST_NAME)-test: $(addsuffix -$(TEST_NAME)-run,$(TEST_PREFIXES) rv32-x86)
	$(foreach PREFIX,$(TEST_PREFIXES),\
$(foreach MODE,$(MODES),\
$(call DIFF,$(TEST_DIR),$(PREFIX)-$(TEST_NAME),rv32-x86-$(TEST_NAME)-$(MODE))))

endef


# unit test
define UTEST
$(eval UT_NARCHS = $(1))
$(eval UT = $(2))
$(eval UT_SRCDIR = $(3))
$(eval UT_DSTDIR = $(4))
$(eval UT_GFLAGS = $(5))
$(eval UT_LFLAGS = $(6))
$(eval UT_RFLAGS = $(7))
$(eval UT_TLFLAGS = $(8))
$(eval UT_PREF = $(9))

$(eval UT_PREFIXES = $(subst -linux,,$(UT_NARCHS)))

$(foreach UT_ARCH,$(UT_NARCHS),\
$(call BLDNRUN,$(UT_ARCH),$(UT_SRCDIR),$(UT_DSTDIR),\
$(if $(UT_PREF),$(UT_ARCH)-,)$(UT),\
$(UT_ARCH)-$(UT),\
$(UT_GFLAGS),$(UT_LFLAGS),$(UT_RFLAGS)))

$(call TRANSLATE,x86,$(UT_DSTDIR),rv32-$(UT),rv32-x86-$(UT),\
$(UT_GFLAGS),$(UT_LFLAGS),$(UT_TLFLAGS))

$(call RUN,x86,$(UT_DSTDIR),rv32-x86-$(UT),$(UT_RFLAGS),rv32-x86-$(UT))

$(call BLDALL,$(UT_NARCHS) rv32-x86,$(UT),$(UT_DSTDIR))

$(call TEST,$(UT_PREFIXES),$(UT),$(UT_DSTDIR))
endef

