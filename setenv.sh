export TOPDIR="$PWD"
TC="$TOPDIR/toolchain/bin"
echo "$PATH" | grep "$TC" >/dev/null || export PATH="$TC:$PATH"
