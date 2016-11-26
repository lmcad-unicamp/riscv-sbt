DIR_ROOT := $(PWD)
DIR_TOOLCHAIN := $(DIR_ROOT)/toolchain

all: riscv-gnu-toolchain riscv-isa-sim riscv-pk

### riscv-gnu-toolchain ###

riscv-gnu-toolchain/Makefile:
	cd riscv-gnu-toolchain && \
	./configure --prefix=$(DIR_TOOLCHAIN) --enable-multilib

.PHONY: riscv-gnu-toolchain
riscv-gnu-toolchain: riscv-gnu-toolchain/Makefile
	$(MAKE) -C riscv-gnu-toolchain -j9

riscv-gnu-toolchain-clean:
	$(MAKE) -C riscv-gnu-toolchain distclean
	rm -f riscv-gnu-toolchain/Makefile

### riscv-fesvr ####

riscv-fesvr/Makefile:
	cd riscv-fesvr && \
	./configure --prefix=$(DIR_TOOLCHAIN)

.PHONY: riscv-fesvr
riscv-fesvr $(DIR_TOOLCHAIN)/lib/libfesvr.so: riscv-fesvr/Makefile
	$(MAKE) -C riscv-fesvr -j9
	$(MAKE) -C riscv-fesvr install

riscv-fesvr-clean:
	$(MAKE) -C riscv-fesvr distclean
	rm -f riscv-fesvr/Makefile

### riscv-isa-sim ###

riscv-isa-sim/Makefile: $(DIR_TOOLCHAIN)/lib/libfesvr.so
	cd riscv-isa-sim && \
	./configure --prefix=$(DIR_TOOLCHAIN) --with-fesvr=$(DIR_TOOLCHAIN)

.PHONY: riscv-isa-sim
riscv-isa-sim: riscv-isa-sim/Makefile
	$(MAKE) -C riscv-isa-sim -j9
	$(MAKE) -C riscv-isa-sim install

riscv-isa-sim-clean:
	$(MAKE) -C riscv-isa-sim distclean
	rm -f riscv-isa-sim/Makefile

### riscv-pk ###

riscv-pk/build/Makefile riscv-pk/build64/Makefile:
	# 32 bit
	mkdir -p riscv-pk/build
	cd riscv-pk/build && \
	../configure --prefix=$(DIR_TOOLCHAIN) --host=riscv64-unknown-elf --enable-32bit
	# 64 bit
	mkdir -p riscv-pk/build64
	cd riscv-pk/build64 && \
	../configure --prefix=$(DIR_TOOLCHAIN) --host=riscv64-unknown-elf

.PHONY: riscv-pk
riscv-pk: riscv-pk/build/Makefile riscv-pk/build64/Makefile
	$(MAKE) -C riscv-pk/build
	$(MAKE) -C riscv-pk/build install
	$(MAKE) -C riscv-pk/build64
	$(MAKE) -C riscv-pk/build64 install

riscv-pk-clean:
	rm -rf riscv-pk/build riscv-pk/build64

###

clean: riscv-gnu-toolchain-clean riscv-fesvr-clean riscv-isa-sim-clean riscv-pk-clean
	rm -rf $(DIR_TOOLCHAIN)
