# Mibench source
# http://vhosts.eecs.umich.edu/mibench//automotive.tar.gz
# http://vhosts.eecs.umich.edu/mibench//network.tar.gz
# http://vhosts.eecs.umich.edu/mibench//security.tar.gz
# http://vhosts.eecs.umich.edu/mibench//telecomm.tar.gz
# http://vhosts.eecs.umich.edu/mibench//office.tar.gz
# http://vhosts.eecs.umich.edu/mibench//consumer.tar.gz
#
# 01- automotive/basicmath/basicmath_large
# - need FP: yes
# - multiple files
#
# 02- automotive/bitcount/bitcnts
# - need FP: yes
# - multiple files
#
# 03- automotive/susan/susan
# - need FP: yes
# - single file
# - 2122 lines
#
# 04- network/patricia/patricia
# - need FP: yes
# - multiple files
#
# 05- network/dijkstra/dijkstra_large
# - need FP: no
# - single file
# - 174 lines
#
# 06- security/rijndael/rijndael
# - need FP: no
# - multiple files
# - 827
#
# 07- security/blowfish/bf
# - huge
#
# 08- security/sha/sha
# - need FP: no
# - multiple files
# - 269 lines
#
# 09- telecomm/adpcm/rawcaudio
# - need FP: no
# - multiple files
#
# 10- telecomm/adpcm/rawdaudio
# - need FP: no
# - multiple files
#
# 11- telecomm/CRC32/crc
# - need FP: no
# - single file
# - 281 lines
#
# 12- telecomm/FFT/fft
# - need FP: yes
# - multiple files
# - 469 lines
#
# 13- office/stringsearch/search_large
# - need FP: no
# - multiple files
# - lines: 3216
#
# 14- consumer/lame/lame3.70/lame
# - huge
#
# can run now: 5, 11 (2)
# need multiple files only: 6, 8, 9, 10, 13 (5)
# need FP only: 3 (1)
# need multiple files and FP: 1, 2, 4, 12 (4)
# huge: 7, 14 (2)

ifeq ($(TOPDIR),)
$(error "TOPDIR not set. Please run 'source scripts/setenv.sh' first.")
endif

###
### CONFIG
###

include $(TOPDIR)/make/config.mk


MIBENCH         := $(TOPDIR)/mibench
BUILD_MIBENCH   := $(BUILD_DIR)/mibench

RV32_ARCH       := RV32_LINUX
ARCHS           := $(RV32_ARCH) X86
BENCH_TRANSLATE  =
BENCH_GCC_BUILD := 1

## 01- BASICMATH
# wrong results

BASICMATH_BIN   := basicmath
BASICMATH_DIR   := automotive/basicmath
BASICMATH_MODS  := basicmath_large rad2deg cubic isqrt
BASICMATH_ARGS  :=

## 02- BITCOUNT
# wrong results

BITCOUNT_BIN    := bitcount
BITCOUNT_DIR    := automotive/bitcount
BITCOUNT_MODS   := bitcnt_1 bitcnt_2 bitcnt_3 bitcnt_4 \
                   bitcnts bitfiles bitstrng bstr_i
BITCOUNT_ARGS   := 1125000 | sed 's/Time:[^;]*; //;/^Best/d;/^Worst/d'

## 03- SUSAN
# smoothing: OK
# edges: wrong results
# corners: segfault

SUSAN_BIN       := susan
SUSAN_DIR       := automotive/susan
SUSAN_MODS      := susan
SUSAN_ARGS      := notests

## 04- PATRICIA
# wrong results

PATRICIA_BIN    := patricia
PATRICIA_DIR    := network/patricia
PATRICIA_MODS   := patricia patricia_test
PATRICIA_ARGS   := $(MIBENCH)/$(PATRICIA_DIR)/large.udp

## 05- DIJKSTRA
# OK

DIJKSTRA_BIN    := dijkstra
DIJKSTRA_DIR    := network/dijkstra
DIJKSTRA_MODS   := dijkstra_large
DIJKSTRA_ARGS   := $(MIBENCH)/$(DIJKSTRA_DIR)/input.dat

## 06- RIJNDAEL
# OK

RIJNDAEL_BIN    := rijndael
RIJNDAEL_DIR    := security/rijndael
RIJNDAEL_MODS   := aes aesxam
RIJNDAEL_ARGS   := notests

