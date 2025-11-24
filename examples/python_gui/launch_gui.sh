#!/bin/bash
# Launch script for ucxclient GUI on Linux/macOS

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Check for Python 3
if command -v python3 &> /dev/null; then
    PYTHON_CMD="python3"
elif command -v python &> /dev/null; then
    # Check if it's Python 3
    PYTHON_VERSION=$(python --version 2>&1 | grep -oP '\d+\.\d+' | head -1)
    MAJOR_VERSION=$(echo $PYTHON_VERSION | cut -d. -f1)
    if [ "$MAJOR_VERSION" -ge 3 ]; then
        PYTHON_CMD="python"
    else
        echo "Error: Python 3.7 or higher is required"
        exit 1
    fi
else
    echo "Error: Python is not installed"
    exit 1
fi

# Check if the shared library exists
LIB_NAME=""
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    LIB_NAME="libucxclient.so"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    LIB_NAME="libucxclient.dylib"
else
    LIB_NAME="libucxclient.so"
fi

LIB_FOUND=0
for BUILD_DIR in "$SCRIPT_DIR/build" "$SCRIPT_DIR/build/Debug" "$SCRIPT_DIR/build/Release"; do
    if [ -f "$BUILD_DIR/$LIB_NAME" ]; then
        LIB_FOUND=1
        break
    fi
done

if [ $LIB_FOUND -eq 0 ]; then
    echo "Error: $LIB_NAME not found. Please build the project first:"
    echo "  mkdir -p build && cd build"
    echo "  cmake .."
    echo "  cmake --build ."
    exit 1
fi

# Check for required Python modules
$PYTHON_CMD -c "import tkinter" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "Error: tkinter is not installed"
    echo "On Ubuntu/Debian, install with: sudo apt-get install python3-tk"
    echo "On Fedora/CentOS, install with: sudo dnf install python3-tkinter"
    echo "On macOS, tkinter should be included with Python"
    exit 1
fi

$PYTHON_CMD -c "import serial" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "Error: pyserial is not installed"
    echo "Install with: pip3 install pyserial"
    exit 1
fi

# Launch the GUI
cd "$SCRIPT_DIR/examples/python_gui"
$PYTHON_CMD launcher.py

exit $?
