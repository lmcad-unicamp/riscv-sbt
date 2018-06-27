#!/bin/bash

# TOPDIR
export TOPDIR=$PWD
export ARM="luporl@192.168.0.15"
export ARM_TOPDIR="~/riscv-sbt"
export ADB=adb
export ADB_TOPDIR=/data/local/tmp/riscv-sbt

# build type
if [ $# -eq 1 -a "$1" == "release" ]; then
    BUILD_TYPE=Release
    BUILD_TYPE_DIR=release
else
    BUILD_TYPE=Debug
    BUILD_TYPE_DIR=debug
fi

# toolchains: debug and release
TC=$TOPDIR/toolchain
TCR=$TC/release
TCD=$TC/debug

# scripts
SCRIPTS_DIR=$TOPDIR/scripts
export PYTHONPATH=$SCRIPTS_DIR

# set PATH

addpath()
{
    local path=$1

    echo $PATH | grep $path >/dev/null ||
    PATH=$path:$PATH
}

addpath $SCRIPTS_DIR
addpath $TCR/bin
addpath $TCD/bin
addpath $TCR/opt/riscv/bin
addpath $TCD/lowrisc-llvm/bin
export PATH

# enable core dump
ulimit -c unlimited
# echo core > /proc/sys/kernel/core_pattern

# aliases (just for convenience)

BUILD_DIR=$TOPDIR/build
PK32=$TCR/riscv32-unknown-elf/bin/pk
PK64=$TCR/riscv64-unknown-elf/bin/pk

QEMU32="qemu-riscv32 -L $TOPDIR/toolchain/release/opt/riscv/sysroot"

alias elf="$BUILD_DIR/test/sbt/elf"
alias spike32="spike $PK32"
alias spike64="spike --isa=RV64IMAFDC $PK64"
alias qemu32="$QEMU32"
alias qemu64=qemu-riscv64
alias git_status_all="git status --ignore-submodules=none"
alias git_status_noperm="git diff --name-status --diff-filter=t -G."
alias git_diff_noperm="git diff --diff-filter=t -G."
alias perfperm="echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid"
alias perfmax="echo 500000 | sudo tee /proc/sys/kernel/perf_event_max_sample_rate"

rv32-gdb()
{
    if [ ! "$1" ]; then
        echo "No program specified"
        return 1
    fi

    $QEMU32 -g 2222 $1 &
    sleep 1
    riscv64-unknown-linux-gnu-gdb \
        -ex "target remote :2222" \
        -ex "set riscv use_compressed_breakpoints off" \
        -ex "set listsize 30" \
        $1
}

arm-cp()
{
    if [ $# -eq 0 ]; then
        echo "Missing arg(s)"
        return 1
    fi

    local src
    local dst
    while [ "$1" ]; do
        src=$1
        dst=$1
        shift
        echo "scp $src $ARM:$ARM_TOPDIR/$dst"
        scp $src $ARM:$ARM_TOPDIR/$dst
    done
}

arm-modcp()
{
    arm-cp $(git status | grep modified: | awk '{print$2}' | sort | uniq)
}

# ADB copy

adb-cp()
{
    if [ $# -eq 0 ]; then
        echo "Missing arg(s)"
        return 1
    fi

    local src
    local dst
    while [ "$1" ]; do
        src=$1
        dst=$1
        shift
        echo "$ADB push $src $ADB_TOPDIR/$dst"
        $ADB push $src $ADB_TOPDIR/$dst
    done
}

# ADB on Windows

ADB36_WIN="/cygdrive/c/Users/leandro.lupori/Documents/programs/adb36/adb.exe"

adb-win-server-start()
{
    $ADB36_WIN -a nodaemon server&
}

adb-win-server-stop()
{
    $ADB36_WIN -H 127.0.0.1 kill-server
}

adb-win()
{
    $ADB36_WIN -H 127.0.0.1 $@
}
