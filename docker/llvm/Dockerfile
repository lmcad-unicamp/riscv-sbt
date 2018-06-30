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

ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH

CMD ["/bin/bash"]
