# Binary RX Header Corruption Bug — Root Cause & Fix

## Summary

Large binary downloads over `AT+UHTCGBB` (HTTP body fetch, chunked reads)
intermittently corrupted the 2-byte binary length header partway through the
transfer, aborting with a "framing desync" error. The bug was a **stack-local
buffer used across two separate function calls** in the shared AT client
parser (`u_cx_at_client.c`) — not a UART/DMA hardware fault, not a firmware
bug, and not a threading race, even though all three were suspected and
investigated first.

Confirmed hardware-verified fix. Reproduced identically on native Windows
(FTDI/COM48) and STM32 (DMA UART), which is what eventually pointed at the
shared, OS-agnostic parser code instead of either platform's UART driver.

## Symptom

```
+UHTCGBB:0,1  ->  parsed length = 52404 (raw header bytes 0xCC, 0xB4)
"BIN length 52404 exceeds UDP MTU (1460) - ABORTING (framing desync)"
```

- Never observed on small files (213 KB, 1 MB).
- Only observed on sustained large transfers (22 MB), at a different,
  non-deterministic byte offset every run.
- Looked like a timing race or a UART hardware error at first glance.

## Root Cause

`handleBinaryRx()` in `src/u_cx_at_client.c` reads the 2-byte big-endian
binary length header. The **count** of header bytes received so far
(`rxHeaderCount`) was correctly stored in persistent per-client state, but the
**header bytes themselves** were stored in a **stack-local array**:

```c
static int32_t handleBinaryRx(uCxAtClient_t *pClient)
{
    uCxAtBinaryRx_t *pBinRx = &pClient->binaryRx;
    uint8_t lengthBuf[2];   // BUG: fresh, uninitialized, on EVERY call
    if (pBinRx->rxHeaderCount < 2) {
        ...
        readStatus = bufferedReadBulk(pClient, &lengthBuf[pBinRx->rxHeaderCount], readLen, ...);
        if (readStatus < (int32_t)readLen) {
            return ret;   // partial header — lengthBuf[0] is LOST when the function returns
        }
        uint16_t length = (uint16_t)(lengthBuf[0] << 8) | lengthBuf[1];  // garbage if split
```

If both header bytes arrive in the **same** read, this works correctly. But if
they arrive **split across two separate calls** to `handleBinaryRx()` — byte 1
in call *N*, byte 2 in call *N+1* — the function returns early after byte 1,
the stack frame is destroyed, and the next call declares a **brand-new
uninitialized `lengthBuf[2]`**. Only index 1 gets (re)filled (since
`rxHeaderCount == 1` at that point); index 0 is never repopulated, so the
length is computed from **1 real byte + 1 byte of stack garbage**.

This matches the observed corruption exactly: `0xCC` is MSVC's debug-heap
"uninitialized stack" poison-fill byte — direct evidence of a memory-lifetime
bug, not wire noise or a firmware defect.

## Why It's Rate-Dependent and Cross-Platform

A split header read only happens when the two header bytes don't physically
land in the same UART read — far more likely under a tight, zero-delay
`AT+UHTCGBB` loop than a slow/manual test. Because the bug lives in the
shared, OS-agnostic `u_cx_at_client.c`, it reproduces identically regardless
of which UART port driver sits underneath (Windows FTDI vs. STM32 DMA UART).

## What Was Ruled Out First

- STM32 DMA UART hardware RX errors — `HAL_UART_ErrorCallback` (ORE/FE/NE/PE)
  never fired during a failure.
- NORA-W36 firmware `malloc`/`free` churn in the `xUHTCGBB` AT command
  handler — patched firmware to use a static buffer as a test; corruption
  still occurred with stock firmware while raw AT commands on the reference
  test system had **no** corruption, pointing back at the host library.
- Windows `CRITICAL_SECTION` mutex-fairness race between the foreground
  command path and the background RX thread — plausible on Windows, but
  STM32's background RX task uses a gentle 10 ms poll with no equivalent
  tight-spin, and STM32 reproduced the bug too, weakening this theory.
- A 5 ms pacing delay between `AT+UHTCGBB` calls made the symptom "less
  frequent" — a red herring that correctly hinted at rate-dependency but not
  the true mechanism (it just reduced the odds of hitting a split-header
  read window).

## The Fix

1. `inc/u_cx_at_client.h` — added a persistent 2-byte buffer to the
   `uCxAtBinaryRx_t` struct, alongside the existing `rxHeaderCount`:

   ```c
   uint8_t rxHeaderCount;
   uint8_t lengthBuf[2];  // persists partial header bytes across calls to handleBinaryRx()
   ```

2. `src/u_cx_at_client.c` — `handleBinaryRx()` now reads/writes
   `pBinRx->lengthBuf[...]` instead of a local array, so partial header bytes
   correctly survive across separate invocations.

3. Removed the now-unnecessary pacing workaround from the example app's
   download loop (`examples/http_example.c`).

## Verification Status

Hardware-confirmed on real NORA-W36 (COM48). Repeated full runs of all 3 test
files (213 KB / 1 MB / 22 MB) at full speed with **zero corruption** and MD5
match on every file, both with logging fully enabled and fully disabled.

This is a correctness fix, independent of the separate FTDI USB-serial
`LatencyTimer` throughput finding — the two issues are unrelated and both are
now understood/resolved.

## Recommendation

Low-risk, minimal-diff fix (adds 2 bytes to a per-client struct, changes 3
lines to reference the struct field instead of a local array). Any downstream
consumer doing sustained high-throughput binary transfers (HTTP body, socket
reads, etc.) over `AT+U*` binary-response commands is exposed to this bug and
should pick up the fix.
