#!/bin/bash
# Build script for STM32F407VG ucxclient example

set -e

# Configuration
BUILD_DIR="build_stm32"
WIFI_SSID="${WIFI_SSID:-ubx}"
WIFI_PSK="${WIFI_PSK:-}"

echo "========================================="
echo "Building ucxclient for STM32F407VG"
echo "========================================="
echo "WiFi SSID: $WIFI_SSID"
echo "WiFi PSK: ${WIFI_PSK:+***}"
echo "Build dir: $BUILD_DIR"
echo ""

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DU_EXAMPLE_SSID="$WIFI_SSID" \
    -DU_EXAMPLE_WPA_PSK="$WIFI_PSK"

# Build
cmake --build . -j$(nproc)

echo ""
echo "========================================="
echo "Build complete!"
echo "========================================="
echo "Output files in: $BUILD_DIR/"
ls -lh *.elf *.hex *.bin *.map 2>/dev/null || true
echo ""
echo "Flash with:"
echo "  st-flash write http_example_stm32.bin 0x08000000"
echo "  or"
echo "  openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \\"
echo "    -c 'program http_example_stm32.elf verify reset exit'"
