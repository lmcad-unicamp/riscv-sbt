### rules ###

# CLEAN
# 1: module
# 2: clean .s?
define CLEAN
clean-$(1):
	rm -f $(1) $(1).o $$(if $(2),$(1).s) $(1).bc $(1).out $(CLEAN_EXTRA)
###
endef

# RUN
# 1: prefix
# 2: program
# 3: save output?
define RUN
run-$(2): $(2)
ifeq ($$($(1)_RUN),)
  ifeq ($(3),)
	./$(2)
  else
	./$(2) | tee $(2).out
  endif
else
  ifeq ($(3),)
	$$($(1)_RUN) $(2)
  else
	$$($(1)_RUN) $(2) | tee $(2).out
  endif
endif
endef

# Assemble
# 1: prefix
# 2: output module
define AS
# .s -> .o
$(2).o: $(2).s
	$$($(1)_AS) -o $$@ -c $$<
endef

# C program link
# 1: prefix
# 2: output module
define CLINK
$(2): $(2).o
	$$($(1)_LD) -o $$@ \
		$$($(1)_LD_FLAGS0) $$< \
		$($(1)_LIBS) \
		$$($(1)_LD_FLAGS1)
endef

# assembly program link
# 1: prefix
# 2: output module
define LINK
$(2): $(2).o
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
define BUILD
# .c -> .s
ifneq ($(4),)
$(2).s: $(4).c
	$$($(1)_$(3)) $$($(1)_$(3)_FLAGS) $(CFLAGS) -o $$@ -S $$<
endif

$(call AS,$(1),$(2))

# .o -> elf
ifneq ($(4),)
$(call CLINK,$(1),$(2))
else
$(call LINK,$(1),$(2))
endif

$(call RUN,$(1),$(2))
$(call CLEAN,$(2),$(4))
endef

# TRANSLATE BINARY
# 1: guest arch
# 2: host arch
# 3: module name, without extension and arch prefix
define TRANSLATE
# .o -> .bc
$($(1)_PREFIX)-$($(2)_PREFIX)-$(3).bc: $($(1)_PREFIX)-$(3).o
	riscv-sbt -o $$@ $$<

# .bc -> .s
$($(1)_PREFIX)-$($(2)_PREFIX)-$(3).s: $($(1)_PREFIX)-$($(2)_PREFIX)-$(3).bc
	llc -O0 -o $$@ -march $($(2)_MARCH) $$<

$(eval $(2)_LIBS = $($(2)_SYSCALL_O) $($(2)_RVSC_O))
$(call ASNCLINK,$(2),$($(1)_PREFIX)-$($(2)_PREFIX)-$(3),clean.s,save-run.out)
$(eval $(2)_LIBS =)

test-$(3): run-$($(1)_PREFIX)-$(3) run-$($(1)_PREFIX)-$($(2)_PREFIX)-$(3)
	diff $($(1)_PREFIX)-$(3).out $($(1)_PREFIX)-$($(2)_PREFIX)-$(3).out
endef

# NBUILD: native build
# 1: output module
define NBUILD
$(1).o: $(1).cpp $(1).hpp
	$(CXX) $(CXXFLAGS) -o $$@ -c $$<

$(1): $(1).o
	$(CXX) $(LDFLAGS) -o $$@ $$<

$(call RUN,X86,$(1))
$(call CLEAN,$(1),$(1))
endef
