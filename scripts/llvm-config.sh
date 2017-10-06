#!/bin/bash

if [ $# -eq 0 ]; then
  echo "usage: $0 <flag> [args]*" 1>&2
  exit 1
fi


# CaptureStartOrSpace
CStOrSp='\(^\| \)'
# CaptureNonSpace0ormore
CNSp0='\([^ ]*\)'
# CaptureNonSpace1ormore
CNSp1='\([^ ]\+\)'
##
# FilterD
FilterD="s/${CStOrSp}-D[^ ]*//g"
# FilterI
FilterI="s/${CStOrSp}-I[^ ]*//g"
# FilterNonI
FilterNonI="s/${CStOrSp}-[^I][^ ]*//g"
# FilterNonD
FilterNonD="s/${CStOrSp}-[^D][^ ]*//g"
# FilterNewLine
FILTERNL='tr -d "\n"'
# Trim
Trim="s/^ *//g; s/ *$//g; s/ \\+/ /g"
# Sep
Sep="s/ /;/g"
# FormatLibs
FormatLibs="s/${CStOrSp}-l${CNSp1}/\\1\\2/g; ${Trim}; ${Sep}"

# get BUILD_TYPE_DIR
BUILD_TYPE_DIR=$(echo $BUILD_TYPE | sed "s/.*/\\l&/")
LLVM_CONFIG=llvm-config


case $1 in
  --inc)
    $LLVM_CONFIG --cxxflags |
      sed "s/${CStOrSp}-I${CNSp1}/\\1\\2/g; ${FilterNonI}; ${Trim}; ${Sep}" |
      $FILTERNL
    ;;
  --flags)
    $LLVM_CONFIG --cxxflags |
      sed "${FilterD}; ${FilterI}; ${Trim}; ${Sep}" |
      $FILTERNL
    ;;
  --defs)
    $LLVM_CONFIG --cxxflags |
      sed "s/${CStOrSp}-D${CNSp1}/\\1\\2/g; ${FilterNonD}; ${Trim}; ${Sep}; \
        s/NDEBUG;\\?//" |
      $FILTERNL
    ;;
  --link-dirs)
    $LLVM_CONFIG --ldflags | sed "s/-L//g; ${Trim}; ${Sep}" | $FILTERNL
    ;;
  --system-libs)
    $LLVM_CONFIG --system-libs | sed "${FormatLibs}" | $FILTERNL
    ;;
  --libs)
    shift
    ${LLVM_CONFIG} --libs "$@" | sed "${FormatLibs}" | $FILTERNL
    ;;
  *)
    echo "invalid flag: $1" 1>&2
    exit 1
    ;;
esac
