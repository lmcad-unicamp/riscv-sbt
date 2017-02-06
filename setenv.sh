#!/bin/bash

export TOPDIR="$PWD"
TC="$TOPDIR/toolchain"
export PK32=$TC/riscv32-unknown-elf/bin/pk

echo "$PATH" | grep "$TC/bin" >/dev/null || export PATH="$TC/bin:$PATH"

# put LLVM release dir first in path, if requested
if [ $# -eq 1 -a "$1" == "release" ]; then
  echo "$PATH" | grep "$TC/release/bin" >/dev/null || \
  export PATH="$TC/release/bin:$PATH"
  export BUILD_TYPE=Release
fi
