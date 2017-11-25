#!/bin/bash

exec grep -rnI "$@" Makefile make test/Makefile sbt/test/Makefile mibench/Makefile
