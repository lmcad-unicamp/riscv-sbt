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

$(DSTDIR)/$(TGT_OUT): $(foreach in,$(TGT_INS),$(TGT_SRCDIR)/$(in)$(TGT_SUFFIX))
	$(BUILD_PY) $(TGT_GFLAGS) build $(TGT_LFLAGS) $(TGT_ARCH) \
		$(TGT_SRCDIR) $(TGT_DSTDIR) $(TGT_OUT) $(TGT_INS)

.PHONY: $(TGT_OUT)
$(TGT_OUT): $(DSTDIR)/$(TGT_OUT)

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

$(T1_DIR)/$(T1_OUT).s: $(T1_DIR)/$(T1_IN).o
	$(BUILD_PY) $(T1_GFLAGS) translate $(T1_ARCH) $(T1_DIR) $(T1_DIR) \
		$(T1_OUT) $(T1_IN) $(T1_LFLAGS)

$(call TGT,$(T1_ARCH),$(T1_DIR),$(T1_DIR),$(T1_OUT),$(T1_OUT),\
--asm $(T1_GFLAGS),--syscall,$(T1_RFLAGS))

endef


define TRANSLATE
$(eval T_ARCH = $(1))
$(eval T_DIR = $(2))
$(eval T_IN = $(3))
$(eval T_OUT = $(4))
$(eval T_GFLAGS = $(5))
$(eval T_RFLAGS = $(6))

$(foreach mode,$(MODES),\
$(call _TRANSLATE1,$(T_ARCH),$(T_DIR),$(T_IN),\
$(T_OUT)-$(mode),$(T_GFLAGS),-- -regs=$(mode),$(T_RFLAGS)))

.PHONY: $(T_OUT)
$(T_OUT): $(foreach mode,$(MODES),$(T_OUT)-$(mode))

.PHONY: $(T_OUT)-run
$(T_OUT)-run: $(foreach mode,$(MODES),$(T_OUT)-$(mode)-run)

endef
