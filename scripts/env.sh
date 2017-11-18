#!/bin/bash

# TOPDIR
export TOPDIR="$PWD"

# build type
if [ $# -eq 1 -a "$1" == "release" ]; then
  export BUILD_TYPE=Release
  export BUILD_TYPE_DIR=release
else
  export BUILD_TYPE=Debug
  export BUILD_TYPE_DIR=debug
fi

# TC
TC="$TOPDIR/toolchain"
export TCR=$TC/release
export TCD=$TC/debug
export TCX86=$TC/x86

# toolchain: x86
echo "$PATH" | grep "$TCX86/bin" >/dev/null ||
export PATH="$TCX86/bin:$PATH"

# toolchain: riscv release
echo "$PATH" | grep "$TCR/bin" >/dev/null ||
export PATH="$TCR/bin:$PATH"

# toolchain: riscv debug
echo "$PATH" | grep "$TCD/bin" >/dev/null ||
export PATH="$TCD/bin:$PATH"

# toolchain: riscv32-gnu-linux
echo "$PATH" | grep "$TCR/opt/riscv/bin" >/dev/null ||
export PATH="$TCR/opt/riscv/bin:$PATH"

# toolchain: lowrisc-llvm
echo "$PATH" | grep "$TCD/lowrisc-llvm/bin" >/dev/null ||
export PATH="$TCD/lowrisc-llvm/bin:$PATH"

export BUILD_DIR=$TOPDIR/build
export PK32=$TCR/riscv32-unknown-elf/bin/pk
export PK64=$TCR/riscv64-unknown-elf/bin/pk
export BBL=$TCR/riscv64-unknown-elf/bin/bbl
export ROOT_FS=$TCR/share/riscvemu/root.bin

alias elf="$BUILD_DIR/sbt/$BUILD_TYPE_DIR/test/elf"
alias spike32="spike $PK32"
alias spike64="spike --isa=RV64IMAFDC $PK64"
alias qemu32=qemu-riscv32
alias qemu64=qemu-riscv64
alias linux_spike="spike --isa=RV64IMAFDC $BBL"
alias linux_qemu="qemu-system-riscv64 -kernel $BBL -m 2048M -nographic"
alias git_status_all="git status --ignore-submodules=none"

ulimit -c unlimited
# echo core > /proc/sys/kernel/core_pattern
