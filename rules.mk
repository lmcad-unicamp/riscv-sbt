### rules ###

# CLEAN
# 1: module
# 2: clean .s?
define CLEAN
clean-$(1):
	rm -f $(1) $(1).o $$(if $(2),$(1).s,) $(1).bc $(CLEAN_EXTRA)
endef

# RUN
# 1: prefix
# 2: program
define RUN
run-$(2): $(2)
ifeq ($$($(1)_RUN),)
	./$(2)
else
	$$($(1)_RUN) $(2)
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
