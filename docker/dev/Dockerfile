FROM debian:stretch

WORKDIR /riscv-sbt
ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH
ENV PATH=$TOPDIR/toolchain/release/bin:$PATH
ENV PATH=$TOPDIR/toolchain/debug/bin:$PATH
ENV MAKE_OPTS=-j2

RUN apt-get update \
    && apt-get install -y \
    build-essential \
    clang-3.9 \
    cmake \
    device-tree-compiler \
    g++-multilib \
    gdb \
    git \
    libglib2.0-0 \
    man \
    vim \
    python3 \
    python3-numpy \
    && rm -rf /var/lib/apt/lists/*

RUN echo "alias ls='ls --color'" >> /root/.bashrc

CMD ["/bin/bash"]
