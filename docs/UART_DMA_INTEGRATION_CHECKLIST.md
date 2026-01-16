# UART DMA + Ring Buffer Integration Checklist

## 🎯 Project: RSSensorV35 Firmware

This checklist verifies that your firmware correctly integrates stdout (printf/eLog) over UART DMA using a ring buffer with safe critical sections.

---

## Phase 1: Hardware & Driver Setup

### UART Configuration
- [ ] LPUART1 initialized with correct baud rate (typically 115200)
- [ ] DMA channel assigned for TX (e.g., LPUART1_TX via DMA)
- [ ] DMA configured for circular/normal mode (standard TX)
- [ ] UART interrupt enabled (`NVIC_SetPriority`, `HAL_NVIC_EnableIRQ`)

### Clocks & Power
- [ ] LPUART1 clock enabled (`__HAL_RCC_LPUART1_CLK_ENABLE()`)
- [ ] DMA clock enabled (automatic via HAL)
- [ ] System clock stable before first UART operation

---

## Phase 2: Ring Buffer Setup (rsIO.c)

### Initialization
- [ ] Ring buffer declared: `static ring_t LPUartTxRing;`
- [ ] Backing buffer declared: `static uint8_t LPUTxBuff[TX_IO_BUFF_SIZE];`
- [ ] `ring_init()` called in `debug_interface_init()` with correct size/element_size
- [ ] `setvbuf(stdout, NULL, _IONBF, 0)` sets unbuffered output

### State Machine
- [ ] `LPUartReady` enum defined (IDLE, BUSY)
- [ ] `locale_tx_buffer[]` staging buffer declared (typically 64 bytes)
- [ ] `HAL_UART_Transmit_DMA()` called only when `LPUartReady == IDLE`

---

## Phase 3: Critical Section Safety (The Key Pattern)

### `LPUartQueueBuffWrite()` - Safe TX Kick

- [ ] **Step 1**: `ring_write_multiple()` called with interrupts **ENABLED**
  ```c
  chars_written = ring_write_multiple(&LPUartTxRing, (uint8_t *)buf, bufSize);
  ```

- [ ] **Step 2**: TX engine claim is atomic with **very short IRQ-off**
  ```c
  BACKUP_PRIMASK();
  DISABLE_IRQ();
  if (LPUartReady == IDLE) {
    LPUartReady = BUSY;
    start_tx = true;
  }
  RESTORE_PRIMASK();  // ← Immediately restore
  ```

- [ ] **Step 3**: Ring read and DMA start with interrupts **ENABLED**
  ```c
  if (start_tx) {
    size = ring_read_multiple(&LPUartTxRing, locale_tx_buffer, L_TX_BUFFER_SIZE);
    if (size > 0) {
      LPUartDMAWrite(locale_tx_buffer, size);  // Interrupts still ON
    } else {
      // Release TX engine (another brief IRQ-off)
      BACKUP_PRIMASK();
      DISABLE_IRQ();
      LPUartReady = IDLE;
      RESTORE_PRIMASK();
    }
  }
  ```

- [ ] **CRITICAL**: No `RESTORE_PRIMASK()` is missed on any code path
- [ ] **CRITICAL**: No ring operations occur while `PRIMASK=1`

### `LPUartTxCpltCallback()` - ISR Kick

