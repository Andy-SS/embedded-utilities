# UART DMA + Ring Buffer + eLog Integration (Stable Architecture)

## Overview

This document describes the **stable, proven pattern** for retargeting `stdout` (printf, eLog console subscriber) through a ring buffer and UART DMA on embedded STM32 platforms.

## The Problem We Solved

When mixing **UART DMA TX**, **ring buffer critical sections**, and **RTOS mutexes**, three failure modes were common:

1. **"Prints ~10 chars then stops"** — TX engine state (`LPUartReady`) was racy between thread and ISR contexts, causing missed "kicks" to start next DMA chunk.
2. **"System hangs after early logging"** — Calling `ring_read_multiple()` (which may acquire a mutex) while `PRIMASK=1` (interrupts disabled) deadlocks the system.
3. **"Output corruption or DMA never completes"** — System entering low-power STOP mode during UART DMA disabled the LPUART clock, causing TX to stall.

## Solution: Safe Critical Section Ordering

### ✅ **The Safe Pattern** (What Works)

```c
/* rsIO.c: LPUartQueueBuffWrite() - thread-safe TX kick */
int LPUartQueueBuffWrite(int handle, const char *buf, size_t bufSize)
{
  size_t chars_written = 0;
  bool start_tx = false;

  // Step 1: Write data into ring (always with interrupts ENABLED)
  chars_written = ring_write_multiple(&LPUartTxRing, (uint8_t *)buf, bufSize);

  // Step 2: Try to claim TX engine (very short IRQ-off window, NO ring ops!)
  BACKUP_PRIMASK();
  DISABLE_IRQ();
  if (LPUartReady == IDLE)
  {
    LPUartReady = BUSY;
    start_tx = true;
  }
  RESTORE_PRIMASK();  // <-- Always restore immediately

  // Step 3: If we claimed it, read from ring and start DMA (interrupts ENABLED!)
  if (start_tx)
  {
    uint32_t size = 0;
    memset(locale_tx_buffer, 0, L_TX_BUFFER_SIZE);
    size = ring_read_multiple(&LPUartTxRing, locale_tx_buffer, L_TX_BUFFER_SIZE);
    if (size > 0)
    {
      LPUartDMAWrite(locale_tx_buffer, (uint16_t)size);
    }
    else
    {
      /* Nothing to send; release TX engine (brief IRQ-off) */
      BACKUP_PRIMASK();
      DISABLE_IRQ();
      LPUartReady = IDLE;
      RESTORE_PRIMASK();
    }
  }

  return (chars_written);
}
```

### ✅ **DMA Complete Callback** (ISR Context)

```c
static void LPUartTxCpltCallback(UART_HandleTypeDef *huart)
{
  uint8_t buf;
  uint16_t bufSize;

  memset(locale_tx_buffer, 0, L_TX_BUFFER_SIZE);
  bufSize = ring_read_multiple(&LPUartTxRing, locale_tx_buffer, L_TX_BUFFER_SIZE);
  if (bufSize > 0)
  {
    LPUartReady = BUSY;
    LPUartDMAWrite(locale_tx_buffer, bufSize);
  }
  else
  {
    LPUartReady = IDLE;
    /* Re-enable highest power mode now that UART transmission is complete */
    UTIL_LPM_SetMaxMode(1U << CFG_LPM_LOG, UTIL_LPM_STOP1_MODE);
  }
}
```

### ✅ **DMA Write** (Start Transfer + Power Management)

```c
void LPUartDMAWrite(uint8_t *p_data, uint16_t size)
{
  /* CRITICAL: Disable STOP mode to force SLEEP mode during UART DMA transmission
   * Reason: In STOP1 mode, LPUART peripheral clock is gated, causing DMA transfer corruption.
   * Solution: Keep SLEEP mode active (all clocks on) during TX.
   */
  UTIL_LPM_SetMaxMode(1U << CFG_LPM_LOG, UTIL_LPM_SLEEP_MODE);
  
  if (HAL_UART_Transmit_DMA(&hlpuart1, p_data, size) != HAL_OK)
  {
    /* If we failed to start DMA, don't leave TX engine stuck BUSY. */
    LPUartReady = IDLE;
    UTIL_LPM_SetMaxMode(1U << CFG_LPM_LOG, UTIL_LPM_STOP1_MODE);
  }
}
```

