FROM debian:stretch

RUN apt-get update && \
    apt-get install -y \
    git \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/OpenISA/riscv-sbt.git

RUN cd /riscv-sbt && git submodule update --init --recursive

# RISC-V toolchain pre-reqs
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

#    libusb-1.0-0-dev \
#    pkg-config \

CMD ["/bin/bash"]