- [ ] DMA completion callback defined (weak override of `HAL_UART_TxCpltCallback`)
- [ ] Callback reads from ring: `ring_read_multiple(&LPUartTxRing, locale_tx_buffer, ...)`
- [ ] **NO** `DISABLE_IRQ()` / `BACKUP_PRIMASK()` around ring ops (we're already in ISR!)
- [ ] If more data, sets `LPUartReady = BUSY` and kicks DMA
- [ ] If empty, sets `LPUartReady = IDLE` and re-enables low-power mode

### Power Management (`LPUartDMAWrite()`)

- [ ] Before `HAL_UART_Transmit_DMA()`: Force SLEEP mode
  ```c
  UTIL_LPM_SetMaxMode(1U << CFG_LPM_LOG, UTIL_LPM_SLEEP_MODE);
  ```

- [ ] If `HAL_UART_Transmit_DMA()` fails (`!= HAL_OK`):
  ```c
  if (HAL_UART_Transmit_DMA(...) != HAL_OK) {
    LPUartReady = IDLE;  // Release TX engine
    UTIL_LPM_SetMaxMode(1U << CFG_LPM_LOG, UTIL_LPM_STOP1_MODE);  // Back to STOP mode
  }
  ```

- [ ] When TX completes (in callback), re-enable STOP mode:
  ```c
  UTIL_LPM_SetMaxMode(1U << CFG_LPM_LOG, UTIL_LPM_STOP1_MODE);
  ```

---

## Phase 4: Ring Buffer Mutex Callbacks (After ThreadX)

### In `app_rtos.c` (or equivalent RTOS init):

After ThreadX scheduler setup, before any RTOS-aware logging:

- [ ] Ring mutex callbacks registered:
  ```c
  extern const mutex_callbacks_t mutex_callbacks;
  ring_register_cs_callbacks(&mutex_callbacks);
  ```

- [ ] eLog mutex callbacks registered:
  ```c
  elog_register_mutex_callbacks(&mutex_callbacks);
  ```

- [ ] Utilities marked RTOS-ready:
  ```c
  utilities_set_RTOS_ready(true);
  ```

- [ ] Confirm that these are called **after** ThreadX scheduler setup
- [ ] Confirm `utilities_is_RTOS_ready()` would return `true` after this point

---

## Phase 5: Early Boot Logging (Before ThreadX)

### In `app.c` (or main):

- [ ] `debug_interface_init(&hlpuart1)` called early
- [ ] `LOG_INIT_WITH_CONSOLE_AUTO()` called early
- [ ] Early `printf()` or `ELOG_*` calls **do not hang**
  - Even though ring doesn't have mutex callbacks yet, PRIMASK-based critical sections work
- [ ] First ~100+ characters print without stopping
- [ ] No "prints 10 chars then stops" behavior

---

## Phase 6: Post-RTOS Logging (After ThreadX Starts)

### During RTOS operation:

- [ ] Logging from multiple threads is thread-safe (no corruption)
- [ ] Output is continuous; nothing stops after a few characters
- [ ] Ring buffer use count stays reasonable (not stuck BUSY)
- [ ] System remains responsive (DMA is non-blocking)

---

## Phase 7: Test Cases

### Test 1: Early Boot Multiple Logs
```c
int main(void) {
    debug_interface_init(&hlpuart1);
    LOG_INIT_WITH_CONSOLE_AUTO();
    
    // Should all print without hanging
    ELOG_INFO(ELOG_MD_DEFAULT, "Boot line 1");
    ELOG_INFO(ELOG_MD_DEFAULT, "Boot line 2");
    ELOG_INFO(ELOG_MD_DEFAULT, "Boot line 3");
    
    printf("Early printf 1\r\n");
    printf("Early printf 2\r\n");
    
    MX_ThreadX_Init();  // Should not hang
}
```

**Expected**: All lines print; system does not hang.

### Test 2: Long Early Boot Message
```c
ELOG_INFO(ELOG_MD_DEFAULT, 
    "This is a long early boot message that spans multiple ring buffer wraps");
```

**Expected**: Entire message prints; no truncation.

### Test 3: Rapid Successive Logs
```c
for (int i = 0; i < 100; i++) {
    ELOG_DEBUG(ELOG_MD_MAIN, "Message %d", i);
}
```

**Expected**: All 100 messages appear (possibly buffered by ring).

### Test 4: Logging from Multiple Threads
```c
void thread_1_func(ULONG arg) {
    for (int i = 0; i < 50; i++) {
        ELOG_DEBUG(ELOG_MD_MAIN, "Thread 1 msg %d", i);
    }
}

void thread_2_func(ULONG arg) {
    for (int i = 0; i < 50; i++) {
        ELOG_DEBUG(ELOG_MD_MAIN, "Thread 2 msg %d", i);
    }
}
```

**Expected**: All 100 messages appear; output is not corrupted (no half-messages).

---

## Phase 8: Diagnostics / Troubleshooting

### Symptom: "Prints ~10 characters then stops"

**Likely Cause**: `LPUartReady` state is racy or interrupts left disabled.

**Check**:
1. Does `LPUartQueueBuffWrite()` always call `RESTORE_PRIMASK()`? ✓
2. Is `ring_read_multiple()` ever called with `PRIMASK=1`? ✗ (should not be)
3. Is DMA completion callback defined? ✓

### Symptom: "Hangs on first ELOG_INFO after LOG_INIT_WITH_CONSOLE_AUTO"

**Likely Cause**: Ring trying to acquire mutex before ThreadX started, or deadlock on PRIMASK+mutex.

**Check**:
1. Are mutex callbacks registered only **after** ThreadX? ✓
2. Are ring calls protected by brief `DISABLE_IRQ()` at early boot? ✓
3. Is power mode set to SLEEP during TX? ✓

### Symptom: "Output is corrupted / half-messages"

**Likely Cause**: Ring buffer being accessed without proper synchronization.

**Check**:
1. Is `ring_write_multiple()` always atomic? (It should be with enter_cs/exit_cs)
2. Does `LPUartTxCpltCallback()` acquire the same mutex as thread context? ✓
3. Are there multiple DMA kicks happening simultaneously? ✗ (should not be)

### Symptom: "High power consumption during logging"

**Likely Cause**: Not forcing SLEEP mode during TX (staying in STOP1 wastes power).

**Check**:
1. Is `UTIL_LPM_SetMaxMode(..., UTIL_LPM_SLEEP_MODE)` called in `LPUartDMAWrite()`? ✓
2. Is it called **before** `HAL_UART_Transmit_DMA()`? ✓
3. Is STOP1 mode re-enabled after TX complete? ✓

---

## Validation Checklist (Pre-Deployment)

Before flashing to hardware:

- [ ] All ring operations happen with interrupts **ENABLED**
- [ ] TX engine state transitions use **brief IRQ-off only**
- [ ] No code path leaves `PRIMASK=1` without `RESTORE_PRIMASK()`
- [ ] Power management forces **SLEEP mode during UART TX**
- [ ] Mutex callbacks registered **only after ThreadX** starts
- [ ] Early boot logs work **before ThreadX** starts
- [ ] No "prints 10 chars then stops" on any test
- [ ] No deadlocks or hangs in stress tests (100+ logs, multiple threads)
- [ ] Output is clean (no corruption, half-messages, or dropped characters)

---

## References

- [UART_DMA_STDOUT_INTEGRATION.md](UART_DMA_STDOUT_INTEGRATION.md) — Detailed architecture and why this pattern works
- [rsIO.c](../../../App/Platform/rsIO.c) — Actual implementation
- [ring.c](../ring/ring.c) — Ring buffer critical section logic
- [RING.md](RING.md) — Ring buffer API reference
- [ELOG.md](ELOG.md) — eLog logging system reference

---

## Version History

| Version | Date       | Changes |
|---------|------------|---------|
| 1.0     | 2025-01-16 | Initial stable pattern documented |