## Why This Works

| Problem | Root Cause | Solution |
|---------|-----------|----------|
| **Missed TX kicks** | `LPUartReady` updated unsafely in thread/ISR context | Short atomic IRQ-off window guards state transition only |
| **Deadlock on mutex** | `ring_read_multiple()` called with `PRIMASK=1` | Ring ops always happen with interrupts **enabled** |
| **DMA stall/corruption** | System entered STOP1 during TX, disabling LPUART clock | Force SLEEP mode during `HAL_UART_Transmit_DMA()` |
| **Output stops** | Combination of all three above | All three patterns applied consistently |

## Integration Checklist

- [ ] `App/Platform/rsIO.c` has `LPUartQueueBuffWrite()` with the safe pattern (Ring ops after PRIMASK restore)
- [ ] `App/Platform/rsIO.c` has `LPUartTxCpltCallback()` reading from ring without IRQ-off
- [ ] `App/Platform/rsIO.c` has `LPUartDMAWrite()` calling `UTIL_LPM_SetMaxMode(..., UTIL_LPM_SLEEP_MODE)` before DMA start
- [ ] `App/app_rtos.c` registers ring callbacks via `ring_register_cs_callbacks(&mutex_callbacks)` **after** ThreadX starts
- [ ] `App/app_rtos.c` registers eLog callbacks via `elog_register_mutex_callbacks(&mutex_callbacks)` **after** ThreadX starts
- [ ] Early boot logs (before `MX_ThreadX_Init()`) work without hanging

## Testing

### Early Boot (Before ThreadX)
Ring **won't** have mutex callbacks registered yet — only uses PRIMASK/IRQ-based CS.

```c
int main(void) {
    debug_interface_init(&hlpuart1);
    LOG_INIT_WITH_CONSOLE_AUTO();
    ELOG_INFO(ELOG_MD_DEFAULT, "=== Early Boot Log ===");  // Should print without hanging
    
    MX_ThreadX_Init();
    // ... rest of code ...
}
```

### After ThreadX (During Operation)
Ring **will** have mutex callbacks — adds per-instance mutex protection **on top of** PRIMASK/IRQ.

```c
void tx_application_define(void) {
    // After ThreadX scheduler setup:
    utilities_register_cs_cbs(&mutex_callbacks);
    ring_register_cs_callbacks(&mutex_callbacks);
    elog_register_mutex_callbacks(&mutex_callbacks);
    ELOG_INFO(ELOG_MD_MAIN, "RTOS started - full thread safety enabled");
}
```

## Gotchas to Avoid

### ❌ **DON'T** disable interrupts around ring calls in thread context
```c
// WRONG - will deadlock!
DISABLE_IRQ();
ring_write_multiple(&ring, data, len);
RESTORE_PRIMASK();
```

Reason: If ring tries to take an RTOS mutex, you've deadlocked the system.

### ❌ **DON'T** leave PRIMASK disabled when returning from `_write()`
```c
// WRONG - kills all interrupts permanently!
DISABLE_IRQ();
if (something) {
  // ...
  return (chars_written);  // Oops, interrupts still off!
}
RESTORE_PRIMASK();
```

### ❌ **DON'T** enter low-power STOP mode while UART DMA is active
```c
// WRONG - LPUART clock gets gated, TX stalls
UTIL_LPM_SetMaxMode(..., UTIL_LPM_STOP1_MODE);  // Too early!
HAL_UART_Transmit_DMA(&hlpuart1, ...);
```

## Performance Notes

- **Power consumption**: Stays in SLEEP mode (~1-3 mA) during TX instead of STOP1 (~7-25 µA). Trade-off: Immediate responsiveness over power efficiency during logging.
- **Latency**: DMA kicks immediately; no polling loops.
- **Thread safety**: Per-instance mutexes don't block each other; independent rings can operate in parallel.

## See Also

- [RING.md](RING.md) — Ring buffer API and per-instance mutex design
- [ELOG.md](ELOG.md) — eLog thread safety and subscriber pattern
- [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) — Common utilities setup (mutex callbacks)
