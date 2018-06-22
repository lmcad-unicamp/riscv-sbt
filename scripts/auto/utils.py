#!/usr/bin/env python3

import os
import subprocess

class cd:
    def __init__(self, dir):
        self.dir = os.path.expanduser(dir)

    def __enter__(self):
        self.prev_dir = os.getcwd()
        print("cd", self.dir)
        os.chdir(self.dir)

    def __exit__(self, etype, value, traceback):
        print("cd", self.prev_dir)
        os.chdir(self.prev_dir)


def shell(cmd, save_out=False, bin=False, exp_rc=0, quiet=False):
    """ run shell command """
    if not quiet:
        print(cmd)

    check = exp_rc == 0
    def check_rc(cp):
        if exp_rc != 0:
            if cp.returncode != exp_rc:
                raise Exception("shell(" + cmd + ") failed! rc=" +
                        str(cp.returncode))

    if save_out:
        if bin:
            cp = subprocess.run(cmd, shell=True, check=check,
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            check_rc(cp)
            return cp.stdout
        else:
            cp = subprocess.run(cmd, shell=True, check=check,
                universal_newlines=True,
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            check_rc(cp)
            return cp.stdout
    else:
        cp = subprocess.run(cmd, shell=True, check=check)
        check_rc(cp)


def cat(*args):
    """ concatenate args """
    out = ""
    for arg in args:
        if not arg:
            pass
        elif len(arg) > 0:
            if len(out) == 0:
                out = arg
            else:
                out = out + " " + arg
    return out


def chsuf(s, ns):
    """ change suffix
        s - string
        ns - new suffix
    """
    i = s.rindex(".")
    # check there is at least on suffix char
    if len(s) -1 -i < 1:
        raise Exception("no suffix")
    return s[0:i] + ns


def path(dir, file):
    if dir == ".":
        return file
    else:
        return dir + "/" + file


def mpath(*args):
    dir = args[0]
    p1 = args[1]

    out = path(dir, p1)

    for p in args[2:]:
        out = out + "/" + p
    return out


def xarch_brk(xarch):
    dash = xarch.index("-")
    farch = xarch[0 : dash]
    narch = xarch[dash + 1 :]
    return farch, narch


def mkdir_if_needed(dir):
    if not os.path.exists(dir):
        shell("mkdir -p " + dir)


def unique(l):
    u = []
    for i in l:
        if not i in u:
            u.append(i)
    return u


def xassert(v):
    if v == None:
        raise Exception("v is None")
    if not v:
        raise Exception("not v")
