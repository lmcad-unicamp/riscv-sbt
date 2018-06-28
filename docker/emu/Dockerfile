FROM sbt-riscv-sbt

RUN apt-get update \
    && apt-get install -y \
    autoconf \
    automake \
    autotools-dev \
    bison \
    build-essential \
    device-tree-compiler \
    flex \
    libglib2.0-dev \
    libtool \
    pkg-config \
    python \
    && rm -rf /var/lib/apt/lists/*

ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH

WORKDIR /riscv-sbt
CMD ["/bin/bash"]