## 07- BLOWFISH
# wrong results

BLOWFISH_BIN          := blowfish
BLOWFISH_DIR          := security/blowfish
BLOWFISH_OBJS         := bf_skey bf_ecb bf_enc bf_cbc bf_cfb64 bf_ofb64
BLOWFISH_BF           := bf
BLOWFISH_BFTEST       := bftest
BLOWFISH_BFSPEED      := bfspeed
BLOWFISH_MODS         :=
BLOWFISH_ARGS         := notests

## 11- CRC32
# OK

CRC32_BIN       := crc32
CRC32_DIR       := telecomm/CRC32
CRC32_MODS      := crc_32
CRC32_ARGS      := $(MIBENCH)/telecomm/adpcm/data/large.pcm

##

BENCHS          := BASICMATH BITCOUNT SUSAN PATRICIA RIJNDAEL BLOWFISH \
                   DIJKSTRA CRC32

###
### RULES
###

all: $(foreach bench,$(BENCHS),$($(bench)_BIN))

include $(TOPDIR)/make/rules.mk

# 1: arch
# 2: dir/
# 3: bin
# 4: args
# 5: prefix suffix (eg: gcc-)
define TEST2
$(eval TEST2_BIN = $$($(1)_PREFIX)-$(5)$(3))
test2-$(TEST2_BIN): $(2)$(TEST2_BIN)
	$$($(1)_RUN) $(2)$(TEST2_BIN) $(4) | tee $(2)$(TEST2_BIN).out
endef


# 1: bench name
define BENCH
$(eval BENCH_BIN = $($(1)_BIN))
$(eval BENCH_SRC_DIR = $(MIBENCH)/$($(1)_DIR)/)
$(eval BENCH_OUT_DIR = $(BUILD_MIBENCH)/$($(1)_DIR)/)
$(eval BENCH_MODS = $($(1)_MODS))
$(eval BENCH_ARGS = $($(1)_ARGS))

$(call BUILD,X86,$(BENCH_SRC_DIR),$(BENCH_OUT_DIR),$(BENCH_MODS),x86-$(BENCH_BIN))

$(if $(BENCH_GCC_BUILD),\
$(call GCC_BUILD,$(RV32_ARCH),$(BENCH_SRC_DIR),$(BENCH_OUT_DIR),$(BENCH_MODS),rv32-gcc-$(BENCH_BIN)))

$(call BUILD,$(RV32_ARCH),$(BENCH_SRC_DIR),$(BENCH_OUT_DIR),$(BENCH_MODS),rv32-$(BENCH_BIN))

ifeq ($(BENCH_TRANSLATE),1)
$(eval BENCH_$(RV32_ARCH)_CLANG_FLAGS = $($(RV32_ARCH)_CLANG_FLAGS))
$(eval $(RV32_ARCH)_CLANG_FLAGS = )
$(call BUILDO,$(RV32_ARCH),$(BENCH_SRC_DIR),$(BENCH_OUT_DIR),$(BENCH_MODS),rv32_x86-$(BENCH_BIN))
$(eval $(RV32_ARCH)_CLANG_FLAGS = $(BENCH_$(RV32_ARCH)_CLANG_FLAGS))
$(call TRANSLATE,X86,$(BENCH_SRC_DIR),$(BENCH_OUT_DIR),rv32_x86-$(BENCH_BIN),rv32-x86-$(BENCH_BIN))
endif

$(BENCH_OUT_DIR):
	mkdir -p $(BENCH_OUT_DIR)

$(BENCH_BIN): $(BENCH_OUT_DIR) \
    rv32-$(BENCH_BIN) x86-$(BENCH_BIN) \
    $(if $(BENCH_GCC_BUILD),rv32-gcc-$(BENCH_BIN),) \
    $(if $(BENCH_TRANSLATE),rv32-x86-$(BENCH_BIN),)

clean-$(BENCH_BIN):
	rm -rf $(BENCH_OUT_DIR)

ifneq ($(BENCH_ARGS),notests)
test-$(BENCH_BIN):
	$(SCRIPTS_DIR)/measure.py $(BENCH_OUT_DIR) $(BENCH_BIN) $(BENCH_ARGS)

