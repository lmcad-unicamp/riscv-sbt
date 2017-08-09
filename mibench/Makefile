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


MIBENCH       := $(TOPDIR)/mibench
BUILD_MIBENCH := $(BUILD_DIR)/mibench

BENCH_TRANSLATE = 1

## 05- DIJKSTRA

DIJKSTRA_BIN    := dijkstra
DIJKSTRA_DIR    := network/dijkstra
DIJKSTRA_MODS   := dijkstra_large
DIJKSTRA_ARGS   := $(MIBENCH)/$(DIJKSTRA_DIR)/input.dat

## 11- CRC32

CRC32_BIN       := crc
CRC32_DIR       := telecomm/CRC32
CRC32_MODS      := crc_32
CRC32_ARGS      := $(MIBENCH)/telecomm/adpcm/data/large.pcm

##

BENCHS          := DIJKSTRA CRC32

###
### RULES
###

all: $(foreach bench,$(BENCHS),$($(bench)_BIN))

include $(TOPDIR)/make/rules.mk

# 1: arch
# 2: dir/
# 3: bin
# 4: args
define TEST2
$(eval TEST2_BIN = $$($(1)_PREFIX)-$(3))
test2-$(TEST2_BIN):
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

$(call BUILD,RV32,$(BENCH_SRC_DIR),$(BENCH_OUT_DIR),$(BENCH_MODS),rv32-$(BENCH_BIN))

ifeq ($(BENCH_TRANSLATE),1)
$(eval BENCH_RV32_CLANG_FLAGS = $(RV32_CLANG_FLAGS))
$(eval RV32_CLANG_FLAGS = )
$(call BUILDO,RV32,$(BENCH_SRC_DIR),$(BENCH_OUT_DIR),$(BENCH_MODS),rv32_x86-$(BENCH_BIN))
$(eval RV32_CLANG_FLAGS = $(BENCH_RV32_CLANG_FLAGS))
$(call TRANSLATE,X86,$(BENCH_SRC_DIR),$(BENCH_OUT_DIR),rv32_x86-$(BENCH_BIN),rv32-x86-$(BENCH_BIN))
endif

$(BENCH_OUT_DIR):
	mkdir -p $(BENCH_OUT_DIR)

$(BENCH_BIN): $(BENCH_OUT_DIR) rv32-$(BENCH_BIN) x86-$(BENCH_BIN) \
    $(if $(BENCH_TRANSLATE),rv32-x86-$(BENCH_BIN),)

clean-$(BENCH_BIN):
	rm -rf $(BENCH_OUT_DIR)

test-$(BENCH_BIN):
	$(SCRIPTS_DIR)/measure.py $(BENCH_OUT_DIR) $(BENCH_BIN) $(BENCH_ARGS)

$(foreach arch,RV32 X86,$(call TEST2,$(arch),$(BENCH_OUT_DIR),$(BENCH_BIN),$(BENCH_ARGS)))
endef


###
### GEN RULES
###

$(foreach bench,$(BENCHS),$(eval $(call BENCH,$(bench))))

###

clean:
	rm -rf $(BUILD_MIBENCH)

