# XMODEM Firmware Update Support

This directory contains XMODEM protocol implementation for firmware updates on u-connectXpress modules.

## Files

- **inc/u_cx_xmodem.h** / **src/u_cx_xmodem.c**: XMODEM protocol implementation

## Features

- ✅ XMODEM-CRC with CRC16 error checking
- ✅ Support for 128-byte and 1K block sizes (XMODEM-1K)
- ✅ Automatic retry on transmission errors
- ✅ Progress callbacks for UI integration
- ✅ Configurable UART device and baud rate
- ✅ File transfer or callback-based data transfer
- ✅ Platform-independent UART abstraction (u_port)

## Usage Example

### Basic Firmware Update

```c
#include "u_cx_xmodem.h"

// Progress callback (optional)
void progressCallback(size_t totalBytes, size_t bytesTransferred, void *pUserData) {
    int percent = (int)((bytesTransferred * 100) / totalBytes);
    printf("\rFirmware update: %d%% (%zu/%zu bytes)", percent, bytesTransferred, totalBytes);
    fflush(stdout);
}

int main() {
    const char *comPort = "COM31";  // Windows
    // const char *comPort = "/dev/ttyUSB0";  // Linux
    
    // Step 1: Initialize XMODEM configuration
    uCxXmodemConfig_t xmodemConfig;
    uCxXmodemInit(comPort, &xmodemConfig);
    xmodemConfig.use1K = true;  // Use 1K blocks for faster transfer
    
    // Step 2: Open UART at desired baud rate (921600 for fast transfer)
    int32_t result = uCxXmodemOpen(&xmodemConfig, 921600, false);
    if (result != 0) {
        printf("ERROR: Failed to open UART (error %d)\n", result);
        return -1;
    }
    
    // Step 3: Send firmware file
    result = uCxXmodemSendFile(&xmodemConfig, "firmware_v3.2.0.bin",
                               progressCallback, NULL);
    
    // Step 4: Close UART
    uCxXmodemClose(&xmodemConfig);
    
    if (result == 0) {
        printf("\n✓ Firmware transferred successfully!\n");
    } else {
        printf("\n✗ Firmware transfer failed: %d\n", result);
    }
    
    return (result == 0) ? 0 : -1;
}
```

### Complete Firmware Update Workflow

```c
#include "u_cx_at_client.h"
#include "u_cx_system.h"
#include "u_cx_xmodem.h"

int main() {
    const char *comPort = "COM31";
    uCxAtClient_t atClient;
    uCxHandle_t ucxHandle;
    
    // Step 1: Connect to module via AT interface
    int32_t result = uCxAtClientInit(comPort, &atClient);
    result = uCxAtClientOpen(&atClient, 115200, false);
    
    // Initialize UCX handle
    ucxHandle.pAtClient = atClient.pAtClient;
    
    // Use UCX API to enter firmware update mode
    // Module will respond OK and wait for XMODEM transfer
    result = uCxSystemStartSerialFirmwareUpdate2(&ucxHandle, 921600, 0);
    // Note: Command may timeout as module switches to XMODEM mode
    
    // Step 2: Close AT client (releases UART)
    uCxAtClientClose(&atClient);
    Sleep(2000);  // Wait for module to switch modes
    
    // Step 3: Open UART for XMODEM and transfer firmware
    uCxXmodemConfig_t xmodemConfig;
    uCxXmodemInit(comPort, &xmodemConfig);
    xmodemConfig.use1K = true;
    
    result = uCxXmodemOpen(&xmodemConfig, 921600, false);
    if (result == 0) {
        result = uCxXmodemSendFile(&xmodemConfig, "firmware.bin",
                                   progressCallback, NULL);
        uCxXmodemClose(&xmodemConfig);
    }
    
    // Step 4: Wait for module reboot (5 seconds)
    Sleep(5000);
    
    // Step 5: Reconnect AT client
    result = uCxAtClientOpen(&atClient, 115200, false);
    
    return 0;
}
```

### Advanced Usage with Callback-Based Data Transfer

For streaming or custom data sources, use the callback-based API:

