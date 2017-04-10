#!/bin/bash

# TOPDIR
export TOPDIR="$PWD"

# TC
TC="$TOPDIR/toolchain"

# toolchain: x86
echo "$PATH" | grep "$TC/x86/bin" >/dev/null ||
export PATH="$TC/x86/bin:$PATH"

# toolchain: risc-v release
echo "$PATH" | grep "$TC/release/bin" >/dev/null ||
export PATH="$TC/release/bin:$PATH"

# toolchain: risc-v debug
echo "$PATH" | grep "$TC/debug/bin" >/dev/null ||
export PATH="$TC/debug/bin:$PATH"

if [ $# -eq 1 -a "$1" == "release" ]; then
  export BUILD_TYPE=Release
else
  export BUILD_TYPE=Debug
fi

export TCR=$TC/release
export PK32=$TCR/riscv32-unknown-elf/bin/pk
export PK64=$TCR/riscv64-unknown-elf/bin/pk
export BBL=$TCR/riscv64-unknown-elf/bin/bbl
export ROOT_FS=$TCR/share/riscvemu/root.bin
alias elf="$TOPDIR/sbt/test/elf"
alias spike32="spike $PK32"
alias spike64="spike --isa=RV64IMAFDC $PK64"
alias linux_spike="spike --isa=RV64IMAFDC $BBL"
alias linux_qemu="qemu-system-riscv64 -kernel $BBL -m 2048M -nographic"
