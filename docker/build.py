#!/usr/bin/env python3

from auto.utils import cat, cd, mkdir_if_needed, mpath, path, shell

import argparse
import re
import os
import sys

TOPDIR          = os.environ["TOPDIR"]
DOCKER_DIR      = path(TOPDIR, "docker")

#
# Helper
#

class Helper:
    @staticmethod
    def is_cygwin():
        try:
            shell("uname | grep -i cygwin")
            return True
        except:
            return False

    @staticmethod
    def cygpath(path):
        if Helper.is_cygwin():
            return shell("cygpath -m " + path, save_out=True).strip()
        else:
            return path

    @staticmethod
    def clean_imgs():
        # remove all container instances
        containers = shell("docker ps -a | awk '{print$1}' | sed 1d",
            save_out=True).split()
        if len(containers) > 0:
            shell("docker rm " + " ".join(containers))
        # remove all '<none>' and 'sbt' images
        images = shell(
            "docker images | grep -e sbt -e '<none>' | awk '{print$3}'",
            save_out=True).split()
        if len(images) > 0:
            shell("docker rmi " + " ".join(images))

#
# Volumes
#

class Volume:
    def __init__(self, name, vol_name, mount_point):
        self.name = name
        self.vol_name = vol_name
        self.mount_point = mount_point


    def create(self):
        # create volume if needed
        try:
            shell("docker volume ls | grep " + self.vol_name)
        except:
            shell("docker volume create " + self.vol_name)


    def volstr(self):
        return "--mount type=volume,source={},destination={}".format(
                    self.vol_name, self.mount_point)


class Volumes:
    def __init__(self):
        self.vols = {
            "build":
                Volume("build", "sbt-vol-build", "/riscv-sbt/build"),
            "toolchain":
                Volume("toolchain", "sbt-vol-toolchain", "/riscv-sbt/toolchain")
        }


    def __getitem__(self, key):
        return self.vols[key]


    def volstr(self):
        vstr = ''
        for k, v in self.vols.items():
            vstr = cat(vstr, v.volstr())
        return vstr


VOLS = Volumes()

#
# Docker
#

class Docker:
    def __init__(self, name, img):
        self.name = name
        self.img = img


    def cmd(self, dcmd, cmd, interactive, vols=None):
        if Helper.is_cygwin():
            prefix = "winpty "
        else:
            prefix = ''

        privileged = True
        make_opts = os.getenv("MAKE_OPTS")
        if make_opts:
            env = "MAKE_OPTS=" + make_opts
        else:
            env = None
        fmtdata = {
            "prefix":       prefix if interactive else "",
            "dcmd":         dcmd,
            "privileged":   " --privileged" if privileged else "",
            "interactive":  " -it" if interactive else  "",
            "rm":           " --rm" if dcmd == "run" else "",
            "hostname":     " -h dev" if dcmd == "run" else "",
            "name":         " --name " + self.name if dcmd == "run" else "",
            "vols":         " " + vols if vols else "",
            "img":          " " + self.img,
            "cmd":          " " + cmd if cmd else "",
            "env":          " -e " + env if env and dcmd == "run" else "",
        }

        shell(("{prefix}docker {dcmd}" +
            "{privileged}{interactive}{rm}{hostname}{name}{vols}{env}{img}{cmd}"
            ).format(**fmtdata))


    def run(self, cmd=None, interactive=False):
        self.cmd("run", cmd, interactive, vols=Docker.volstr())


    def exec(self, cmd, interactive=True):
        self.cmd("exec", cmd, interactive)


    @staticmethod
    def volstr():
        src = Helper.cygpath(TOPDIR)
        dst = "/riscv-sbt"
        vstr = "--mount type=bind,source={},destination={}".format(src, dst)
        vstr = cat(vstr, VOLS.volstr())
        return vstr

#
# Commits
#

# module + commit_hash
class Commit:
    def __init__(self, module, commit):
        self.module = module
        self.commit = commit