```c
#include "u_cx_xmodem.h"
#include <stdio.h>
#include <stdlib.h>

// Custom data source
uint8_t *firmwareData = NULL;
size_t firmwareSize = 0;

// Data callback - called by XMODEM to get each block
int32_t dataCallback(uint8_t *pBuffer, size_t offset, size_t maxLen, void *pUserData) {
    if (offset >= firmwareSize) {
        return 0;  // End of data
    }
    
    size_t remaining = firmwareSize - offset;
    size_t toRead = (remaining < maxLen) ? remaining : maxLen;
    
    memcpy(pBuffer, firmwareData + offset, toRead);
    return (int32_t)toRead;
}

int main() {
    // Load firmware into memory
    FILE *fp = fopen("firmware.bin", "rb");
    fseek(fp, 0, SEEK_END);
    firmwareSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    firmwareData = malloc(firmwareSize);
    fread(firmwareData, 1, firmwareSize, fp);
    fclose(fp);
    
    // Initialize XMODEM
    uCxXmodemConfig_t xmodemConfig;
    uCxXmodemInit("COM31", &xmodemConfig);
    xmodemConfig.use1K = true;
    
    // Open UART
    int32_t result = uCxXmodemOpen(&xmodemConfig, 921600, false);
    if (result == 0) {
        // Send using callback
        result = uCxXmodemSend(&xmodemConfig, firmwareSize,
                               dataCallback, progressCallback, NULL);
        uCxXmodemClose(&xmodemConfig);
    }
    
    free(firmwareData);
    return (result == 0) ? 0 : -1;
}
```

### XMODEM API Reference

The XMODEM implementation provides these key functions:

```c
// Initialize configuration with default values
void uCxXmodemInit(const char *pUartDevName, uCxXmodemConfig_t *pConfig);

// Open UART connection for transfer
int32_t uCxXmodemOpen(uCxXmodemConfig_t *pConfig, int32_t baudRate, bool flowControl);

// Close UART connection
void uCxXmodemClose(uCxXmodemConfig_t *pConfig);

// Send data using callback to retrieve blocks
int32_t uCxXmodemSend(uCxXmodemConfig_t *pConfig,
                      size_t dataLen,
                      uCxXmodemDataCallback_t dataCallback,
                      uCxXmodemProgressCallback_t progressCallback,
                      void *pUserData);

// Send file directly (convenience function)
int32_t uCxXmodemSendFile(uCxXmodemConfig_t *pConfig,
                          const char *pFilePath,
                          uCxXmodemProgressCallback_t progressCallback,
                          void *pUserData);
```

**Configuration Structure:**

```c
typedef struct {
    const char *pUartDevName;      // UART device (e.g., "COM31", "/dev/ttyUSB0")
    uPortUartHandle_t uartHandle;  // Internal UART handle (managed internally)
    bool use1K;                    // Use 1K blocks (XMODEM-1K) instead of 128-byte
    int32_t timeoutMs;             // Timeout for ACK/NAK (default: 15000ms)
    int32_t instance;              // Instance number for logging
    volatile bool opened;          // UART opened state (internal)
} uCxXmodemConfig_t;
```

**Callbacks:**

```c
// Data retrieval callback - return bytes read (0=end, <0=error)
typedef int32_t (*uCxXmodemDataCallback_t)(uint8_t *pBuffer, 
                                            size_t offset, 
                                            size_t maxLen, 
                                            void *pUserData);

// Progress callback
typedef void (*uCxXmodemProgressCallback_t)(size_t totalBytes, 
                                             size_t bytesTransferred, 
                                             void *pUserData);
```

## Protocol Details

### XMODEM-CRC Implementation

- Uses **CRC16-CCITT** polynomial 0x1021
- Supports both **128-byte** (original XMODEM) and **1K** blocks (XMODEM-1K)
- Waits for receiver to initiate with 'C' character (CRC mode)
- Automatic retransmission on NAK or timeout
- Graceful cancellation on CAN character
- EOT sent after last block with ACK confirmation

### Block Format

```
+-----+----------+----------+--------+------+
| SOH | Block #  | ~Block # |  Data  | CRC  |
+-----+----------+----------+--------+------+
  1B      1B         1B       128/1KB   2B
```

- **SOH** (0x01): 128-byte block header
- **STX** (0x02): 1K block header
- **Block #**: Sequential block number (wraps at 256)
- **~Block #**: Complement of block number for error detection
- **Data**: Actual data, padded with 0x1A (EOF) if needed
- **CRC**: CRC16-CCITT checksum (big-endian)

### Transfer Flow

```
Receiver              Sender
   |                    |
   |-------- C -------->|  (Request CRC mode)
   |                    |
   |<--- SOH+Data+CRC --|  (Send block 1)
   |                    |
   |-------- ACK ------>|  (Acknowledge)
   |                    |
   |<--- SOH+Data+CRC --|  (Send block 2)
   |                    |
   |-------- ACK ------>|
   |                    |
   |       ...          |
   |                    |
   |<------ EOT --------|  (End of transmission)
   |                    |
   |-------- ACK ------>|
```

## Error Handling

| Error Code | Description |
|------------|-------------|
| 0 | Success |
| -1 | General error (timeout, write failure, max retries exceeded) |
| -2 | Receiver requested checksum mode (not supported) |
| -3 | Transfer cancelled by receiver |

## Performance Tips

