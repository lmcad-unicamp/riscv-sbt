#!/usr/bin/env python3

from auto.config import DIR
from auto.utils import cat, path, shell

import auto.pkg

import os

class GnuToolchain(auto.pkg.Package):

    PREFIX = None
    PREFIX_GCC7 = None

    def __init__(self, name, prefix, build_dir,
            makefile="Makefile", configure=None, toolchain=None,
            target=None):
        super(GnuToolchain, self).__init__(name, prefix, build_dir,
            makefile=makefile, configure=configure, toolchain=toolchain)
        self.build_target = target


    def _prepare(self):
        if not os.path.exists(self.build_dir):
            print("*** preparing {} ***".format(self.name))
            shell("cp -a {}/riscv-gnu-toolchain {}"
                .format(DIR.submodules, self.build_dir))


    def _build_and_install(self):
        self.configure()

        print("*** building and installing " + self.name + " ***")
        self._make(self.build_target)


def _pkgs():
    pkgs = []

    # riscv-gnu-toolchain
    name = "riscv-gnu-toolchain-newlib"
    prefix = path(DIR.toolchain_release, "opt/riscv")
    GnuToolchain.PREFIX = prefix
    build_dir = path(DIR.build, name)

    def configure1(prefix):
        return cat(
            "./configure",
            "--prefix=" + prefix,
            "--with-arch=rv32gc",
            "--with-abi=ilp32d")

    toolchain1 = "bin/riscv32-unknown-elf-gcc"

    pkgs.append(
        GnuToolchain(name, prefix, build_dir,
            configure=configure1(prefix),
            toolchain=toolchain1))

    # riscv-gnu-toolchain-linux
    name = "riscv-gnu-toolchain-linux"
    build_dir = path(DIR.build, name)

    def configure2(prefix):
        return cat(
           "./configure",
           "--prefix=" + prefix,
           "--enable-multilib")

    toolchain2 = "bin/riscv64-unknown-linux-gnu-gcc"

    pkgs.append(
        GnuToolchain(name, prefix, build_dir,
            configure=configure2(prefix),
            toolchain=toolchain2,
            target="linux"))


    ### gcc7 ###

    # riscv-gnu-toolchain
    name = "riscv-gnu-toolchain-newlib-gcc7"
    prefix = path(DIR.toolchain_release, "gcc7/opt/riscv")
    GnuToolchain.PREFIX_GCC7 = prefix
    build_dir = path(DIR.build, name)

    pkgs.append(
        GnuToolchain(name, prefix, build_dir,
            configure=configure1(prefix),
            toolchain=toolchain1))

    # riscv-gnu-toolchain-linux
    name = "riscv-gnu-toolchain-linux-gcc7"
    build_dir = path(DIR.build, name)

    pkgs.append(
        GnuToolchain(name, prefix, build_dir,
            configure=configure2(prefix),
            toolchain=toolchain2,
            target="linux"))

    return pkgs

auto.pkg.Package.pkgs.extend(_pkgs())
