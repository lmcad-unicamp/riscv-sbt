### rules ###

# $(warning CLEAN,$(1),$(2))

# CLEAN
# 1: module
# 2: clean .s?
define CLEAN
clean-$(1):
	@echo $$@:
	rm -f $(addprefix $(MAKE_DIR),\
		$(1) $(1).o $(if $(2),$(1).s) $(1).bc $(1).ll $(1).out $(CLEAN_EXTRA))
###
endef

# RUN
# 1: prefix
# 2: program
# 3: save output?
define RUN
$(eval RUN_DIR = $(if $(MAKE_DIR),$(MAKE_DIR),./))
$(eval RUN_PROG = $(RUN_DIR)$(2))
run-$(2): $(RUN_PROG)
	@echo $$@:
ifeq ($$($(1)_RUN),)
  ifeq ($(3),)
	$(RUN_PROG)
  else
	$(RUN_PROG) | tee $(RUN_PROG).out
  endif
else
  ifeq ($(3),)
	$$($(1)_RUN) $(RUN_PROG)
  else
	$$($(1)_RUN) $(RUN_PROG) | tee $(RUN_PROG).out
  endif
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

# C program link
# 1: prefix
# 2: output module
define CLINK
$(eval CLINK_FILE = $(MAKE_DIR)$(2))
$(CLINK_FILE): $(CLINK_FILE).o
	$$($(1)_LD) -o $$@ \
		$$($(1)_LD_FLAGS0) $$< \
		$($(1)_LIBS) \
		$$($(1)_LD_FLAGS1)
endef

# assembly program link
# 1: prefix
# 2: output module
define LINK
$(eval LINK_FILE = $(MAKE_DIR)$(2))
$(LINK_FILE): $(LINK_FILE).o
	$$($(1)_LD) -o $$@ $$< $($(1)_LIBS)
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

# BUILD
# 1: prefix
# 2: output module
# 3: [opt] GCC/CLANG
# 4: [opt] source .c file
# 5: save run output?
define BUILD
$(eval BUILD_FILE = $(MAKE_DIR)$(2))

# .c -> .s
ifneq ($(4),)
$(BUILD_FILE).s: $(MAKE_DIR)$(4).c
	$$($(1)_$(3)) $$($(1)_$(3)_FLAGS) $(CFLAGS) -o $$@ -S $$<
endif

$(call AS,$(1),$(2))

# .o -> elf
ifneq ($(4),)
$(call CLINK,$(1),$(2))
else
$(call LINK,$(1),$(2))
endif

$(call RUN,$(1),$(2),$(5))
$(call CLEAN,$(2),$(4))
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
	riscv-sbt $(SBTFLAGS) -o $$@ $$<; rc=$$$$?; \
		$(LLVM_INSTALL_DIR)/bin/llvm-dis -o $(TRANSLATE_FILE).ll $$@; \
		if [ $$$$rc -ne 0 ]; then false; else true; fi

# .bc -> .s
$(TRANSLATE_FILE).s: $(TRANSLATE_FILE).bc $(TRANSLATE_FILE).ll
	llc $(LLCFLAGS) -o $$@ -march $($(2)_MARCH) $$<
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

$(eval $(2)_LIBS = $($(2)_SYSCALL_O) $($(2)_RVSC_O))
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

$(eval $(2)_LIBS = $($(2)_SYSCALL_O) $($(2)_RVSC_O) $($(2)_DUMMY_O))
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
