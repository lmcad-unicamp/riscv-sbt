#!/bin/bash

if [ x`whoami` != xroot ]; then
  echo "$0 need to be run as root"
  exit 1
fi

lsmod | grep msr >/dev/null || modprobe msr

NCORES=`cat /proc/cpuinfo | grep -c processor`
echo NCORES=$NCORES
MAXCORE=$((NCORES-1))
echo MAXCORE=$MAXCORE

# Enable all counters in IA32_PERF_GLOBAL_CTRL
#   bits 34:32 enabled the three fixed function counters
#   bits 3:0 enable the four programmable counters
echo "IA32_PERF_GLOBAL_CTRL (0x38F) (should be 000000070000000f)"

# 0: OS
# 1: User
# 2: ?
# 3: PMI
# (CTR0)
# CTR1: 4,5,6,7
# CTR2: 8,9,10,11
echo "IA32_FIXED_CTR_CTRL (0x38D) (should be 00000000000003b3)"
for core in `seq 0 $MAXCORE`; do
  msr=0x38F
  echo -n "$msr $core "
  rdmsr -p $core -x -0 $msr
  wrmsr -p $core $msr 0x000000070000000f

  msr=0x38D
  echo -n "$msr $core "
  rdmsr -p $core -x -0 $msr
  wrmsr -p $core $msr 0x00000000000003b3
done

echo 2 > /sys/devices/cpu/rdpmc
