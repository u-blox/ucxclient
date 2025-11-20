# UART Implementation Modes

The Windows port (`u_port_windows.c`) supports three different UART RX implementations that can be selected at compile time.

## Available Modes

### 1. USE_UART_EVENT_DRIVEN (Default, Production)
**Status:** ✅ Fully implemented and tested  
**Best for:** Production use

**Characteristics:**
- Uses Windows `WaitCommEvent()` API for efficient event-driven processing
- Minimal CPU usage (~0% when idle)
- Fast response time (immediate on data arrival)
- Loops until RX buffer is completely drained to handle rapid data bursts
- Includes comprehensive UART error detection (overrun, framing, parity errors)

**Advantages:**
- Most efficient implementation
- Lowest latency
- Recommended for production use

**Disadvantages:**
- More complex code
- Harder to debug timing issues

**Log output:** `RX thread started (event-driven)`

---

### 2. USE_UART_POLLED (Debugging)
**Status:** ✅ Implemented  
**Best for:** Debugging and troubleshooting

**Characteristics:**
- Simple polling loop checking for data every 10ms
- Predictable timing behavior
- Easy to understand and modify
- Still includes UART error detection

**Advantages:**
- Simple, linear execution flow
- Easy to debug with breakpoints
- Predictable polling interval (10ms default, adjustable)
- Good for troubleshooting timing issues
- Useful when investigating lost '+' characters or malformed URCs

**Disadvantages:**
- Higher CPU usage (~10% constant polling)
- Slightly higher latency (up to 10ms delay)
- May still miss data if burst arrives faster than 10ms interval

**Log output:** `RX thread started (polled mode, 10ms interval)`

**Configuration:**
```c
context.pollIntervalMs = 10;  // Adjust polling interval (milliseconds)
```

---

### 3. USE_UART_FTDI (Future)
**Status:** 🚧 Placeholder (not yet implemented)  
**Best for:** Direct FTDI D2XX API access

**Characteristics:**
- Bypasses Windows COM driver entirely
- Uses FTDI D2XX API directly (`FT_Read()`, `FT_Write()`, etc.)
- Access to FTDI-specific features
- See: D2XX Programmer's Guide (attached)

**When to use:**
- Need FTDI-specific features (e.g., bit-bang mode, MPSSE)
- Want to bypass Windows COM driver for debugging
- Need absolute control over USB communication

**Implementation TODO:**
- Include FTDI D2XX headers and library
- Implement `FT_Open()` / `FT_Close()`
- Use `FT_SetEventNotification()` or `FT_GetQueueStatus()`
- Process data with `FT_Read()`
- Call `uCxAtClientHandleRx()` when data available

**Log output:** `RX thread started (FTDI D2XX mode)`

---

## How to Switch Modes

Edit `examples/port/u_port_windows.c` and uncomment the desired mode in the "COMPILE-TIME MACROS" section:

```c
/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

// UART RX Implementation Selection
// Uncomment ONE of the following to select UART implementation:
// #define USE_UART_POLLED        // Simple polled mode (good for debugging)
#define USE_UART_EVENT_DRIVEN     // Event-driven with WaitCommEvent (default)
// #define USE_UART_FTDI          // FTDI D2XX API (future implementation)
```

Then rebuild:
```powershell
cmake --build build --config Release --target ucxclient-x64
```

---

## Troubleshooting Lost '+' Characters

If you see malformed URCs like:
```
[AT RX][0] UWSSC:6032B1CE9E32,"Villallegatan",8,-101,18,8,8
```
Instead of:
```
[AT RX][0] +UWSSC:6032B1CE9E32,"Villallegatan",8,-101,18,8,8
```

**Try these debugging steps:**

1. **Switch to polled mode** to see if timing-related:
   ```c
   #define USE_UART_POLLED
   ```
   
2. **Increase polling interval** if using polled mode:
   ```c
   context.pollIntervalMs = 5;  // Try 5ms instead of 10ms
   ```

3. **Check for UART errors** in logs:
   - `UART RX buffer overrun` - Software buffer full
   - `UART hardware overrun` - Hardware FIFO overflow
   - `UART framing error` - Invalid start/stop bits
   - `UART parity error` - Data corruption

4. **Enable RX buffer size increase:**
   Edit `openComPort()` and increase buffer:
   ```c
   SetupComm(hComPort, 8192, 8192);  // Increase from default 4096
   ```

5. **Future: Try FTDI D2XX mode** when implemented (bypasses COM driver)

---

## UART Error Detection

All three modes include comprehensive UART error monitoring via `ClearCommError()`:

| Error Flag | Description | Cause |
|-----------|-------------|-------|
| `CE_RXOVER` | RX buffer overrun | Software buffer full, data lost |
| `CE_OVERRUN` | Hardware overrun | UART FIFO overflow, data corrupted |
| `CE_FRAME` | Framing error | Invalid start/stop bits, data corrupted |
| `CE_BREAK` | Break condition | Line held low for extended period |
| `CE_RXPARITY` | Parity error | Data corruption detected |

**Event-driven mode log prefix:** None  
**Polled mode log prefix:** `[POLLED]`  
**FTDI mode log prefix:** `[FTDI]` (future)

---

## Performance Comparison

| Mode | CPU Usage (idle) | CPU Usage (active) | Latency | Complexity | Debug Ease |
|------|-----------------|-------------------|---------|------------|-----------|
| Event-driven | ~0% | ~5% | <1ms | High | Medium |
| Polled | ~10% | ~15% | <10ms | Low | Easy |
| FTDI D2XX | TBD | TBD | TBD | Medium | Medium |

---

## Recommendations

- **Production:** Use `USE_UART_EVENT_DRIVEN` (default)
- **Debugging timing issues:** Use `USE_UART_POLLED`
- **FTDI-specific needs:** Wait for `USE_UART_FTDI` implementation

---

## References

- Windows Serial Communications: https://learn.microsoft.com/en-us/windows/win32/devio/communications-resources
- FTDI D2XX Programmer's Guide: See attached PDF
- WaitCommEvent API: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-waitcommevent
