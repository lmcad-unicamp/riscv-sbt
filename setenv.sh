#!/bin/bash

# TOPDIR
export TOPDIR="$PWD"

# TC
TC="$TOPDIR/toolchain"

# PK32
export PK32=$TC/release/riscv32-unknown-elf/bin/pk

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
