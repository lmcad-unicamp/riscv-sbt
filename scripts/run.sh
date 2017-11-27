#!/bin/bash
# Run a command, saves the output and return the exit status of the first
# command.
# The main purpose of this script is to allow the use of "tee" while still
# preserving the return status of the first command in the pipeline.

usage()
{
    echo "usage: $0 (--log|-o output) [--no-tee] <command> <arg>*"
}

if [ "$1" == "--log" ]; then
    shift
    set -x
    eval "$@" |& tee -a log.txt
    exit ${PIPESTATUS[0]}

elif [ "$1" == "-o" ]; then
    shift
    out=$1
    shift
    cmd=$1
    if [ "$cmd" == "--no-tee" ]; then
        notee=1
        shift
        cmd=$1
    fi

    if [ ! "$out" -o ! "$cmd" ]; then
        usage
        exit 1
    fi

    if [ "$notee" ]; then
        eval "$@" >$out 2>&1
        exit $?
    else
        eval "$@" |& tee $out
        exit ${PIPESTATUS[0]}
    fi

else
    usage
    exit 1
fi
