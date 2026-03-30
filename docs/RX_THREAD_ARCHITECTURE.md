# ucxclient RX Thread Architecture

## Overview

ucxclient uses a **two-thread architecture** for AT command processing:

1. **Background RX thread** — reads UART, parses AT responses, queues URCs
2. **Application main thread** — dispatches URCs, sends AT commands

## The Problem: cmdMutex Contention

`uCxAtClientHandleRx()` acquires `cmdMutex` to serialize UART access. When both
the background RX thread and the main thread call this function, the main thread
blocks waiting for the RX thread to finish its UART read cycle.

**Blocking chain:**
```
RX thread (tight loop):
  uCxAtClientHandleRx() → LOCK cmdMutex → handleRxData() → ReadFile (1ms timeout) → UNLOCK as

Main thread (app event loop):
  uCxAtClientHandleRx() → BLOCKS on cmdMutex → waits 100-1700ms
```

The RX thread holds `cmdMutex` while calling `handleRxData()`, which loops reading
bytes from UART until a complete AT response or URC line is parsed. Each `ReadFile`
has a 1ms timeout, but the loop continues until a complete line is received. With
the RX thread immediately re-acquiring the lock after releasing it, the main thread
suffers **lock starvation**.

## The Solution: uCxAtClientProcessUrcs()

Since the background RX thread already handles all UART reading and AT parsing,
the main thread only needs to **dispatch queued URCs** — no UART access required.

```c
// OLD: Main thread blocks on cmdMutex (100-1700ms stalls)
uCxAtClientHandleRx(&client);

// NEW: Main thread dispatches URCs without any lock (< 1ms)
uCxAtClientProcessUrcs(&client);
```

`uCxAtClientProcessUrcs()` dequeues entries from the URC queue and calls the
registered URC callback — no mutex, no UART reads, no blocking.

## When to Use What

| Function | Thread | Purpose |
|----------|--------|---------|
| `uCxAtClientHandleRx()` | RX thread only | Read UART, parse AT, queue URCs |
| `uCxAtClientProcessUrcs()` | Main thread | Dispatch queued URCs (lock-free) |

**Rule:** If `uPortBgRxTaskCreate()` starts a background RX thread, the main
thread should call `uCxAtClientProcessUrcs()` instead of `uCxAtClientHandleRx()`.

If no background RX thread exists (bare-metal/no-OS), call `uCxAtClientHandleRx()`
from the main loop as before — it handles both reading and dispatching.

## Performance Impact

| Metric | Before | After |
|--------|--------|-------|
| Main loop iteration (idle) | 100-1700ms | < 1ms |
| SLOW iterations per minute | 20-30 | 0-1 |
| URC dispatch latency | Same | Same (< 1ms cadence) |
| UART throughput | Same | Same (RX thread unchanged) |

## Data Flow

```
UART RX bytes
    ↓
[RX Thread] uCxAtClientHandleRx()
    ├─ LOCK cmdMutex
    ├─ handleRxData() → bufferedReadByte() → uPortUartRead()
    ├─ parseIncomingChar() → detects URC line
    ├─ uCxAtUrcQueueEnqueue() → copies URC to queue (queueMutex)
    ├─ UNLOCK cmdMutex
    └─ processUrcs() → dispatches from queue (dequeueMutex)

[Main Thread] uCxAtClientProcessUrcs()
    └─ processUrcs() → dispatches from queue (dequeueMutex)
        └─ urcCallback() → application handles URC
```

Both threads can dispatch URCs — the URC queue uses its own mutex (`dequeueMutex`)
separate from `cmdMutex`, so there is no contention on URC dispatch.
