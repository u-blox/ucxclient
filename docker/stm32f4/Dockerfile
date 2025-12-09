FROM ubuntu:22.04

# Non-interactive debconf package configuration
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    wget \
    xz-utils \
    make \
    cmake \
    ccache \
    git \
    mono-complete \
    python3 python3-pip python-is-python3 python3-dev python3-wheel \
    netcat-openbsd \
    sudo \
    gdb-multiarch \
    && pip3 install invoke \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Download and install official ARM GNU Toolchain (13.2.Rel1)
RUN wget -q https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz \
    && tar xf arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz -C /opt \
    && rm arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz \
    && ln -s /opt/arm-gnu-toolchain-13.2.Rel1-x86_64-arm-none-eabi /opt/arm-toolchain

# Add toolchain to PATH
ENV PATH="/opt/arm-toolchain/bin:${PATH}"

# Download and install Renode (latest stable)
RUN wget -q https://github.com/renode/renode/releases/download/v1.15.3/renode-1.15.3.linux-portable.tar.gz \
    && tar xf renode-1.15.3.linux-portable.tar.gz -C /opt \
    && rm renode-1.15.3.linux-portable.tar.gz \
    && ln -s /opt/renode_1.15.3_portable /opt/renode

# Add Renode to PATH
ENV PATH="/opt/renode:${PATH}"

# Configure ccache to use a directory accessible to all users
ENV CCACHE_DIR=/tmp/ccache
RUN mkdir -p /tmp/ccache && chmod 777 /tmp/ccache

# Mark that we're inside the STM32 Docker container (prevents re-invoking docker)
ENV UCX_STM32_DOCKER=1

# Ensure PATH is available in login shells
RUN echo 'export PATH=/opt/renode:/opt/arm-toolchain/bin:$PATH' >> /etc/profile

# Configure git to allow /project directory (system-level for all users)
RUN git config --system --add safe.directory /project \
    && git config --system --add safe.directory '*'

# Create vscode user for devcontainer/codespaces support
RUN groupadd --gid 1000 vscode \
    && useradd --uid 1000 --gid 1000 -m vscode \
    && echo vscode ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/vscode \
    && chmod 0440 /etc/sudoers.d/vscode

WORKDIR /project
