# XMODEM and RX Thread Management

## Problem Summary

When performing firmware updates via XMODEM, modules were rebooting at blocks 12-13 with ERROR:46. Root cause analysis revealed:

1. **Background RX threads** in port implementations continuously call `uCxAtClientHandleRx()`
2. This **consumes XMODEM ACK bytes** (0x06) before XMODEM code can read them
3. XMODEM times out (15 seconds) and **retries blocks**
4. Module interprets duplicate blocks as errors and **reboots**

## Solution: Pause/Resume API

Added platform-specific pause/resume functions to give raw binary protocols (XMODEM, etc.) exclusive serial port access:

```c
void uPortAtPauseRx(uCxAtClient_t *pClient);   // Stop RX thread
void uPortAtResumeRx(uCxAtClient_t *pClient);  // Restart RX thread
```

### Usage in Firmware Update

```c
// In u_cx_firmware_update.c
uPortAtFlush(puCxHandle->pAtClient);
uPortAtPauseRx(puCxHandle->pAtClient);           // ← Stop AT parser
result = uCxAtClientXmodemSendFile(...);         // Transfer with exclusive access
uPortAtResumeRx(puCxHandle->pAtClient);          // ← Restart AT parser

if (result != 0) {
    return result;
}
```

## Why NOT Modify uCxAtClientHandleRx()?

| Approach | Pros | Cons |
|----------|------|------|
| **Modify AT client** | Single point of change | ❌ Architecture violation<br>❌ AT layer knows about XMODEM<br>❌ Complex state machine<br>❌ Performance overhead on every byte |
| **Pause/Resume (✅)** | Clean separation of concerns<br>Explicit control<br>Zero overhead when not used<br>Platform-specific optimization | Requires port layer changes |

## Implementation Status

### ✅ Implemented (Working)
- **Windows (u_port_windows.c)** - Tested successfully with CreateThread pause/resume
- **POSIX/Linux (u_port_posix.c)** - Added pthread pause/resume (ready for testing)
- **No-OS (u_port_no_os.c)** - No-op stubs (no threading, already works)

### ⚠️ Pending Implementation
- **FreeRTOS (u_port_freertos.c)** - Needs task suspend/resume or delete/recreate
- **Zephyr (u_port_zephyr.c)** - Needs workqueue or thread pause

## Port Implementation Guidelines

### For Threaded Ports (Windows, POSIX, FreeRTOS, Zephyr)

**Requirements:**
1. Stop the background RX thread/task
2. Wait for thread to fully terminate (join/wait)
3. Keep serial port handle open
4. Allow recreation of thread after XMODEM completes

**Example (pthread-based):**
```c
void uPortAtPauseRx(uCxAtClient_t *pClient) {
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    
    if (pCtx != NULL && !pCtx->terminateRxTask) {
        pCtx->terminateRxTask = true;    // Signal thread
        pthread_join(pCtx->rxThread, NULL);  // Wait for exit
    }
}

void uPortAtResumeRx(uCxAtClient_t *pClient) {
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    
    if (pCtx != NULL && pCtx->terminateRxTask) {
        pCtx->terminateRxTask = false;
        pthread_create(&pCtx->rxThread, &attr, rxTask, pCtx);
    }
}
```

**Example (FreeRTOS):**
```c
void uPortAtPauseRx(uCxAtClient_t *pClient) {
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    
    if (pCtx != NULL && pCtx->rxTaskHandle != NULL) {
        pCtx->terminateRxTask = true;
        // Wait for task to self-delete or use vTaskDelete()
        while (pCtx->rxTaskHandle != NULL) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void uPortAtResumeRx(uCxAtClient_t *pClient) {
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    
    if (pCtx != NULL && pCtx->rxTaskHandle == NULL) {
        pCtx->terminateRxTask = false;
        xTaskCreate(rxTask, "ucxRx", 2048, pCtx, 
                    configMAX_PRIORITIES - 2, &pCtx->rxTaskHandle);
    }
}
```

### For Non-Threaded Ports (No-OS, Bare Metal)

**Implementation:** No-op stubs (application controls when `uCxAtClientHandleRx()` is called)

```c
void uPortAtPauseRx(uCxAtClient_t *pClient) {
    (void)pClient;  // No background thread - nothing to pause
}

void uPortAtResumeRx(uCxAtClient_t *pClient) {
    (void)pClient;  // No background thread - nothing to resume
}
```

## Architecture Benefits

✅ **Separation of Concerns**
- Port layer: Threading management
- AT client: Command parsing
- XMODEM: Binary protocol
- Firmware update: High-level orchestration

✅ **Explicit Control**
- Clear pause/resume boundaries
- No hidden state changes
- Easy to debug and trace

✅ **Zero Overhead**
- No performance impact when not in use
- No extra checks in hot path

✅ **Platform Flexibility**
- Each OS can implement optimally
- Bare metal ports need no changes

## Testing Checklist

When implementing pause/resume for a new port:

- [ ] Pause stops RX thread completely (verify with debugger)
- [ ] Serial port remains open during pause
- [ ] XMODEM can read directly from serial port
- [ ] Resume recreates RX thread successfully
- [ ] AT commands work normally after resume
- [ ] Multiple pause/resume cycles work (e.g., retry scenarios)
- [ ] Thread cleanup on port close works correctly

## References

- **Root Cause Analysis:** Serial monitor showed 15-second gaps between block retries (matching XMODEM timeout)
- **Successful Test:** Windows implementation transferred all 1352 blocks without errors
- **Module Behavior:** ERROR:46 = timeout during firmware update due to duplicate blocks
- **XMODEM Spec:** ACK (0x06) must be received within timeout or block is retried

## Next Steps

1. **Implement pause/resume for FreeRTOS port** (most common embedded OS)
2. **Implement pause/resume for Zephyr port**
3. **Add unit tests** to verify thread pause/resume cycles
4. **Document in port README** files for developers
5. **Consider adding to port template** for new platforms

---

**Conclusion:** The pause/resume approach provides a clean, maintainable solution that respects the layered architecture while solving the byte consumption problem across all platforms.
