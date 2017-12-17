#!/usr/bin/env python3

from auto.config import *
import auto.pkg

GNU_TOOLCHAIN_PREFIX = None

class GnuToolchain(auto.pkg.Package):
    def __init__(self, name, prefix, build_dir,
            makefile="Makefile", configure=None, toolchain=None,
            target=None):
        super(GnuToolchain, self).__init__(name, prefix, build_dir,
            makefile=makefile, configure=configure, toolchain=toolchain)
        self.build_target = target


    def prepare(self):
        if not os.path.exists(self.build_dir):
            print("*** preparing {} ***".format(self.name))
            shell("mkdir -p {}".format(path(DIR.build, "riscv-gnu-toolchain")))
            shell("cp -a {}/riscv-gnu-toolchain {}"
                .format(DIR.submodules, self.build_dir))


    def build_and_install(self):
        if not os.path.exists(path(self.prefix, self.toolchain)):
            self.configure()

            print("*** building and installing " + self.name + " ***")
            self._make(self.build_target)


def _pkgs():
    # riscv-gnu-toolchain
    name = "riscv-gnu-toolchain"
    prefix = path(DIR.toolchain_release, "opt/riscv")
    global GNU_TOOLCHAIN_PREFIX
    GNU_TOOLCHAIN_PREFIX = prefix
    build_dir = path(DIR.build, "riscv-gnu-toolchain/newlib32")

    configure = cat(
        "./configure",
        "--prefix=" + prefix,
        "--with-arch=rv32gc",
        "--with-abi=ilp32d")

    toolchain = "bin/riscv32-unknown-elf-gcc"

    pkg1 = GnuToolchain(name, prefix, build_dir,
            configure=configure,
            toolchain=toolchain)


    # riscv-gnu-toolchain-linux
    name = "riscv-gnu-toolchain-linux"
    build_dir = path(DIR.build, "riscv-gnu-toolchain/linux")

    configure = cat(
       "./configure",
       "--prefix=" + prefix,
       "--enable-multilib"
    )

    toolchain = path(prefix, "bin/riscv64-unknown-linux-gnu-gcc")

    pkg2 = GnuToolchain(name, prefix, build_dir,
            configure=configure,
            toolchain=toolchain,
            target="linux")

    return [pkg1, pkg2]

auto.pkg.Package.pkgs.extend(_pkgs())
