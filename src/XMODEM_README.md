# XMODEM Firmware Update Support

This directory contains XMODEM protocol implementation for firmware updates on u-connectXpress modules.

## Files

- **inc/u_cx_at_xmodem.h** / **src/u_cx_at_xmodem.c**: Low-level XMODEM protocol implementation
- **ucx_api/u_cx_firmware_update.h** / **ucx_api/u_cx_firmware_update.c**: High-level firmware update API

## Features

- ✅ XMODEM with CRC16 error checking
- ✅ Support for 128-byte and 1K block sizes
- ✅ Automatic retry on transmission errors
- ✅ Progress callbacks for UI integration
- ✅ Configurable timeouts and retry limits
- ✅ Simple API: `uCxFirmwareUpdate(handle, filepath, baudrate, callback, userdata)`

## Usage Example

### Simple Firmware Update

```c
#include "u_cx_firmware_update.h"

// Progress callback (optional)
void progressCallback(size_t total, size_t transferred, int32_t percent, void *pUserData) {
    printf("\rFirmware update: %d%% (%zu/%zu bytes)", percent, transferred, total);
    fflush(stdout);
}

int main() {
    uCxHandle_t uCxHandle;
    
    // Initialize and open connection (see examples/http_example.c)
    // ... initialize uCxHandle ...
    
    // Update firmware at 921600 baud for faster transfer
    int32_t result = uCxFirmwareUpdate(&uCxHandle, 
                                       "firmware_v3.2.0.bin",
                                       921600,  // Use fast baudrate (or 0 to keep current)
                                       progressCallback,
                                       NULL);
    
    if (result == 0) {
        printf("\n✓ Firmware updated successfully!\n");
        printf("Module will reboot. Please reconnect.\n");
    } else {
        printf("\n✗ Firmware update failed: %d\n", result);
    }
    
    return 0;
}
```

### Advanced Usage with Pre-loaded Data

```c
#include "u_cx_firmware_update.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Load firmware into memory
    FILE *fp = fopen("firmware.bin", "rb");
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    uint8_t *firmware = malloc(size);
    fread(firmware, 1, size, fp);
    fclose(fp);
    
    // Update from memory buffer
    uCxHandle_t uCxHandle;
    // ... initialize ...
    
    int32_t result = uCxFirmwareUpdateFromData(&uCxHandle,
                                               firmware,
                                               size,
                                               921600,
                                               progressCallback,
                                               NULL);
    
    free(firmware);
    return (result == 0) ? 0 : 1;
}
```

### Low-Level XMODEM API

For custom implementations, you can use the low-level XMODEM API:

```c
#include "u_cx_at_xmodem.h"

// Configure XMODEM
uCxXmodemConfig_t config;
uCxXmodemConfigInit(&config);
config.use1K = true;          // Use 1K blocks (faster)
config.timeoutMs = 3000;      // 3 second timeout per block
config.maxRetries = 10;       // Retry up to 10 times per block

// Send data
int32_t result = uCxXmodemSend(puCxHandle->pAtClient,
                               pDataBuffer,
                               dataSize,
                               &config,
                               progressCallback,
                               pUserData);
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

1. **Use fast baudrates**: 460800 or 921600 baud significantly reduces transfer time
2. **Use 1K blocks**: Set `config.use1K = true` for ~8x faster transfers than 128-byte blocks
3. **Adjust timeouts**: Lower timeout values speed up error recovery but may cause false retries
4. **Flow control**: Ensure CTS/RTS flow control is enabled for reliable high-speed transfers

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
- Ensure module is in firmware update mode and ready to receive
- Check that UART is properly connected and configured
- Verify flow control settings match on both sides

### "Failed to send block X after N retries"
- Check serial connection quality
- Reduce baudrate if seeing frequent retries
- Enable flow control
- Check for electrical interference

### "No ACK after EOT"
- Transfer may have succeeded despite error
- Check module status after reboot
- Verify firmware file integrity

## Integration with GUI

The progress callback makes it easy to integrate with GUI applications:

```python
# Python example using ctypes
def progress_callback(total, transferred, percent, userdata):
    # Update progress bar in GUI
    progress_bar.set_value(percent)
    status_label.set_text(f"{transferred}/{total} bytes ({percent}%)")

# Call from Python
result = lib.uCxFirmwareUpdate(
    handle, 
    b"firmware.bin",
    921600,
    progress_callback,
    None
)
```

## Module Support

This implementation is designed for u-connectXpress modules including:
- NORA-W36 (Wi-Fi 6)
- NINA-W10 series
- ODIN-W2
- Other u-connectXpress based modules

Consult your module's documentation for specific firmware update procedures and any required AT commands before initiating XMODEM transfer.

## License

Copyright 2024 u-blox

Licensed under the Apache License, Version 2.0