$(foreach arch,$(ARCHS),$(call TEST2,$(arch),$(BENCH_OUT_DIR),$(BENCH_BIN),$(BENCH_ARGS),))
test2-$(BENCH_BIN): $(BENCH_OUT_DIR) \
		$(foreach arch,$(ARCHS),test2-$$($(arch)_PREFIX)-$(BENCH_BIN))
	diff $(BENCH_OUT_DIR)rv32-$(BENCH_BIN).out $(BENCH_OUT_DIR)x86-$(BENCH_BIN).out

$(if $(BENCH_GCC_BUILD),\
$(call TEST2,$(RV32_ARCH),$(BENCH_OUT_DIR),$(BENCH_BIN),$(BENCH_ARGS),gcc-))

ifeq ($(BENCH_GCC_BUILD),1)
test2-gcc-$(BENCH_BIN): $(BENCH_OUT_DIR) \
		$(foreach pref,rv32-gcc x86,test2-$(pref)-$(BENCH_BIN))
	diff $(BENCH_OUT_DIR)rv32-gcc-$(BENCH_BIN).out $(BENCH_OUT_DIR)x86-$(BENCH_BIN).out
endif

endif
endef


###
### GEN RULES
###

$(foreach bench,$(filter-out BLOWFISH,$(BENCHS)),$(eval $(call BENCH,$(bench))))

###
### BLOWFISH
###

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

###
### SUSAN TESTS
###

SUSAN_SRC_DIR := $(MIBENCH)/$(SUSAN_DIR)
SUSAN_OUT_DIR := $(BUILD_MIBENCH)/$(SUSAN_DIR)
SUSAN_INPUT   := $(SUSAN_SRC_DIR)/input_large.pgm
RV32_SUSAN    := $(RV32_PREFIX)-$(SUSAN_BIN)
X86_SUSAN     := $(X86_PREFIX)-$(SUSAN_BIN)
SUSAN_BINS    := $(foreach bin,$(RV32_SUSAN) $(X86_SUSAN),$(SUSAN_OUT_DIR)/$(bin))

# 1: name
# 2: flag
define SUSAN_TEST
test2-susan-$(1): $(SUSAN_BINS)
	cd $(SUSAN_OUT_DIR) && \
		$($(RV32_ARCH)_RUN) $(RV32_SUSAN) $(SUSAN_INPUT) rv32-output_large.$(1).pgm $(2)
	cd $(SUSAN_OUT_DIR) && \
		./$(X86_SUSAN) $(SUSAN_INPUT) x86-output_large.$(1).pgm $(2)
	cd $(SUSAN_OUT_DIR) && \
		diff rv32-output_large.$(1).pgm x86-output_large.$(1).pgm
endef

$(eval $(call SUSAN_TEST,smoothing,-s))
$(eval $(call SUSAN_TEST,edges,-e))
$(eval $(call SUSAN_TEST,corners,-c))

test2-susan: $(foreach test,smoothing edges corners,test2-susan-$(test))

###
### RIJNDAEL TESTS
###

RIJNDAEL_SRC_DIR       := $(MIBENCH)/$(RIJNDAEL_DIR)
RIJNDAEL_OUT_DIR       := $(BUILD_MIBENCH)/$(RIJNDAEL_DIR)
RV32_RIJNDAEL_INPUT_E  := $(RIJNDAEL_SRC_DIR)/input_large.asc
RV32_RIJNDAEL_OUTPUT_E := $(RIJNDAEL_OUT_DIR)/$($(RV32_ARCH)_PREFIX)-output_large.enc
RV32_RIJNDAEL_INPUT_D  := $(RV32_RIJNDAEL_OUTPUT_E)
RV32_RIJNDAEL_OUTPUT_D := $(RIJNDAEL_OUT_DIR)/$($(RV32_ARCH)_PREFIX)-output_large.dec
RV32_RIJNDAEL          := $($(RV32_ARCH)_PREFIX)-$(RIJNDAEL_BIN)
X86_RIJNDAEL_INPUT_E   := $(RIJNDAEL_SRC_DIR)/input_large.asc
X86_RIJNDAEL_OUTPUT_E  := $(RIJNDAEL_OUT_DIR)/$(X86_PREFIX)-output_large.enc
X86_RIJNDAEL_INPUT_D   := $(X86_RIJNDAEL_OUTPUT_E)
X86_RIJNDAEL_OUTPUT_D  := $(RIJNDAEL_OUT_DIR)/$(X86_PREFIX)-output_large.dec
X86_RIJNDAEL           := $(X86_PREFIX)-$(RIJNDAEL_BIN)
RIJNDAEL_BINS          := $(foreach bin,$(RV32_RIJNDAEL) $(X86_RIJNDAEL),$(RIJNDAEL_OUT_DIR)/$(bin))

