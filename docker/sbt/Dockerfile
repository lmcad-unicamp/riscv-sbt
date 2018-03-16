FROM sbt-llvm as llvm

FROM sbt-riscv-sbt
WORKDIR /riscv-sbt
COPY --from=llvm /riscv-sbt/toolchain /riscv-sbt/toolchain

# build-essential: needed to build sbt
# clang-3.9: needed to build sbt
# cmake: needed to build sbt
# device-tree-compiler: needed to run spike
# g++-multilib: needed to link 32-bit x86 bins
# libglib2.0-0: needed by qemu
# python3-numpy: needed by measure.py
RUN apt-get update \
    && apt-get install -y \
    build-essential \
    clang-3.9 \
    cmake \
    device-tree-compiler \
    g++-multilib \
    libglib2.0-0 \
    python3-numpy \
    && rm -rf /var/lib/apt/lists/*

ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH
ENV PATH=$TOPDIR/toolchain/release/bin:$PATH
ENV PATH=$TOPDIR/toolchain/debug/bin:$PATH
ENV MAKE_OPTS=-j2

# Copying from Windows file systems may screw up symbolic links.
# So, it's better to reset it to its original state first.
RUN git reset --hard

# XXX fast update
# RUN git checkout unstable && git pull

### SBT ###

# build sbt
RUN make sbt

# run sbt tests (all but system)
RUN mkdir -p junk && make almost-alltests

CMD ["/bin/bash"]
