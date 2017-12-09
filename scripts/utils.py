#!/usr/bin/env python3

import subprocess


def shell(cmd, save_out=False):
    """ run shell command """
    print(cmd)
    if save_out:
        cp = subprocess.run(cmd, shell=True, check=True,
            universal_newlines=True,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return cp.stdout
    else:
        subprocess.run(cmd, shell=True, check=True)


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
