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

all: dijkstra_large

include $(TOPDIR)/make/rules.mk


# 1: arch
# 2: src dir
# 3: out dir
# 4: mods
# 5: bin
define BENCH
$(eval $(foreach mod,$(4),\
    $(call C2S,$(1),$(2),$(3),$(mod))))

$(eval $(foreach mod,$(4),\
    $(call S2O,$(1),$(3),$(3),$(mod))))

$(eval $(call CLINK2,$(1),$(3),$(addsuffix .o,$(4)),$(5)))

$($(1)_PREFIX)-$(5): $(3) $(3)$($(1)_PREFIX)-$(5)
endef

# 1: archs
# 2: out dir
# 3: bin
define BENCH2
$(2):
	mkdir -p $$@

$(3)-clean:
	rm -f $(2)*
endef

###
###


MIBENCH := $(TOPDIR)/mibench
BUILD_MIBENCH := $(BUILD_DIR)/mibench
CFLAGS = -Wall -O3

##
## 05- DIJKSTRA
##

DIJKSTRA_DIR := network/dijkstra
DIJKSTRA_SRC_DIR := $(MIBENCH)/$(DIJKSTRA_DIR)/
DIJKSTRA_OUT_DIR := $(BUILD_MIBENCH)/$(DIJKSTRA_DIR)/

DIJKSTRA_MODS := dijkstra_large
DIJKSTRA_BIN := dijkstra_large

$(eval $(call BENCH,RV32,$(DIJKSTRA_SRC_DIR),$(DIJKSTRA_OUT_DIR),$(DIJKSTRA_MODS),$(DIJKSTRA_BIN)))
$(eval $(call BENCH,X86,$(DIJKSTRA_SRC_DIR),$(DIJKSTRA_OUT_DIR),$(DIJKSTRA_MODS),$(DIJKSTRA_BIN)))
$(eval $(call BENCH2,RV32 X86,$(DIJKSTRA_OUT_DIR),$(DIJKSTRA_BIN)))

$(eval $(call TRANSLATE2,RV32,X86,$(DIJKSTRA_OUT_DIR),$(DIJKSTRA_MODS),$(DIJKSTRA_BIN)))

$(DIJKSTRA_BIN): $(foreach arch,RV32 X86,$($(arch)_PREFIX)-$(DIJKSTRA_BIN)) \
    $(foreach arch,X86,rv32-$($(arch)_PREFIX)-$(DIJKSTRA_BIN))

##


#$(eval $(call BUILD,X86,x86-mm,CLANG,mm,save-run.out))
#$(eval $(call BUILD,RV32,rv32-mm,CLANG,mm,save-run.out))
#$(eval $(call TRANSLATE_C,RV32,X86,mm))

