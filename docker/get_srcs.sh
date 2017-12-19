#!/bin/bash

set -x
set -e
set -u

TOPDIR=$PWD/riscv-sbt

get_src()
{
    local name=$1
    shift
    local commit=$1
    shift

    if [ ! -d $TOPDIR/submodules/$name ]; then
            cd $TOPDIR/submodules
            # clone
            $@
            cd $name
            git checkout $commit
    fi
}

if [ ! -d $TOPDIR ]; then
    git clone https://github.com/OpenISA/riscv-sbt.git
    cd riscv-sbt
    git checkout 699d342fbff312a76da2c23c9f47da70780875a0
    rmdir submodules/*
fi

get_src riscv-gnu-toolchain bf5697a1a6509705b50dcc1f67b8c620a7b21ec4 \
git clone --recursive https://github.com/riscv/riscv-gnu-toolchain

get_src llvm 5c76c41588f9816dc750c7be565a1f109b39c72f \
git clone --recursive -b lowrisc https://github.com/OpenISA/llvm

get_src clang 1e850113a8799cf1aa5a8b9e9fa6d8181f96b8d2 \
git clone --recursive -b lowrisc https://github.com/OpenISA/clang

get_src riscv-fesvr f683e01542acf60e50774d061bcb396b628e3e67 \
git clone --recursive https://github.com/riscv/riscv-fesvr

get_src riscv-pk 18087efa98d77918d55127c3d7745cd6d6d9d77b \
git clone --recursive https://github.com/riscv/riscv-pk

get_src riscv-isa-sim 3b1e9ab7522b3b20cde6bd8d9f2b28222463cf1b \
git clone --recursive https://github.com/riscv/riscv-isa-sim

get_src riscv-qemu-tests 92f007e7c145976dbf64de43d99679461272d108 \
git clone --recursive https://github.com/arsv/riscv-qemu-tests
