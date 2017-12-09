###
### SUSAN TESTS
###

SUSAN_BIN     := $(SUSAN_NAME)
SUSAN_SRC_DIR := $(MIBENCH)/$(SUSAN_DIR)
SUSAN_OUT_DIR := $(BUILD_MIBENCH)/$(SUSAN_DIR)
SUSAN_INPUT   := $(SUSAN_SRC_DIR)/input_large.pgm
X86_SUSAN     := $(X86_PREFIX)-$(SUSAN_BIN)

# 1: name
# 2: flag
# 3: gcc-?
define SUSAN_TEST
$(eval RV32_SUSAN = $(RV32_PREFIX)-$(3)$(SUSAN_BIN))
$(eval SUSAN_BINS = $(RV32_SUSAN) $(X86_SUSAN))

test2-$(3)susan-$(1): $(SUSAN_BINS)
	cd $(SUSAN_OUT_DIR) && \
		$($(RV32_ARCH)_RUN) $(RV32_SUSAN) $(SUSAN_INPUT) rv32-$(3)output_large.$(1).pgm $(2)
	cd $(SUSAN_OUT_DIR) && \
		./$(X86_SUSAN) $(SUSAN_INPUT) x86-output_large.$(1).pgm $(2)
	cd $(SUSAN_OUT_DIR) && \
		diff rv32-$(3)output_large.$(1).pgm x86-output_large.$(1).pgm
endef

define SUSAN_TESTS
$(call SUSAN_TEST,smoothing,-s,$(1))
$(call SUSAN_TEST,edges,-e,$(1))
$(call SUSAN_TEST,corners,-c,$(1))
endef

$(eval $(call SUSAN_TESTS,))
$(eval $(call SUSAN_TESTS,gcc-))

test2-susan: $(foreach test,smoothing edges corners,test2-susan-$(test))
test2-gcc-susan: $(foreach test,smoothing edges corners,test2-gcc-susan-$(test))

###
### BLOWFISH TESTS
###

BLOWFISH_BIN           := $(BLOWFISH_NAME)
BLOWFISH_SRC_DIR       := $(MIBENCH)/$(BLOWFISH_DIR)
BLOWFISH_OUT_DIR       := $(BUILD_MIBENCH)/$(BLOWFISH_DIR)
X86_BLOWFISH_INPUT_E   := $(BLOWFISH_SRC_DIR)/input_large.asc
X86_BLOWFISH_OUTPUT_E  := $(BLOWFISH_OUT_DIR)/$(X86_PREFIX)-output_large.enc
X86_BLOWFISH_INPUT_D   := $(X86_BLOWFISH_OUTPUT_E)
X86_BLOWFISH_OUTPUT_D  := $(BLOWFISH_OUT_DIR)/$(X86_PREFIX)-output_large.asc
X86_BLOWFISH           := $(X86_PREFIX)-$(BLOWFISH_BIN)

# 1: test
# 2: flag
# 3: gcc-?
define BLOWFISH_TEST
$(eval BLOWFISH_TEST_SUFFIX = $(shell echo -n $(2) | tr a-z A-Z))

$(eval RV32_BLOWFISH = $($(RV32_ARCH)_PREFIX)-$(3)$(BLOWFISH_BIN))
$(eval BLOWFISH_BINS = $(RV32_BLOWFISH) $(X86_BLOWFISH))

$(eval RV32_BLOWFISH_INPUT_E = $(BLOWFISH_SRC_DIR)/input_large.asc)
$(eval RV32_BLOWFISH_OUTPUT_E =\
 $(BLOWFISH_OUT_DIR)/$($(RV32_ARCH)_PREFIX)-$(3)output_large.enc)
$(eval RV32_BLOWFISH_INPUT_D = $(RV32_BLOWFISH_OUTPUT_E))
$(eval RV32_BLOWFISH_OUTPUT_D =\
 $(BLOWFISH_OUT_DIR)/$($(RV32_ARCH)_PREFIX)-$(3)output_large.asc)

