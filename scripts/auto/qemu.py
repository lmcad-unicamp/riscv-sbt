#!/usr/bin/env python3

from auto.config import *
from auto.pkg import Package

import os


def _pkgs():
    pkgs = []

    # qemu user
    name = "qemu-user"
    prefix = DIR.toolchain_release
    build_dir = path(DIR.build, name)
    build_out = "riscv32-linux-user/qemu-riscv32"
    toolchain = "bin/qemu-riscv32"
    configure = cat(
        path(DIR.submodules, "riscv-qemu/configure"),
        "--prefix=" + prefix,
        "--target-list=riscv64-linux-user,riscv32-linux-user"
    )

    class QEMU(Package):
        def _prepare(self):
            link = path(DIR.submodules, "riscv-qemu")
            if not os.path.exists(link):
                shell("ln -sf {}/riscv-gnu-toolchain/riscv-qemu {}"
                    .format(DIR.submodules, link))

    pkgs.append(
        QEMU(name, prefix, build_dir,
            configure=configure,
            build_out=build_out,
            toolchain=toolchain))

    # qemu system
    name = "qemu-system"
    prefix = DIR.toolchain_release
    build_dir = path(DIR.build, name)
    build_out = "qemu-system-riscv32"
    toolchain = "bin/qemu-system-riscv32"
    configure = cat(
        path(DIR.submodules, "riscv-qemu/configure"),
        "--prefix=" + prefix,
        "--target-list=riscv64-softmmu,riscv32-softmmu"
    )

    pkgs.append(
        QEMU(name, prefix, build_dir,
            configure=configure,
            build_out=build_out,
            toolchain=toolchain))

    return pkgs


Package.pkgs.extend(_pkgs())
