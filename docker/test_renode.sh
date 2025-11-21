#!/bin/bash
# Test script for Renode emulation

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "========================================="
echo "Renode Test for STM32F407VG"
echo "========================================="

# Build the firmware first
cd "$SCRIPT_DIR"
bash build_stm32.sh

echo ""
echo "========================================="
echo "Running Renode Emulation Test"
echo "========================================="

# Create a test script for Renode (headless mode)
mkdir -p "$PROJECT_ROOT/stm32/build_stm32"
cat > "$PROJECT_ROOT/stm32/build_stm32/test_headless.resc" << 'RESC'
# Headless test script for STM32F407VG

# Suppress GUI requirements
emulation SetGlobalQuantum "0.00001"

# Create machine
mach create "stm32f407"
machine LoadPlatformDescription @platforms/boards/stm32f4_discovery-kit.repl

# Create file backend for UART output
emulation CreateUARTHub "uart_hub"
emulation LogToFile @uart_output.txt true "usart2"
connector Connect sysbus.usart2 uart_hub

# Load firmware
sysbus LoadELF @http_example_stm32.elf.elf

# Set logging
logFile @renode.log
logLevel 3

# Run emulation
start

# Run for 5 seconds
emulation RunFor "00:00:05"

# Get some info
mach GetTimeSourceInfo

# Clean exit
quit
RESC

# Run Renode in headless mode
echo "Starting Renode in headless mode..."
docker compose run --rm stm32f4-renode bash -c "
    cd build_stm32 && \
    renode --disable-xwt --console --plain test_headless.resc 2>&1 | tee renode_console.txt
" || true

echo ""
echo "========================================="
echo "Renode Test Complete"
echo "========================================="

# Check if output files were created
if [ -f "$PROJECT_ROOT/stm32/build_stm32/uart_output.txt" ]; then
    echo "✓ UART output captured"
    echo "First 20 lines of UART output:"
    head -20 "$PROJECT_ROOT/stm32/build_stm32/uart_output.txt" || true
else
    echo "⚠ No UART output file found"
fi

if [ -f "$PROJECT_ROOT/stm32/build_stm32/renode.log" ]; then
    echo ""
    echo "✓ Renode log created"
    # Check for errors
    if grep -i "error" "$PROJECT_ROOT/stm32/build_stm32/renode.log"; then
        echo "⚠ Errors found in Renode log"
    fi
else
    echo "⚠ No Renode log file found"
fi

echo ""
echo "Output files:"
echo "  - renode_console.txt: Console output"
echo "  - uart_output.txt: UART2 serial output"
echo "  - renode.log: Renode internal log"
