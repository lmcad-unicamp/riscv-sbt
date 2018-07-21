FROM debian:buster

RUN apt-get update \
    && apt-get install -y \
    build-essential \
    cmake \
    clang-6.0 \
    git \
    linux-perf \
    python3 \
    python3-numpy \
    vim \
    && rm -rf /var/lib/apt/lists/*

ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV MAKE_OPTS=-j2

ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH
ENV PATH=$TOPDIR/toolchain/release/bin:$PATH
ENV PATH=$TOPDIR/toolchain/debug/bin:$PATH
ENV PATH=$TOPDIR/toolchain/release/gcc7/opt/riscv/bin:$PATH
ENV PATH=$TOPDIR/toolchain/debug/gcc7/bin:$PATH

COPY vimrc /root/.vimrc

RUN echo "alias ls='ls --color'" >> /root/.bashrc \
    && echo "echo 0 > /proc/sys/kernel/kptr_restrict" >> /root/.bashrc \
    && echo "source scripts/env.sh" >> /root/.bashrc

WORKDIR /riscv-sbt
CMD ["/bin/bash"]
