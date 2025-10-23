# ucxclient Windows GUI

A Windows graphical user interface for the ucxclient library, providing easy interaction with u-blox u-connectXpress modules (NORA-W36).

## Features

- **COM Port Management**: Automatic detection and connection to available COM ports
- **AT Command Terminal**: Direct AT command interface with command history
- **WiFi Management**: Network scanning, connection, and monitoring
- **Socket Testing**: TCP/UDP socket testing with HTTP client functionality
- **Real-time Logging**: Comprehensive logging with save/export functionality

## Prerequisites

### Software Requirements

1. **Python 3.7 or higher** with tkinter support
2. **Visual Studio 2019/2022** (for building the C library)
3. **CMake 3.4 or higher**
4. **Windows 10/11** (recommended)

### Hardware Requirements

1. **NORA-W36 module** connected via USB-to-UART adapter
2. Available COM port (USB-to-UART adapter or built-in serial port)

## Building and Installation

### Step 1: Build the ucxclient Library

First, build the Windows DLL:

```powershell
# Open PowerShell in the ucxclient root directory
cd c:\u-blox\ucxclient

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -G "Visual Studio 17 2022"

# Build the project
cmake --build . --config Release

# Or build with Debug configuration
cmake --build . --config Debug
```

This will create `ucxclient_windows.dll` in the build directory.

### Step 2: Test the Console Application

Before using the GUI, test the console application:

```powershell
# Run the Windows test application
.\Release\windows_test.exe COM3 MyWiFiSSID MyWiFiPassword

# Or without arguments to see usage and available ports
.\Release\windows_test.exe --help
```

### Step 3: Launch the GUI

```powershell
# Navigate to the GUI directory
cd ..\windows_gui

# Launch the GUI application
python launcher.py

# Or run directly
python main_window.py
```

## Usage Guide

### 1. Connection Setup

1. **Select COM Port**: Choose from the dropdown list of available ports
2. **Configure Settings**: Set baud rate (default: 115200) and flow control
3. **Connect**: Click "Connect" to establish communication with the module

### 2. AT Command Terminal

- Enter AT commands in the text field and press Enter or click "Send"
- Use quick command buttons for common operations
- View responses in the scrollable text area
- Command history available with up/down arrow keys

### 3. WiFi Management

- **Scan Networks**: Discover available WiFi networks
- **Connect**: Enter SSID and password to connect to a network
- **Monitor Status**: View connection status and signal strength

### 4. Socket Testing

- **TCP/UDP Testing**: Connect to remote hosts for testing
- **HTTP Client**: Perform HTTP GET requests for web service testing
- **Data Monitoring**: View sent/received data in real-time

### 5. Logging

- All operations are logged with timestamps
- Save logs to file for analysis
- Clear logs when needed

## Troubleshooting

### Common Issues

1. **"ucxclient_windows.dll not found"**
   - Ensure the project is built successfully
   - Check that the DLL exists in the build directory
   - Verify the correct configuration (Debug/Release)

2. **"COM port not available"**
   - Check physical connections to the NORA-W36 module
   - Verify USB-to-UART drivers are installed
   - Ensure no other applications are using the port

3. **"No response from module"**
   - Check power supply to the module
   - Verify correct COM port and baud rate
   - Check cable connections and pinout

4. **Python Import Errors**
   - Ensure Python 3.7+ is installed
   - Verify tkinter is available (`python -m tkinter`)
   - Check that all files are in the correct directories

### Debug Mode

To enable debug logging, modify the CMake build:

```powershell
cmake --build . --config Debug -DU_CX_LOG_DEBUG=1
```

### COM Port Detection

To manually check available COM ports:

```python
python -c "from ucx_wrapper import get_available_com_ports; print(get_available_com_ports())"
```

## Development

### Project Structure

```
windows_gui/
├── launcher.py          # Application launcher with requirements checking
├── main_window.py       # Main GUI application
├── ucx_wrapper.py       # Python wrapper for ucxclient DLL
├── requirements.txt     # Python dependencies
├── README.md           # This file
└── widgets/            # Future: Additional GUI widgets
    ├── wifi_widget.py
    ├── socket_widget.py
    └── terminal_widget.py
```

### Adding New Features

1. **New Widgets**: Add to the `widgets/` directory
2. **New API Functions**: Extend `ucx_wrapper.py`
3. **New GUI Tabs**: Add to `main_window.py`

### Testing

Test individual components:

```powershell
# Test the wrapper
python ucx_wrapper.py

# Test GUI without hardware
python main_window.py
```

## API Reference

### UcxClientWrapper Class

Main interface to the ucxclient library:

- `enumerate_com_ports()`: List available COM ports
- `is_com_port_available(port)`: Check if a port is available
- `connect(port, baud_rate, flow_control)`: Connect to COM port
- `disconnect()`: Disconnect from COM port
- `send_at_command(command)`: Send AT command and get response

### Callback System

Register callbacks for events:

```python
wrapper.register_callback('network_up', lambda: print("Network connected"))
wrapper.register_callback('network_down', lambda: print("Network disconnected"))
```

## Known Limitations

1. **Single Instance**: Only one ucxclient connection supported at a time
2. **Windows Only**: This implementation is Windows-specific
3. **Module Support**: Currently optimized for NORA-W36, other modules may need adjustments
4. **Threading**: Some operations may block the GUI temporarily

## License

Copyright © u-blox

Licensed under the Apache License, Version 2.0. See the LICENSE.txt file in the root directory for details.

## Support

For issues related to:
- **ucxclient library**: Check the main project documentation
- **GUI application**: Create an issue with detailed error messages and logs
- **NORA-W36 module**: Consult u-blox documentation and support

## Version History

- **v1.0**: Initial Windows GUI implementation
  - Basic COM port connectivity
  - AT command terminal
  - WiFi management framework
  - Socket testing framework
  - Logging system