class Commits:
    def __init__(self, topdir, docker_dir):
        self.topdir = topdir
        self.commits_txt = path(docker_dir, "commits.txt")


    # get current commit hashes
    def get_current(self):
        commits = []

        with cd(self.topdir):
            # get submodules
            out = shell("git submodule", save_out=True)
            lines = out.split("\n")
            patt = re.compile("[ -]*([^ ]+) +submodules/([^ ]+) *")
            for l in lines:
                if l.find("lowrisc-llvm") >= 0:
                    continue
                r = re.match(patt, l)
                if r:
                    commit = r.group(1)
                    module = r.group(2)
                    commits.append(Commit(module, commit))

            # add sbt
            commits.append(Commit("riscv-sbt",
                shell("git rev-parse HEAD", save_out=True)
                    .format(self.topdir)
                    .strip()))
            return commits


    # save to file
    def save(self, commits):
        with open(self.commits_txt, "w") as f:
            for commit in commits:
                f.write("{} {}\n".format(commit.module, commit.commit))


    # load from file
    def load(self):
        self.commits = {}
        with open(self.commits_txt, "r") as f:
            patt = re.compile("([^ ]+) ([^ ]+)")
            for line in f:
                r = re.match(patt, line)
                self.commits[r.group(1)] = r.group(2)
        return self


    def __getitem__(self, key):
        return self.commits[key]

#
# Sources
#

# source package
class Source:
    def __init__(self, name, url, dir=None, clone_flags=None):
        self.name = name
        if dir == "top":
            self.dstdir = TOPDIR
            self.parent_dir = None
        else:
            self.parent_dir = path(TOPDIR, "submodules")
            self.dstdir = path(self.parent_dir, name)
        self.url = url
        self.clone_flags = clone_flags
        self.update = True


    def get(self):
        if not os.path.exists(self.dstdir):
            mkdir_if_needed(self.parent_dir)
            with cd(self.parent_dir):
                cmd = cat("git clone", self.clone_flags, self.url)
                shell(cmd)
        elif self.update:
            with cd(self.dstdir):
                shell("git fetch")

        with cd(self.dstdir):
            shell((
                "if [ `git rev-parse HEAD` != {0} ]; then " +
                    "git checkout {0}; fi").format(self.commit.strip()))


class Sources:
    def __init__(self):
        self.srcs = [
            Source("riscv-sbt",
                "https://github.com/OpenISA/riscv-sbt.git",
                dir="top"),
            Source("riscv-gnu-toolchain",
                "https://github.com/riscv/riscv-gnu-toolchain",
                clone_flags="--recursive"),
            Source("llvm",
                "https://github.com/OpenISA/llvm",
                clone_flags="--recursive -b lowrisc"),
            Source("clang",
                "https://github.com/OpenISA/clang",
                clone_flags="--recursive -b lowrisc"),
            Source("riscv-fesvr",
                "https://github.com/riscv/riscv-fesvr",
                clone_flags="--recursive"),
            Source("riscv-pk",
                "https://github.com/riscv/riscv-pk",
                clone_flags="--recursive"),
            Source("riscv-isa-sim",
                "https://github.com/riscv/riscv-isa-sim",
                clone_flags="--recursive"),
            Source("riscv-qemu-tests",
                "https://github.com/arsv/riscv-qemu-tests",
                clone_flags="--recursive")
        ]

        commits = Commits(TOPDIR, DOCKER_DIR)
        commits.load()

        # set commits
        for src in self.srcs:
            src.commit = commits[src.name]

    # get all sources
    def get(self):
        for src in self.srcs:
            src.get()


#
# Images
#

