#!/usr/bin/env python3

from auto.utils import cd, path, shell
import auto.gnu_toolchain

import argparse
import os

class Package:
    def __init__(self, name, prefix, build_dir,
            makefile="Makefile", configure=None, toolchain=None):
        self.name = name
        self.prefix = prefix
        self.build_dir = build_dir
        self.makefile = makefile
        self._configure = configure
        self.toolchain = toolchain


    def prepare(self):
        pass


    def configure(self):
        # TODO check that self.deps exist

        if not self._configure:
            raise Exception("configure not set!")

        self.prepare()

        if not os.path.exists(self.build_dir):
            shell("mkdir -p " + self.build_dir)

        if not os.path.exists(path(self.build_dir, self.makefile)):
            with cd(self.build_dir):
                print("*** configuring " + self.name + " ***")
                shell(self._configure)


    def build(self):
        if not os.path.exists(path(self.build_dir, self.build_out)):
            self.configure()

            print("*** building {} ***".format(self.name))
            shell('make -C {} {}$(MAKE_OPTS)'
                .format(self.build_dir,
                    self.build_target + " " if self.build_target else ''))


    def install(self):
        if not os.path.exists(path(self.prefix, self.toolchain)):
            self.build()

            print("*** installing {} ***".format(self.name))
            shell('make -C {} install $(MAKE_OPTS)'.format(self.build_dir))


    def build_and_install(self):
        self.build()
        self.install()


if __name__ == "__main__":
    pkgs = auto.gnu_toolchain.pkgs()

    parser = argparse.ArgumentParser(description="build packages")
    parser.add_argument("pkg")
    args = parser.parse_args()

    pkg = None
    for p in pkgs:
        if p.name == args.pkg:
            pkg = p
            break

    print("building", pkg.name)
    pkg.build_and_install()
