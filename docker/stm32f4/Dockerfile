FROM debian:stable-slim

RUN apt-get update && apt-get install -y \
    wget \
    xz-utils \
    make \
    cmake \
    ccache \
    git \
    mono-complete \
    python3 \
    python3-pip \
    netcat-openbsd \
    && pip3 install --break-system-packages invoke \
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

# Ensure PATH is available in login shells
RUN echo 'export PATH=/opt/renode:/opt/arm-toolchain/bin:$PATH' >> /etc/profile

# Configure git to allow /project directory (system-level for all users)
RUN git config --system --add safe.directory /project

WORKDIR /project
