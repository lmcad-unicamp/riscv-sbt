BENCHNAME=basicmath_small
BENCHNAME2=basicmath_large
#ARCH=arm
#MATTR=-mattr=vfp3,d16,a8,-neon -mcpu=cortex-a8 -float-abi=hard

FILE1 = basicmath_small.c   rad2deg.c  cubic.c   isqrt.c
FILE2 = basicmath_large.c   rad2deg.c  cubic.c   isqrt.c

TARGETS =basicmath_small basicmath_large

SBTOPT?=-optimize -oneregion

include ../../Makefile.common
