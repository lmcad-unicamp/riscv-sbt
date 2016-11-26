DIR_ROOT := $(PWD)
DIR_TOOLCHAIN := $(DIR_ROOT)/toolchain

all: riscv-gnu-toolchain

riscv-gnu-toolchain/Makefile:
	cd riscv-gnu-toolchain && \
	./configure --prefix=$(DIR_TOOLCHAIN) --enable-multilib

.PHONY: riscv-gnu-toolchain
riscv-gnu-toolchain: riscv-gnu-toolchain/Makefile
	$(MAKE) -C riscv-gnu-toolchain -j9

riscv-gnu-toolchain-clean:
	$(MAKE) -C riscv-gnu-toolchain distclean
	rm -f riscv-gnu-toolchain/Makefile

clean: riscv-gnu-toolchain-clean
	rm -rf $(DIR_TOOLCHAIN)
