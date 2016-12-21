DIR_ROOT := $(PWD)
DIR_TOOLCHAIN := $(DIR_ROOT)/toolchain

all: llvm riscv-isa-sim riscv-pk

###
### riscv-gnu-toolchain ###
###

GCC_BUILD := riscv-gnu-toolchain/build
GCC_MAKEFILE := $(GCC_BUILD)/Makefile
GCC_OUT := $(GCC_BUILD)/build-gcc-newlib/gcc/xgcc
GCC_TOOLCHAIN := $(DIR_TOOLCHAIN)/bin/riscv64-unknown-elf-gcc

$(GCC_MAKEFILE):
	mkdir -p $(GCC_BUILD)
	cd $(GCC_BUILD) && \
	../configure --prefix=$(DIR_TOOLCHAIN) --enable-multilib

$(GCC_OUT) $(GCC_TOOLCHAIN): $(GCC_MAKEFILE)
	# this builds AND installs the riscv-gnu-toolchain
	$(MAKE) -C $(GCC_BUILD) -j9
	touch $@

.PHONY: riscv-gnu-toolchain
riscv-gnu-toolchain: $(GCC_TOOLCHAIN)

riscv-gnu-toolchain-clean:
	rm -rf $(GCC_BUILD)

###
### riscv-fesvr ####
###

FESVR_BUILD := riscv-fesvr/build
FESVR_MAKEFILE := $(FESVR_BUILD)/Makefile
FESVR_OUT := $(FESVR_BUILD)/libfesvr.so
FESVR_TOOLCHAIN := $(DIR_TOOLCHAIN)/lib/libfesvr.so

$(FESVR_MAKEFILE):
	mkdir -p $(FESVR_BUILD)
	cd $(FESVR_BUILD) && \
	../configure --prefix=$(DIR_TOOLCHAIN)

$(FESVR_OUT): $(FESVR_MAKEFILE)
	$(MAKE) -C $(FESVR_BUILD) -j9

$(FESVR_TOOLCHAIN): $(FESVR_OUT)
	$(MAKE) -C $(FESVR_BUILD) install

.PHONY: riscv-fesvr
riscv-fesvr: $(FESVR_TOOLCHAIN)

riscv-fesvr-clean:
	rm -rf $(FESVR_BUILD)

###
### riscv-isa-sim ###
###

SIM_BUILD := riscv-isa-sim/build
SIM_MAKEFILE := $(SIM_BUILD)/Makefile
SIM_OUT := $(SIM_BUILD)/spike
SIM_TOOLCHAIN := $(DIR_TOOLCHAIN)/bin/spike

$(SIM_MAKEFILE): $(FESVR_TOOLCHAIN)
	mkdir -p $(SIM_BUILD)
	cd $(SIM_BUILD) && \
	../configure --prefix=$(DIR_TOOLCHAIN) --with-fesvr=$(DIR_TOOLCHAIN)

$(SIM_OUT): $(SIM_MAKEFILE)
	$(MAKE) -C $(SIM_BUILD) -j9
	touch $@

$(SIM_TOOLCHAIN): $(SIM_OUT)
	$(MAKE) -C $(SIM_BUILD) install

.PHONY: riscv-isa-sim
riscv-isa-sim: $(SIM_TOOLCHAIN)

riscv-isa-sim-clean:
	rm -rf $(SIM_BUILD)

###
### riscv-pk ###
###

PK_HOST := riscv64-unknown-elf
PK_BUILD := riscv-pk/build

PK32_BUILD := $(PK_BUILD)/32
PK32_MAKEFILE := $(PK32_BUILD)/Makefile
PK32_OUT := $(PK32_BUILD)/pk
PK32_TOOLCHAIN := $(DIR_TOOLCHAIN)/riscv32-unknown-elf/bin/pk

$(PK32_MAKEFILE):
	mkdir -p $(PK32_BUILD)
	cd $(PK32_BUILD) && \
	../../configure --prefix=$(DIR_TOOLCHAIN) --host=$(PK_HOST) --enable-32bit

$(PK32_OUT): $(PK32_MAKEFILE)
	$(MAKE) -C $(PK32_BUILD)
	touch $@

$(PK32_TOOLCHAIN): $(PK32_OUT)
	$(MAKE) -C $(PK32_BUILD) install

PK64_BUILD := $(PK_BUILD)/64
PK64_MAKEFILE := $(PK64_BUILD)/Makefile
PK64_OUT := $(PK64_BUILD)/pk
PK64_TOOLCHAIN := $(DIR_TOOLCHAIN)/riscv64-unknown-elf/bin/pk

$(PK64_MAKEFILE):
	mkdir -p $(PK64_BUILD)
	cd $(PK64_BUILD) && \
	../../configure --prefix=$(DIR_TOOLCHAIN) --host=$(PK_HOST)

$(PK64_OUT): $(PK64_MAKEFILE)
	$(MAKE) -C $(PK64_BUILD)
	touch $@

$(PK64_TOOLCHAIN): $(PK64_OUT)
	$(MAKE) -C $(PK64_BUILD) install

.PHONY: riscv-pk
riscv-pk: $(PK32_TOOLCHAIN) $(PK64_TOOLCHAIN)

riscv-pk-clean:
	rm -rf $(PK32_BUILD) $(PK64_BUILD)

###
### cmake
###

CMAKE_URL := http://www.cmake.org/files/v3.5/cmake-3.5.2-Linux-x86_64.tar.gz
CMAKE_ROOT := cmake
CMAKE_PKG := $(CMAKE_ROOT)/cmake-3.5.2-Linux-x86_64.tar.gz
CMAKE := $(PWD)/$(CMAKE_ROOT)/bin/cmake

$(CMAKE_PKG):
	mkdir -p $(CMAKE_ROOT)
	cd $(CMAKE_ROOT) && \
	wget $(CMAKE_URL)

$(CMAKE): $(CMAKE_PKG)
	tar --strip-components=1 -xvf $(CMAKE_PKG) -C $(CMAKE_ROOT)
	touch $(CMAKE)

.PHONY:
cmake: $(CMAKE)

###
### llvm ###
###

LLVM_BUILD := llvm/build
LLVM_MAKEFILE := $(LLVM_BUILD)/Makefile
LLVM_OUT := $(LLVM_BUILD)/bin/llc
LLVM_TOOLCHAIN := $(DIR_TOOLCHAIN)/bin/llc

$(LLVM_MAKEFILE): $(GCC_TOOLCHAIN) $(CMAKE)
	mkdir -p $(LLVM_BUILD)
	cd $(LLVM_BUILD) && \
	$(CMAKE) -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD="ARM;RISCV;X86" -DCMAKE_INSTALL_PREFIX=$(DIR_TOOLCHAIN) ../
	touch $@

$(LLVM_OUT): $(LLVM_MAKEFILE)
	$(MAKE) -C $(LLVM_BUILD) #-j9 #| tee $(LLVM_BUILD)/log.txt
	touch $@

$(LLVM_TOOLCHAIN): $(LLVM_OUT)
	$(MAKE) -C $(LLVM_BUILD) install
	touch $@

.PHONY: llvm
llvm: $(LLVM_TOOLCHAIN)

llvm-clean:
	rm -rf $(LLVM_BUILD)

###

clean: \
	riscv-gnu-toolchain-clean \
	riscv-fesvr-clean \
	riscv-isa-sim-clean \
	riscv-pk-clean \
	llvm-clean
	rm -rf $(DIR_TOOLCHAIN)