test2-$(3)blowfish-$(1): $(BLOWFISH_BINS)
	cd $(BLOWFISH_OUT_DIR) && \
		$($(RV32_ARCH)_RUN) $(RV32_BLOWFISH) $(2) \
		$(RV32_BLOWFISH_INPUT_$(BLOWFISH_TEST_SUFFIX)) \
		$(RV32_BLOWFISH_OUTPUT_$(BLOWFISH_TEST_SUFFIX)) \
		1234567890abcdeffedcba0987654321; \
		[ $$$$? -eq 1 ]
	cd $(BLOWFISH_OUT_DIR) && \
		./$(X86_BLOWFISH) $(2) \
		$(X86_BLOWFISH_INPUT_$(BLOWFISH_TEST_SUFFIX)) \
		$(X86_BLOWFISH_OUTPUT_$(BLOWFISH_TEST_SUFFIX)) \
		1234567890abcdeffedcba0987654321; \
		[ $$$$? -eq 1 ]
	cd $(BLOWFISH_OUT_DIR) && \
		diff \
		$(RV32_BLOWFISH_OUTPUT_$(BLOWFISH_TEST_SUFFIX)) \
		$(X86_BLOWFISH_OUTPUT_$(BLOWFISH_TEST_SUFFIX))
endef

define BLOWFISH_TESTS
$(call BLOWFISH_TEST,enc,e,$(1))
$(call BLOWFISH_TEST,dec,d,$(1))
endef

$(eval $(call BLOWFISH_TESTS,))
$(eval $(call BLOWFISH_TESTS,gcc-))

test2-blowfish: $(foreach test,enc dec,test2-blowfish-$(test))
test2-gcc-blowfish: $(foreach test,enc dec,test2-gcc-blowfish-$(test))

###
### FFT TESTS
###

# 1: arch
# 2: dir/
# 3: bin
# 4: args
# 5: out redirect: >, | tee
# 6: output file
# 7: prefix suffix (eg: gcc-)
# 8: test name (default=bin)
define TEST3
$(eval TEST3_PREFIX = $($(1)_PREFIX)-$(7))
$(eval TEST3_BASENAME = $(if $(8),$(8),$(3)))
$(eval TEST3_NAME = $(TEST3_PREFIX)$(TEST3_BASENAME))
$(eval TEST3_BIN = $(TEST3_PREFIX)$(3))
$(eval TEST3_OUT = $(TEST3_PREFIX)$(6))
$(eval TEST3_AS_ALIAS = $(shell test $(1) != $(RV32_ARCH) -a -n "$(7)" && echo -n 1))

ifeq ($(TEST3_AS_ALIAS),)
test3-$(TEST3_NAME): $(2)$(TEST3_BIN)
	$$($(1)_RUN) $(2)$(TEST3_BIN) $(4) $(5) $(2)$(TEST3_OUT)

else
test3-$(TEST3_NAME): test3-$($(1)_PREFIX)-$(TEST3_BASENAME)

endif

endef

# 1: dir/
# 2: bin
# 3: args
# 4: out redirect: >, | tee
# 5: output file
# 6: prefix suffix (eg: gcc-)
# 7: test name (default=bin)
define TEST3_ALL_ARCHS
$(foreach arch,$(ARCHS),$(eval \
$(call TEST3,$(arch),$(1),$(2),$(3),$(4),$(5),$(6),$(7))))

$(eval GCC_ = $(6))
$(eval TEST3_BASENAME = $(if $(7),$(7),$(2)))
$(eval TEST3_NAME = $(GCC_)$(TEST3_BASENAME))
$(eval TEST3_OUT = $(5))
test3-$(TEST3_NAME): $(foreach arch,$(ARCHS),test3-$($(arch)_PREFIX)-$(TEST3_NAME))
	diff $(1)rv32-$(GCC_)$(TEST3_OUT) $(1)x86-$(TEST3_OUT)

endef

# 1: dir/
# 2: bin
# 3: args
# 4: out redirect: >, | tee
# 5: output file
# 6: test name (default=bin)
define TEST3_LLVM_GCC
$(call TEST3_ALL_ARCHS,$(1),$(2),$(3),$(4),$(5),,$(6))
$(call TEST3_ALL_ARCHS,$(1),$(2),$(3),$(4),$(5),gcc-,$(6))
endef

FFT_BIN     := $(FFT_NAME)
FFT_OUT_DIR := $(BUILD_MIBENCH)/$(FFT_DIR)/

$(eval $(call TEST3_LLVM_GCC,$(FFT_OUT_DIR),$(FFT_BIN),8 32768,>,output_large.txt,fft1))
$(eval $(call TEST3_LLVM_GCC,$(FFT_OUT_DIR),$(FFT_BIN),8 32768 -i,>,output_large.inv.txt,fft2))

test2-fft: test3-fft1 test3-fft2

test2-gcc-fft: test3-gcc-fft1 test3-gcc-fft2


###
### LAME TESTS
###

LAME_BIN     := $(LAME_NAME)
LAME_OUT_DIR := $(BUILD_MIBENCH)/$(LAME_DIR)/
LAME_SRC_DIR := $(MIBENCH)/$(LAME_DIR)/

