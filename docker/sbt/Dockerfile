FROM sbt-llvm

WORKDIR /riscv-sbt

ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH
ENV PATH=$TOPDIR/toolchain/release/bin:$PATH
ENV PATH=$TOPDIR/toolchain/debug/bin:$PATH

### SBT ###

# XXX
RUN git pull

# build sbt
RUN make sbt \
    && rm -rf build

# run sbt tests (all but system)
RUN mkdir -p junk \
    && make almost-alltests \
    && rm -rf build

CMD ["/bin/bash"]
