FROM sbt-riscv-sbt

# riscv-gnu-toolchain
RUN apt-get update \
    && apt-get install -y \
    autoconf \
    automake \
    autotools-dev \
    bc \
    bison \
    build-essential \
    curl \
    device-tree-compiler \
    flex \
    gawk \
    gperf \
    libgmp-dev \
    libmpc-dev \
    libmpfr-dev \
    libtool \
    patchutils \
    texinfo \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

COPY src /riscv-sbt/submodules/riscv-gnu-toolchain/
WORKDIR /riscv-sbt

ENV TOPDIR=/riscv-sbt
ENV PYTHONPATH=/riscv-sbt/scripts

RUN make riscv-gnu-toolchain-newlib \
    && rm -rf build

RUN make riscv-gnu-toolchain-linux \
    && rm -rf build

CMD ["/bin/bash"]
