#!/usr/bin/env python3

from auto.config import DIR, RV32
from auto.pkg import Package
from auto.utils import cat, cd, path, shell

import os


def _pkgs():
    pkgs = []

    # riscv-fesvr
    name = "riscv-fesvr"
    prefix = DIR.toolchain_release
    build_dir = path(DIR.build, "riscv-fesvr")
    build_out = "libfesvr.so"
    toolchain = "lib/libfesvr.so"
    configure = cat(
        path(DIR.submodules, "riscv-fesvr/configure"),
        "--prefix=" + prefix)

    pkgs.append(
        Package(name, prefix, build_dir,
            configure=configure,
            build_out=build_out,
            toolchain=toolchain))


    # riscv-isa-sim
    name = "riscv-isa-sim"
    prefix = DIR.toolchain_release
    build_dir = path(DIR.build, name)
    build_out = "spike"
    toolchain = "bin/spike"
    configure = cat(
        path(DIR.submodules, "riscv-isa-sim/configure"),
        "--prefix=" + prefix,
        "--with-fesvr=" + prefix,
        "--with-isa=RV32IMAFDC"
    )
    deps = ["riscv-fesvr"]

    pkgs.append(
        Package(name, prefix, build_dir,
            configure=configure,
            build_out=build_out,
            toolchain=toolchain,
            deps=deps))


    # riscv-pk

    # riscv-pk 32 bit
    name = "riscv-pk-32"
    prefix = DIR.toolchain_release
    build_dir = path(DIR.build, name)
    build_out = "pk"
    srcdir = path(DIR.submodules, "riscv-pk")
    toolchain = path(RV32.triple, "bin/pk")

    configure = cat(
        path(srcdir, "configure"),
        "--prefix=" + prefix,
        "--host=" + RV32.triple,
        "--with-arch=rv32g")

    class RISCVPK(Package):
        def _prepare(self):
            stamp = path(self.build_dir, ".patched")
            if not os.path.exists(stamp):
                with cd(srcdir):
                    shell("patch < " + path(DIR.patches, "riscv-pk-32-bit-build-fix.patch"))
                    shell("mkdir -p " + self.build_dir)
                    shell("touch " + stamp)


        def _clean(self):
            shell("rm -rf " + self.build_dir)
            # unpatch
            with cd(srcdir):
                shell("git checkout configure configure.ac")

    pkg = RISCVPK(name, prefix, build_dir,
            configure=configure,
            build_out=build_out,
            toolchain=toolchain)

    pkgs.append(pkg)

    return pkgs


Package.pkgs.extend(_pkgs())
