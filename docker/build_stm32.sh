#!/bin/bash
# Quick build script using Docker Compose for STM32F407VG

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Default values
WIFI_SSID="${WIFI_SSID:-ubx}"
WIFI_PSK="${WIFI_PSK:-}"

echo "========================================="
echo "ucxclient STM32F407VG Docker Build"
echo "========================================="
echo "Project root: $PROJECT_ROOT"
echo "WiFi SSID: $WIFI_SSID"
echo "WiFi PSK: ${WIFI_PSK:+***}"
echo ""

# Build Docker image if needed
echo "Building Docker image (if not already built)..."
cd "$SCRIPT_DIR"
docker compose build

# Run build using Docker Compose
echo ""
echo "Running build in Docker container..."
WIFI_SSID="$WIFI_SSID" WIFI_PSK="$WIFI_PSK" docker compose run --rm stm32f4-builder bash -c "cd stm32 && ./build.sh"

BUILD_STATUS=$?

if [ $BUILD_STATUS -eq 0 ]; then
    echo ""
    echo "========================================="
    echo "Build successful!"
    echo "========================================="
    echo "Output files are in: stm32/build_stm32/"
    echo ""
    echo "Flash to your STM32F407VG with:"
    echo "  st-flash write stm32/build_stm32/http_example_stm32.bin 0x08000000"
    echo ""
    echo "Or use OpenOCD:"
    echo "  openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \\"
    echo "    -c 'program stm32/build_stm32/http_example_stm32.elf verify reset exit'"
else
    echo ""
    echo "========================================="
    echo "Build failed!"
    echo "========================================="
    exit $BUILD_STATUS
fi
