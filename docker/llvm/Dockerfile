FROM sbt-riscv-sbt

# llvm/clang reqs
# clang-3.9: to build LLVM (gcc could be used instead)
RUN apt-get update \
    && apt-get install -y \
    build-essential \
    cmake \
    clang-3.9 \
    g++-multilib \
    ninja-build \
    python \
    && rm -rf /var/lib/apt/lists/*

ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH
ENV MAKE_OPTS=-j2

WORKDIR /riscv-sbt
CMD ["/bin/bash"]