$(eval $(call TEST3_LLVM_GCC,$(LAME_OUT_DIR),$(LAME_BIN),\
$(LAME_SRC_DIR)large.wav $(LAME_OUT_DIR)output_large.mp3,\
&& mv $(LAME_OUT_DIR)output_large.mp3,output_large.mp3,))

test2-lame: test3-lame

test2-gcc-lame: test3-gcc-lame


###

clean:
	rm -rf $(BUILD_MIBENCH)


###
### UNUSED
###

###
### BLOWFISH
###

# not needed
# needed only to also compile bftest and bfspeed
ifeq (1,0)
BLOWFISH_OBJS         := bf_skey bf_ecb bf_enc bf_cbc bf_cfb64 bf_ofb64
BLOWFISH_BFTEST       := bftest
BLOWFISH_BFSPEED      := bfspeed

BF_SRC_DIR := $(MIBENCH)/$(BLOWFISH_DIR)/
BF_OUT_DIR := $(BUILD_MIBENCH)/$(BLOWFISH_DIR)/
BF_OBJS    := $(BLOWFISH_OBJS)

$(BF_OUT_DIR):
	mkdir -p $(BF_OUT_DIR)

# build BF common bcs
# 1: arch
define BF_BUILD1
$(eval BF_PREFIX = $($(1)_PREFIX))
$(eval BF_BLIB = $(BF_PREFIX)-blib)

$(call BUILD2,$(1),$(BF_SRC_DIR),$(BF_OUT_DIR),$(BF_OBJS),$(BF_BLIB))
endef

# build 1 BF bin
# 1: arch
# 2: bin
define BF_BUILD2
$(eval BF_PREFIX = $($(1)_PREFIX))
$(eval BF_BIN = $(BF_PREFIX)-$(2))
$(eval BF_BLIB = $(BF_PREFIX)-blib)

$(call BUILD2,$(1),$(BF_SRC_DIR),$(BF_OUT_DIR),$(2),$(BF_BIN)_bc1)
$(call LLLINK,$(1),$(BF_OUT_DIR),$(BF_BIN)_bc1 $(BF_BLIB),$(BF_BIN))
$(call DIS,$(1),$(BF_OUT_DIR),$(BF_BIN))
$(call BUILD3,$(1),$(BF_OUT_DIR),$(BF_OUT_DIR),$(BF_BIN),$(BF_BIN))
$(call CBUILDS,$(1),$(BF_OUT_DIR),$(BF_OUT_DIR),$(BF_BIN),$(BF_BIN))
endef

define BF_BUILD_ALL_TESTS
$(eval BF_BINS = $(BLOWFISH_BF) $(BLOWFISH_BFTEST) $(BLOWFISH_BFSPEED))

$(call BF_BUILD1,$(1))
$(foreach bin,$(BF_BINS),$(call BF_BUILD2,$(1),$(bin)))

$(eval BF_PREFIX = $($(1)_PREFIX))

$(BF_PREFIX)-$(BLOWFISH_BIN): $(BF_OUT_DIR) \
                              $(addprefix $(BF_OUT_DIR)$(BF_PREFIX)-,$(BF_BINS))
endef

define BF_BUILD
$(eval BF_PREFIX = $($(1)_PREFIX))
$(eval BF_BIN = $(BF_PREFIX)-$(BLOWFISH_BF))

$(call BUILD,$(1),$(BF_SRC_DIR),$(BF_OUT_DIR),$(BLOWFISH_BF) $(BF_OBJS),$(BF_BIN))

$(BF_PREFIX)-$(BLOWFISH_BIN): $(BF_OUT_DIR) $(addprefix $(BF_OUT_DIR),$(BF_BIN))
endef

define BF_BUILD_GCC
$(eval BF_PREFIX = $($(1)_PREFIX))
$(eval BF_BIN = $(BF_PREFIX)-$(BLOWFISH_BF))

$(call GCC_BUILD,$(1),$(BF_SRC_DIR),$(BF_OUT_DIR),$(BLOWFISH_BF) $(BF_OBJS),$(BF_BIN))

$(BF_PREFIX)-$(BLOWFISH_BIN): $(BF_OUT_DIR) $(addprefix $(BF_OUT_DIR),$(BF_BIN))
endef


$(foreach arch,$(ARCHS),$(eval $(call BF_BUILD,$(arch))))

$(BLOWFISH_BIN): $(foreach arch,$(ARCHS),$($(arch)_PREFIX)-$(BLOWFISH_BIN))

clean-$(BLOWFISH_BIN):
	rm -rf $(BF_OUT_DIR)
endif

