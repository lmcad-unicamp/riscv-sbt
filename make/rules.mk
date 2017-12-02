define TGT
$(eval TGT_ARCH = $(1))
$(eval TGT_SRCDIR = $(2))
$(eval TGT_DSTDIR = $(3))
$(eval TGT_INS = $(4))
$(eval TGT_OUT = $(5))
$(eval TGT_GFLAGS = $(6))
$(eval TGT_LFLAGS = $(7))
$(eval TGT_RFLAGS = $(8))

$(eval TGT_SUFFIX = $(if $(findstring --asm,$(TGT_GFLAGS)),.s,.c))

$(TGT_DSTDIR)/$(TGT_OUT): $(foreach in,$(TGT_INS),$(TGT_SRCDIR)/$(in)$(TGT_SUFFIX))
	$(BUILD_PY) $(TGT_GFLAGS) build $(TGT_LFLAGS) $(TGT_ARCH) \
		$(TGT_SRCDIR) $(TGT_DSTDIR) $(TGT_OUT) $(TGT_INS)

.PHONY: $(TGT_OUT)
$(TGT_OUT): $(TGT_DSTDIR)/$(TGT_OUT)

$(TGT_OUT)-run:
	$(BUILD_PY) run $(TGT_ARCH) $(TGT_DSTDIR) $(TGT_OUT) $(TGT_RFLAGS)

$(TGT_OUT)-clean:
	$(BUILD_PY) $(TGT_GFLAGS) clean $(TGT_DSTDIR) $(TGT_OUT)

endef


define _TRANSLATE1
$(eval T1_ARCH = $(1))
$(eval T1_DIR = $(2))
$(eval T1_IN = $(3))
$(eval T1_OUT = $(4))
$(eval T1_GFLAGS = $(5))
$(eval T1_LFLAGS = $(6))
$(eval T1_RFLAGS = $(7))
$(eval T1_TGFLAGS = $(8))
$(eval T1_TLFLAGS = $(9))

$(T1_DIR)/$(T1_OUT).s: $(T1_DIR)/$(T1_IN).o
	$(BUILD_PY) $(T1_TGFLAGS) translate $(T1_ARCH) $(T1_DIR) $(T1_DIR) \
		$(T1_OUT) $(T1_IN) $(T1_TLFLAGS)

$(call TGT,$(T1_ARCH),$(T1_DIR),$(T1_DIR),$(T1_OUT),$(T1_OUT),\
--asm $(T1_GFLAGS),--syscall $(T1_LFLAGS),$(T1_RFLAGS))

endef


define TRANSLATE
$(eval T_ARCH = $(1))
$(eval T_DIR = $(2))
$(eval T_IN = $(3))
$(eval T_OUT = $(4))
$(eval T_GFLAGS = $(5))
$(eval T_LFLAGS = $(6))
$(eval T_RFLAGS = $(7))
$(eval T_TLFLAGS = $(8))

$(eval T_TGFLAGS = $(filter-out --asm,$(T_GFLAGS)))

$(foreach mode,$(MODES),\
$(call _TRANSLATE1,$(T_ARCH),$(T_DIR),$(T_IN),$(T_OUT)-$(mode),\
$(T_GFLAGS),$(T_LFLAGS),$(T_RFLAGS),\
$(T_TGFLAGS),-- -regs=$(mode) $(T_TLFLAGS)))

.PHONY: $(T_OUT)
$(T_OUT): $(foreach mode,$(MODES),$(T_OUT)-$(mode))

.PHONY: $(T_OUT)-run
$(T_OUT)-run: $(foreach mode,$(MODES),$(T_OUT)-$(mode)-run)

endef


define TT
diff $(1)/$(2).out $(1)/$(3).out
	diff $(1)/$(2).out $(1)/$(4).out
	
endef


define TEST
$(eval TEST_NAME = $(1))
$(eval TEST_TGTS = $(2))
$(eval TEST_DIR = $(3))

.PHONY: $(TEST_NAME)
$(TEST_NAME): $(TEST_TGTS)
.PHONY: $(TEST_NAME)-run
$(TEST_NAME)-run: $(addsuffix -run,$(TEST_TGTS))

$(eval W  = $(words $(TEST_TGTS)))
$(eval T  = $(word $(W),$(TEST_TGTS)))
$(eval TS = $(filter-out $(T),$(TEST_TGTS)))
$(eval TG = $(T)-globals)
$(eval TL = $(T)-locals)

.PHONY: $(TEST_NAME)-test
$(TEST_NAME)-test: $(TEST_NAME)-run
	$(foreach t,$(TS),$(call TT,$(TEST_DIR),$(t),$(TG),$(TL)))

endef


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

$(foreach UT_ARCH,$(UT_NARCHS),\
$(call TGT,$(UT_ARCH),$(UT_SRCDIR),$(UT_DSTDIR),\
$(if $(UT_PREF),$(UT_ARCH)-,)$(UT),\
$(UT_ARCH)-$(UT),\
$(UT_GFLAGS),$(UT_LFLAGS),$(UT_RFLAGS)))

$(call TRANSLATE,x86,$(UT_DSTDIR),rv32-$(UT),rv32-x86-$(UT),\
$(UT_GFLAGS),$(UT_LFLAGS),$(UT_RFLAGS),$(UT_TLFLAGS))

$(call TEST,$(UT),$(foreach UT_ARCH,$(UT_NARCHS),$(UT_ARCH)-$(UT)) rv32-x86-$(UT),$(UT_DSTDIR))
endef