class Image:
    commits = Commits(TOPDIR, DOCKER_DIR).load()

    def __init__(self, name, img=None):
        self.name = name
        if not img:
            self.img = "sbt-" + name
        else:
            self.img = img


    # build if done file does not exist
    def build(self, force):
        dir = path(DOCKER_DIR, self.name)
        done = path(dir, "done")
        # skip if done
        if not force and os.path.exists(done):
            return

        # docker build
        with cd(DOCKER_DIR):
            shell("docker build -t {} {}".format(
                self.img, self.name))
            self._build()
            shell("touch " + done)


    def _build(self):
        name = self.name
        docker = Docker(name, self.img)
        if name == "riscv-sbt":
            pass
        elif name == "riscv-gnu-toolchain":
            VOLS["build"].create()
            VOLS["toolchain"].create()
            docker.run("make riscv-gnu-toolchain-newlib")
            docker.run("make riscv-gnu-toolchain-linux")
        elif name == "emu":
            docker.run("make spike")
            docker.run("make qemu")
        elif name == "llvm":
            docker.run("make llvm")
        elif name == "sbt":
            docker.run("make sbt")
            # run sbt tests (all but system)
            docker.run('bash -c "' +
                # sbt log dir
                'mkdir -p junk && ' +
                # restore symlinks (needed if in cygwin fs)
                'git checkout HEAD test/sbt/rv32-hello.s && ' +
                'git checkout HEAD riscv-qemu-tests && ' +
                # build and run tests
                '. scripts/env.sh && ' +
                'make almost-alltests"')
        elif name == "dev":
            pass
        else:
            raise Exception("Invalid: build " + name)


class Images:
    def __init__(self):
        self.imgs = [
            Image("riscv-sbt"),
            Image("riscv-gnu-toolchain"),
            Image("emu"),
            Image("llvm"),
            Image("sbt", img="sbt"),
            Image("dev")
        ]
        self._iter = None


    def build(self, name, force):
        # build all?
        if name == "all":
            for img in self.imgs:
                img.build(force)
            return

        # find and build img by name
        for img in self.imgs:
            if img.name == name:
                img.build(force)
                break
        else:
            sys.exit("ERROR: component not found: " + args.build)


    def __iter__(self):
        self._iter = iter(self.imgs)
        return self._iter


#
# main
#

if __name__ == "__main__":
    imgs = Images()
    names = [img.name for img in imgs]

    parser = argparse.ArgumentParser(description="docker build helper")
    parser.add_argument("--save-current-commits", action="store_true",
        help="save the current commit hash of each submodule in commits.txt")
    parser.add_argument("--get-srcs", action="store_true",
        help="clone and checkout all needed sources")
    parser.add_argument("--clean", action="store_true",
        help="remove done files from every docker build dir")
    parser.add_argument("--clean-imgs", action="store_true",
        help="remove <none> and docker related images")
    parser.add_argument("--build", metavar="img", type=str,
        choices=names + ["all"],
        help="build img. imgs=[{}]".format(", ".join(names + ["all"])))
    parser.add_argument("--run", type=str, help="run a docker image")
    parser.add_argument("--exec", type=str,
        help="exec bash on an existing container")
    parser.add_argument("--rdev", action="store_true",
        help="run dev container")
    parser.add_argument("--xdev", action="store_true",
        help="exec bash on an existing dev container")
    parser.add_argument("-f", "--force", action="store_true")
    args = parser.parse_args()


    def run(img):
        name = img.replace("sbt-", "")
        docker = Docker(name, img)
        docker.run(interactive=True)

    def exec(img):
        name = img.replace("sbt-", "")
        docker = Docker(name, img)
        docker.exec("/bin/bash", interactive=True)


    # --save-current-commits
    if args.save_current_commits:
        commits = Commits(TOPDIR, DOCKER_DIR)
        commits.save(commits.get_current())
    # --get-srcs
    elif args.get_srcs:
        srcs = Sources()
        srcs.get()
    # --clean
    elif args.clean:
        with cd(DOCKER_DIR):
            shell("rm -f {}".format(
                " ".join([name + "/done" for name in names])))
    # --clean-imgs
    elif args.clean_imgs:
        Helper.clean_imgs()
    # --build
    elif args.build:
        imgs.build(args.build, args.force)
    # --run
    elif args.run:
        run(args.run)
    # --exec
    elif args.exec:
        exec(args.exec)
    # --rdev
    elif args.rdev:
        run("sbt-dev")
    # --xdev
    elif args.xdev:
        exec("sbt-dev")
    # error
    else:
        sys.exit("ERROR: no command specified")
