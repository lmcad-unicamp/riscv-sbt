FROM sbt

RUN sed -i "1s/main/& contrib non-free/" /etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y \
    adb \
    gcc-doc \
    gdb \
    man \
    ninja-build \
    nmap \
    vim \
    && rm -rf /var/lib/apt/lists/*

ENV ADB_DOCKER_WIN="adb -H docker.for.win.localhost"

RUN echo "alias ls='ls --color'" >> /root/.bashrc \
    && echo "echo 0 > /proc/sys/kernel/kptr_restrict" >> /root/.bashrc \
    && echo "source scripts/env.sh" >> /root/.bashrc \
    && echo "export ADB=\"$ADB_DOCKER_WIN\"" >> /root/.bashrc \
    && echo "tmpfs /tmp tmpfs defaults 0 0" >> /etc/fstab

COPY vimrc /root/.vimrc

CMD ["/bin/bash"]
