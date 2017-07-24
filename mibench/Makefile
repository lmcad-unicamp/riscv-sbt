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

include $(TOPDIR)/make/config.mk

DIJKSTRA_BIN := dijkstra_large

all: $(DIJKSTRA_BIN)

include $(TOPDIR)/make/rules.mk

MIBENCH       := $(TOPDIR)/mibench
BUILD_MIBENCH := $(BUILD_DIR)/mibench


# 1: src dir/
# 2: out dir/
# 3: mods
# 4: bin
define BENCH
$(call BUILD,X86,$(1),$(2),$(3),x86-$(4))

$(call BUILDO,RV32,$(1),$(2),$(3),rv32-$(4))
$(call TRANSLATE,X86,$(1),$(2),rv32-$(4),rv32-x86-$(4))

$(2):
	mkdir -p $(2)

$(4): $(2) x86-$(4) rv32-x86-$(4)
endef


# 1: out dir/
# 2: test
# 3: args
define MITEST
test-$(2):
	$(SCRIPTS_DIR)/measure.py $(1) $(2) $(3)
endef


##
## 05- DIJKSTRA
##

DIJKSTRA_DIR := network/dijkstra
DIJKSTRA_SRC_DIR := $(MIBENCH)/$(DIJKSTRA_DIR)/
DIJKSTRA_OUT_DIR := $(BUILD_MIBENCH)/$(DIJKSTRA_DIR)/

DIJKSTRA_MODS := dijkstra_large

RV32_CLANG_FLAGS =

$(eval \
$(call BENCH,$(DIJKSTRA_SRC_DIR),$(DIJKSTRA_OUT_DIR),$(DIJKSTRA_MODS),$(DIJKSTRA_BIN)))
$(eval \
$(call MITEST,$(DIJKSTRA_OUT_DIR),$(DIJKSTRA_BIN),$(DIJKSTRA_SRC_DIR)input.dat))

###

clean:
	rm -rf $(BUILD_MIBENCH)
