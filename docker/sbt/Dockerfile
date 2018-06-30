FROM sbt-riscv-sbt

# build-essential: needed to build sbt
# clang-3.9: needed to build sbt
# cmake: needed to build sbt
# device-tree-compiler: needed to run spike
# g++-6-multilib: gcc for x86 and x64
# gcc-6-arm-linux-gnueabihf: cross gcc for ARM
# libc6-dev:i386: headers/libraries needed by x86 binaries
# libglib2.0-0: needed by qemu
# libunwind8:i386: needed by perf
# linux-headers-amd64: maybe needed by perf?
# linux-perf-4.9:i386: perf for x86 binaries
# python3-numpy: needed by measure.py
RUN dpkg --add-architecture i386 \
    && apt-get update \
    && apt-get install -y \
    build-essential \
    clang-3.9 \
    cmake \
    device-tree-compiler \
    g++-6-multilib \
    gcc-6-arm-linux-gnueabihf \
    libc6-dev:i386 \
    libglib2.0-0 \
    libunwind8:i386 \
    linux-headers-amd64 \
    linux-perf-4.9:i386 \
    python3-numpy \
    && rm -rf /var/lib/apt/lists/*

ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH
ENV PATH=$TOPDIR/toolchain/release/bin:$PATH
ENV PATH=$TOPDIR/toolchain/debug/bin:$PATH

CMD ["/bin/bash"]
