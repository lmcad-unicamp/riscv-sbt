FROM sbt-toolchain

ENV TOPDIR=/riscv-sbt
WORKDIR $TOPDIR
ENV PYTHONPATH=$TOPDIR/scripts
ENV PYTHONUNBUFFERED=1
ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH
ENV PATH=$TOPDIR/toolchain/release/bin:$PATH
ENV PATH=$TOPDIR/toolchain/debug/bin:$PATH

### SBT ###

# build sbt
RUN make sbt \
    && rm -rf build

# run sbt tests (all but system)
RUN mkdir -p junk \
    && make almost-alltests \
    && rm -rf build

CMD ["/bin/bash"]
