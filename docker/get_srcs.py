#!/usr/bin/env python3

from auto.utils import cd, path, shell

import argparse
import re
import os

TOPDIR          = os.environ["TOPDIR"]
DOCKER_DIR      = path(TOPDIR, "docker")
OUTPUT_DIR      = path(DOCKER_DIR, "output")

SRC_DIR         = path(OUTPUT_DIR, "src")
BUILD_DIR       = path(OUTPUT_DIR, "build")
TOOLCHAIN_DIR   = path(OUTPUT_DIR, "toolchain")


class Commit:
    def __init__(self, module, commit):
        self.module = module
        self.commit = commit


class Commits:

    def get_current(self):
        commits = []

        out = shell("cd {} && git submodule".format(TOPDIR), save_out=True)
        lines = out.split("\n")
        patt = re.compile(" *([^ ]+) +submodules/([^ ]+) *")
        for l in lines:
            r = re.match(patt, l)
            if r:
                commit = r.group(1)
                module = r.group(2)
                commits.append(Commit(module, commit))

        # add sbt
        commits.append(Commit("sbt",
            shell("cd {} && git rev-parse HEAD", save_out=True)
                .format(TOPDIR)
                .strip()))
        return commits


    def save(self, commits):
        with open("commits.txt", "w") as f:
            for commit in commits:
                f.write("{} {}\n".format(commit.module, commit.commit))


    def load(self):
        self.commits = {}
        with open("commits.txt", "r") as f:
            patt = re.compile("([^ ]+) ([^ ]+)")
            for line in f:
                r = re.match(patt, line)
                self.commits[r.group(1)] = r.group(2)


    def __getitem__(self, key):
        return self.commits[key]



class Source:
    def __init__(self, name, url, clone_flags=None):
        self.name = name
        self.url = url
        self.clone_flags = clone_flags


    def get(self):
        if not os.path.exists(self.name):
            cmd = "git clone {} {}".format(self.clone_flags, self.url)
            shell(cmd)
            with cd(self.name):
                shell("git checkout " + self.commit)


def get_srcs():
    commits = Commits()
    commits.load()

    srcs = [
        Source("riscv-sbt",
            "https://github.com/OpenISA/riscv-sbt.git"),
        Source("riscv-gnu-toolchain",
            "https://github.com/riscv/riscv-gnu-toolchain",
            "--recursive"),
        Source("llvm",
            "https://github.com/OpenISA/llvm",
            "--recursive -b lowrisc"),
        Source("clang",
            "https://github.com/OpenISA/clang",
            "--recursive -b lowrisc"),
        Source("riscv-fesvr",
            "https://github.com/riscv/riscv-fesvr",
            "--recursive"),
        Source("riscv-pk",
            "https://github.com/riscv/riscv-pk",
            "--recursive"),
        Source("riscv-isa-sim",
            "https://github.com/riscv/riscv-isa-sim",
            "--recursive"),
        Source("riscv-qemu-tests",
            "https://github.com/arsv/riscv-qemu-tests",
            "--recursive")
    ]

    # set commits
    for src in srcs:
        src.commit = commits[src.name]

    def mkdir_if_needed(dir):
        if not os.path.exists(dir):
            shell("mkdir -p " + dir)

    mkdir_if_needed(SRC_DIR)

    # get all sources
    with cd(SRC_DIR):
        for src in srcs:
            src.get()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="source getter")
    parser.add_argument("--save-current-commits", action="store_true")

    args = parser.parse_args()

    if args.save_current_commits:
        commits = Commits()
        commits.save(commits.get_current())
    else:
        get_srcs()