1. **Use fast baud rates**: 460800 or 921600 baud significantly reduces transfer time
2. **Use 1K blocks**: Set `xmodemConfig.use1K = true` for ~8x faster transfers than 128-byte blocks
3. **Disable flow control**: For direct USB connections, flow control can often be disabled (`false`)
4. **Pre-load data**: For callback-based transfers, keep data in memory to avoid disk I/O delays

## Baudrate vs. Transfer Time

For a 1 MB firmware file:

| Baudrate | Block Size | Approximate Time |
|----------|------------|------------------|
| 115200   | 128 bytes  | ~18 minutes |
| 115200   | 1K bytes   | ~2.5 minutes |
| 921600   | 128 bytes  | ~2.5 minutes |
| 921600   | 1K bytes   | ~20 seconds |

## Troubleshooting

### "Timeout waiting for start signal"
- Ensure module is in firmware update mode (use `uCxSystemStartSerialFirmwareUpdate2()`) and ready to receive
- Verify UART device name is correct (e.g., "COM31" on Windows, "/dev/ttyUSB0" on Linux)
- Check that AT client connection was properly closed before opening XMODEM
- Wait 2-3 seconds after entering firmware update mode for module to stabilize

### "Failed to send block X"
- Check serial connection quality
- Reduce baud rate if seeing frequent retries (try 115200 instead of 921600)
- Verify UART device is not in use by another process
- On Linux, ensure user has permissions to access serial device (`sudo usermod -a -G dialout $USER`)

### "UART open failed"
- Check that COM port / device exists and is not in use
- On Windows: Verify port number in Device Manager
- On Linux: Check `ls /dev/ttyUSB*` or `ls /dev/ttyACM*`
- Ensure previous AT client connection was properly closed

### "No ACK after EOT"
- Transfer may have succeeded despite error - check module status after reboot
- Module may be applying firmware - wait 5-10 seconds before reconnecting
- Verify firmware file integrity (correct .bin file for your module)

### "Permission denied" (Linux)
- Add user to dialout group: `sudo usermod -a -G dialout $USER`
- Logout and login again for group changes to take effect
- Or run with sudo (not recommended for production)

## Integration Examples

### Windows GUI Integration

```c
// Windows example with progress bar
#include <windows.h>
#include "u_cx_xmodem.h"

HWND hProgressBar;  // Progress bar control

void CALLBACK ProgressCallback(size_t totalBytes, size_t bytesTransferred, void *pUserData) {
    int percent = (int)((bytesTransferred * 100) / totalBytes);
    SendMessage(hProgressBar, PBM_SETPOS, (WPARAM)percent, 0);
}

// In button click handler:
void OnUpdateFirmwareClick() {
    uCxXmodemConfig_t xmodemConfig;
    uCxXmodemInit("COM31", &xmodemConfig);
    xmodemConfig.use1K = true;
    
    if (uCxXmodemOpen(&xmodemConfig, 921600, false) == 0) {
        uCxXmodemSendFile(&xmodemConfig, "firmware.bin", ProgressCallback, NULL);
        uCxXmodemClose(&xmodemConfig);
    }
}
```

### Python Integration (using ctypes)

```python
import ctypes
import os

# Load library
lib = ctypes.CDLL("ucxclient.dll")  # or .so on Linux

# Define callback types
ProgressCallbackType = ctypes.CFUNCTYPE(None, ctypes.c_size_t, ctypes.c_size_t, ctypes.c_void_p)

def progress_callback(total, transferred, userdata):
    percent = (transferred * 100) // total
    print(f"\rProgress: {percent}% ({transferred}/{total} bytes)", end='', flush=True)

# Create callback instance
progress_cb = ProgressCallbackType(progress_callback)

# Initialize and send
class XmodemConfig(ctypes.Structure):
    _fields_ = [("pUartDevName", ctypes.c_char_p),
                ("uartHandle", ctypes.c_void_p),
                ("use1K", ctypes.c_bool),
                ("timeoutMs", ctypes.c_int32),
                ("instance", ctypes.c_int32),
                ("opened", ctypes.c_bool)]

config = XmodemConfig()
lib.uCxXmodemInit(b"COM31", ctypes.byref(config))
config.use1K = True

if lib.uCxXmodemOpen(ctypes.byref(config), 921600, False) == 0:
    result = lib.uCxXmodemSendFile(ctypes.byref(config), b"firmware.bin",
                                    progress_cb, None)
    lib.uCxXmodemClose(ctypes.byref(config))
    print("\nDone!" if result == 0 else f"\nFailed: {result}")
```

## Module Support

This implementation is designed for u-connectXpress modules including:
- NORA-W36 (Wi-Fi + Bluetooth)
- NORA-B26 (Bluetooth only)

Consult your module's documentation for specific firmware update procedures and any required AT commands before initiating XMODEM transfer.

## License

Copyright 2024 u-blox

Licensed under the Apache License, Version 2.0
