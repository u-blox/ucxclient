FROM debian:stable-slim

RUN apt-get update && apt-get install -y \
    wget \
    xz-utils \
    make \
    cmake \
    git \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Download and install official ARM GNU Toolchain (13.2.Rel1)
RUN wget -q https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz \
    && tar xf arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz -C /opt \
    && rm arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz \
    && ln -s /opt/arm-gnu-toolchain-13.2.Rel1-x86_64-arm-none-eabi /opt/arm-toolchain

# Add toolchain to PATH
ENV PATH="/opt/arm-toolchain/bin:${PATH}"

# Get STM32CubeF4 v1.28.3 (only clone what we need to save space)
RUN git clone --depth 1 --branch v1.28.3 --single-branch \
    https://github.com/STMicroelectronics/STM32CubeF4 /opt/STM32CubeF4 && \
    cd /opt/STM32CubeF4 && \
    git submodule update --init --depth 1 Drivers/STM32F4xx_HAL_Driver Drivers/CMSIS Middlewares/Third_Party/FreeRTOS

WORKDIR /project
