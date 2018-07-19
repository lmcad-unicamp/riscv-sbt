FROM debian:buster

#RUN dpkg --add-architecture i386 \
RUN apt-get update \
    && apt-get install -y \
    build-essential \
    cmake \
    clang-6.0 \
    device-tree-compiler \
    gcc-7-i686-linux-gnu \
    gcc-7-arm-linux-gnueabihf \
    gcc-7-riscv64-linux-gnu \
    git \
    linux-perf \
    python3 \
    python3-numpy \
    vim \
    && rm -rf /var/lib/apt/lists/*

#ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH
#ENV PATH=$TOPDIR/toolchain/release/bin:$PATH
#ENV PATH=$TOPDIR/toolchain/debug/bin:$PATH

ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV MAKE_OPTS=-j2

WORKDIR /riscv-sbt
CMD ["/bin/bash"]
