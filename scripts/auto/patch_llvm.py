#!/usr/bin/env python3

from auto.config import *
from auto.utils import cd, path, shell

import argparse
import os


def update(mod):
    """ update mod with upstream """

    with cd(path(DIR.submodules, mod)):
        if mod == "lowrisc-llvm":
            shell("git pull")
        else:
            shell("git checkout master")
            shell("git pull")
            shell("git fetch upstream")
            shell("git merge upstream/master")
            shell("git push")
            shell("git branch -m lowrisc lowrisc-old")


def checkout(mod, commit):
    with cd(path(DIR.submodules, mod)):
        shell("git checkout -b lowrisc " + commit)


def patch_llvm():
    with cd(path(DIR.submodules, "llvm")):
        # patch llvm
        dir = "../lowrisc-llvm"
        ents = os.listdir(dir)
        ents.sort()
        patches = [p for p in ents if p.endswith(".patch")]
        for p in patches:
            shell("patch -p1 < " + dir + "/" + p)

        # patch clang
        dir = "../lowrisc-llvm/clang"
        ents = os.listdir(dir)
        ents.sort()
        patches = [p for p in ents if p.endswith(".patch")]
        for p in patches:
            shell("patch -d tools/clang -p1 < " + dir + "/" + p)


def commit(mod, sha):
    with cd(path(DIR.submodules, mod)):
        shell("git add lib test")
        shell('git commit -m "{} patched with lowrisc/riscv-llvm\n@{}"'.format(
            mod, sha))


def push(mod):
    with cd(path(DIR.submodules, mod)):
        shell("git push origin -d lowrisc")
        shell("git push -u origin lowrisc")


def add():
    with cd(DIR.top):
        shell("git add submodules/clang")
        shell("git add submodules/llvm")
        shell("git add submodules/lowrisc-llvm")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="patch llvm with lowrisc patches")
    parser.add_argument("--llvm-commit")
    parser.add_argument("--clang-commit")
    parser.add_argument("--update", action="store_true")
    parser.add_argument("--checkout", action="store_true")
    parser.add_argument("--patch", action="store_true")
    parser.add_argument("--commit", action="store_true")
    parser.add_argument("--push", action="store_true")
    parser.add_argument("--add", action="store_true")
    args = parser.parse_args()

    if args.update:
        update("lowrisc-llvm")
        update("llvm")
        update("clang")

    # Checkout the target revision of llvm+clang:
    # - Check it at https://github.com/lowRISC/riscv-llvm
    # - Look for REV=123456
    # - Use git log and look for git-svn-id and find the commit that corresponds to
    #   REV, or the closest one not greater than REV
    # - pass the SHA values to this script (--llvm-commit, --clang-commit)
    #
    # last LLVM commit: 68ee79b605bbef2de2e9addb9443a99a433a14dd
    # last clang commit: e0f57df6fa23b61ac3e065244cf190e870e9d158

    if args.checkout:
        if not args.llvm_commit:
            raise Exception("missing --llvm-commit option")
        if not args.clang_commit:
            raise Exception("missing --clang-commit option")
        checkout("llvm", args.llvm_commit)
        checkout("clang", args.clang_commit)

    if args.patch:
        patch_llvm()

    if args.commit:
        sha = ''
        with cd(path(DIR.submodules, "lowrisc-llvm")):
            sha = shell("git rev-parse HEAD", save_out=True)
        commit("llvm", sha)
        commit("clang", sha)

    if args.push:
        push("llvm")
        push("clang")

    if args.add:
        add()
