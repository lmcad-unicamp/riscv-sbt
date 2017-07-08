#!/bin/bash

set -x
eval "$@" |& tee -a log.txt
exit ${PIPESTATUS[0]}