define RIJNDAEL_TEST
$(eval RIJNDAEL_TEST_SUFFIX = $(shell echo -n $(2) | tr a-z A-Z))
test2-rijndael-$(1): $(RIJNDAEL_BINS)
	cd $(RIJNDAEL_OUT_DIR) && \
		$($(RV32_ARCH)_RUN) $(RV32_RIJNDAEL) \
		$(RV32_RIJNDAEL_INPUT_$(RIJNDAEL_TEST_SUFFIX)) \
		$(RV32_RIJNDAEL_OUTPUT_$(RIJNDAEL_TEST_SUFFIX)) \
		$(2) 1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321
	cd $(RIJNDAEL_OUT_DIR) && \
		./$(X86_RIJNDAEL) \
		$(X86_RIJNDAEL_INPUT_$(RIJNDAEL_TEST_SUFFIX)) \
		$(X86_RIJNDAEL_OUTPUT_$(RIJNDAEL_TEST_SUFFIX)) \
		$(2) 1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321
	cd $(RIJNDAEL_OUT_DIR) && \
		diff \
		$(RV32_RIJNDAEL_OUTPUT_$(RIJNDAEL_TEST_SUFFIX)) \
		$(X86_RIJNDAEL_OUTPUT_$(RIJNDAEL_TEST_SUFFIX))
endef

$(eval $(call RIJNDAEL_TEST,enc,e))
$(eval $(call RIJNDAEL_TEST,dec,d))

test2-rijndael: $(foreach test,enc dec,test2-rijndael-$(test))

###
### BLOWFISH TESTS
###

BLOWFISH_SRC_DIR       := $(MIBENCH)/$(BLOWFISH_DIR)
BLOWFISH_OUT_DIR       := $(BUILD_MIBENCH)/$(BLOWFISH_DIR)
RV32_BLOWFISH_INPUT_E  := $(BLOWFISH_SRC_DIR)/input_large.asc
RV32_BLOWFISH_OUTPUT_E := $(BLOWFISH_OUT_DIR)/$($(RV32_ARCH)_PREFIX)-output_large.enc
RV32_BLOWFISH_INPUT_D  := $(RV32_BLOWFISH_OUTPUT_E)
RV32_BLOWFISH_OUTPUT_D := $(BLOWFISH_OUT_DIR)/$($(RV32_ARCH)_PREFIX)-output_large.asc
RV32_BLOWFISH          := $($(RV32_ARCH)_PREFIX)-$(BLOWFISH_BF)
X86_BLOWFISH_INPUT_E   := $(BLOWFISH_SRC_DIR)/input_large.asc
X86_BLOWFISH_OUTPUT_E  := $(BLOWFISH_OUT_DIR)/$(X86_PREFIX)-output_large.enc
X86_BLOWFISH_INPUT_D   := $(X86_BLOWFISH_OUTPUT_E)
X86_BLOWFISH_OUTPUT_D  := $(BLOWFISH_OUT_DIR)/$(X86_PREFIX)-output_large.asc
X86_BLOWFISH           := $(X86_PREFIX)-$(BLOWFISH_BF)
BLOWFISH_BINS          := $(RV32_BLOWFISH) $(X86_BLOWFISH)

define BLOWFISH_TEST
$(eval BLOWFISH_TEST_SUFFIX = $(shell echo -n $(2) | tr a-z A-Z))
test2-blowfish-$(1): $(BLOWFISH_BINS)
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

$(eval $(call BLOWFISH_TEST,enc,e))
$(eval $(call BLOWFISH_TEST,dec,d))

test2-blowfish: $(foreach test,enc dec,test2-blowfish-$(test))

###

clean:
	rm -rf $(BUILD_MIBENCH)

