/**
 * @file uart_dma_ring_elog_example.c
 * @author Andy Chen (clgm216@gmail.com)
 * @version 1.0
 * @date 2025-01-16
 * @brief Complete example: UART DMA + Ring Buffer + eLog integration
 * 
 * This example demonstrates the **stable pattern** for:
 * 1. Retargeting stdout (printf, eLog) through a ring buffer
 * 2. DMA-based UART transmission with safe state management
 * 3. Early boot logging (before RTOS starts)
 * 4. Thread-safe logging (after RTOS starts with mutex callbacks)
 * 5. Power management during active UART TX
 * 
 * Key Takeaways:
 * - Ring operations ALWAYS happen with interrupts ENABLED
 * - TX engine state transitions (IDLE↔BUSY) use brief IRQ-off windows
 * - DMA never runs while system is in low-power STOP mode
 * - Callbacks registered after ThreadX starts, early logging still works
 * 
 * SEE ALSO: UART_DMA_STDOUT_INTEGRATION.md for detailed architecture
 */

#include "rsIO.h"
#include "ring.h"
#include "eLog.h"
#include "stm32_lpm.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

/* ========================================================================== */
/* Phase 1: Early Boot Logging (Before ThreadX)                             */
/* ========================================================================== */

/**
 * @brief Demonstrate early boot logging
 * 
 * Called from main() after debug_interface_init(&hlpuart1):
 *   debug_interface_init(&hlpuart1);       // Init ring buffer
 *   LOG_INIT_WITH_CONSOLE_AUTO();          // Init eLog console subscriber
 *   early_boot_logging_demo();             // THIS FUNCTION
 *   ...
 *   MX_ThreadX_Init();                     // Start ThreadX (enables mutex callbacks)
 * 
 * At this stage:
 * - Ring buffer is operational with PRIMASK-based critical sections
 * - No RTOS mutex callbacks registered yet (we're pre-scheduler)
 * - printf/eLog console output routes through ring → DMA → UART
 * - Multiple early boot logs should NOT cause hangs
 */
void early_boot_logging_demo(void)
{
    printf("=== Early Boot Logging (Pre-RTOS) ===\r\n");
    printf("This is before ThreadX scheduler starts.\r\n");
    printf("Ring buffer uses PRIMASK-only critical sections (no mutexes yet).\r\n");
    
    // eLog early boot logging
    ELOG_INFO(ELOG_MD_DEFAULT, "System initialized successfully");
    ELOG_INFO(ELOG_MD_DEFAULT, "Firmware version: 1.0.0");
    ELOG_INFO(ELOG_MD_DEFAULT, "Ready for ThreadX startup");
    
    printf("=== Early Boot Complete ===\r\n");
}

/* ========================================================================== */
/* Phase 2: RTOS Initialization & Callback Registration                     */
/* ========================================================================== */

/**
 * @brief Called from tx_application_define() after ThreadX setup
 * 
 * This is where we register RTOS mutex callbacks so that:
 * - Ring buffers get per-instance mutexes for full thread safety
 * - eLog uses RTOS mutexes for critical sections
 * - All future logging is fully protected against race conditions
 * 
 * IMPORTANT: Early boot logs (before this function) still work!
 *            Ring falls back to PRIMASK-only critical sections.
 */
void setup_rtos_logging_callbacks(void)
{
    extern const mutex_callbacks_t mutex_callbacks;  // From rtos.c
    
    // Register unified mutex callbacks for Ring Buffer
    ring_register_cs_callbacks(&mutex_callbacks);
    
    // Register unified mutex callbacks for eLog
    elog_register_mutex_callbacks(&mutex_callbacks);
    
    // Signal utilities that RTOS is ready
    utilities_set_RTOS_ready(true);
    
    ELOG_INFO(ELOG_MD_DEFAULT, "RTOS started - full thread safety enabled");
    ELOG_INFO(ELOG_MD_DEFAULT, "Ring buffers now have per-instance mutexes");
    ELOG_INFO(ELOG_MD_DEFAULT, "eLog uses RTOS mutexes for critical sections");
}

/* ========================================================================== */
/* Phase 3: Thread-Safe Logging from Multiple Threads                       */
/* ========================================================================== */

/**
 * @brief Example: Logging from main thread
 */
void main_thread_logging_demo(void)
{
    ELOG_DEBUG(ELOG_MD_MAIN, "Main thread is running");
    ELOG_DEBUG(ELOG_MD_MAIN, "This log is protected by RTOS mutex");
    
    printf("Main thread: printf also goes through ring buffer\r\n");
    printf("Main thread: Multiple writes are safe\r\n");
}

/**
 * @brief Example: Logging from worker thread
 * 
 * ThreadX thread entry function:
 */
void worker_thread_func(ULONG input)
{
    for (int i = 0; i < 5; i++) {
        ELOG_DEBUG(ELOG_MD_MAIN, "Worker thread iteration %d", i);
        
        // Each thread's logging is independent via ring's per-instance mutex
        // No blocking between Main and Worker thread logs
        tx_thread_sleep(100);  // Sleep a bit
    }
}

/* ========================================================================== */
/* Phase 4: Understanding the Safe Pattern (Conceptual)                     */
/* ========================================================================== */

/**
 * @brief Reference implementation of the safe UART TX kick pattern
 * 
 * This is what _write() → LPUartQueueBuffWrite() does:
 * 
 * Step 1: Write to ring with interrupts ENABLED (can acquire mutex if RTOS ready)
 * Step 2: Try to claim TX engine with VERY SHORT IRQ-off window
 * Step 3: If we claimed it, read from ring and start DMA (interrupts ENABLED again)
 * 
 * Why this is safe:
 * - Ring operations can safely use mutex callbacks → no deadlock
 * - TX state transitions are atomic (brief IRQ-off)
 * - ISR (DMA complete) can safely read from ring too
 * - System won't enter STOP mode while TX is active
 * 
 * See rsIO.c for the actual implementation.
 */
void reference_safe_tx_kick_pattern(void)
{
    /*
    int LPUartQueueBuffWrite(int handle, const char *buf, size_t bufSize)
    {
      size_t chars_written = 0;
      bool start_tx = false;
    
      // ✓ Step 1: Ring write with interrupts ENABLED
      chars_written = ring_write_multiple(&LPUartTxRing, (uint8_t *)buf, bufSize);
    
      // ✓ Step 2: Claim TX engine (brief IRQ-off, NO ring operations)
      BACKUP_PRIMASK();
      DISABLE_IRQ();
      if (LPUartReady == IDLE)
      {
        LPUartReady = BUSY;
        start_tx = true;
      }
      RESTORE_PRIMASK();  // Immediately restore
    
      // ✓ Step 3: Start TX if we claimed engine (interrupts ENABLED)
      if (start_tx)
      {
        uint32_t size = 0;
        memset(locale_tx_buffer, 0, L_TX_BUFFER_SIZE);
        
        // Ring read with interrupts ENABLED (safe to use mutex callbacks!)
        size = ring_read_multiple(&LPUartTxRing, locale_tx_buffer, L_TX_BUFFER_SIZE);
        if (size > 0)
        {
          LPUartDMAWrite(locale_tx_buffer, (uint16_t)size);  // Start DMA
        }
        else
        {
          // Release engine
          BACKUP_PRIMASK();
          DISABLE_IRQ();
          LPUartReady = IDLE;
          RESTORE_PRIMASK();
        }
      }
    
      return (chars_written);
    }
    */
}

/* ========================================================================== */
/* Phase 5: Demonstrating Problem Scenarios (What NOT to do)                */
/* ========================================================================== */

/**
 * ❌ WRONG PATTERN 1: Calling ring_* with PRIMASK=1
 * 
 * Problem: If ring_read() tries to acquire an RTOS mutex while PRIMASK=1,
 *          the system deadlocks (mutex acquire can't work with interrupts off).
 * 
 * void wrong_pattern_1(void)
 * {
 *     DISABLE_IRQ();
 *     ring_read_multiple(&ring, buffer, len);  // ❌ DEADLOCK if RTOS ready!
 *     RESTORE_PRIMASK();
 * }
 */

/**
 * ❌ WRONG PATTERN 2: Leaving PRIMASK disabled after a code path
 * 
 * Problem: If an early return happens with PRIMASK=1, interrupts are
 *          permanently disabled → DMA completion never fires → output stops.
 * 
 * void wrong_pattern_2(void)
 * {
 *     DISABLE_IRQ();
 *     if (some_condition) {
 *         return;  // ❌ Left interrupts OFF!
 *     }
 *     RESTORE_PRIMASK();
 * }
 */

/**
 * ❌ WRONG PATTERN 3: Entering STOP mode during UART TX
 * 
 * Problem: STOP1 gates the LPUART clock, causing DMA transfers to corrupt
 *          or stall, and TX_COMPLETE callback never fires.
 * 
 * void wrong_pattern_3(void)
 * {
 *     UTIL_LPM_SetMaxMode(..., UTIL_LPM_STOP1_MODE);  // ❌ Clock off!
 *     HAL_UART_Transmit_DMA(&hlpuart1, data, len);
 *     // DMA may never complete; TX stalls
 * }
 */

/* ========================================================================== */
/* Phase 6: Integration Checklist                                           */
/* ========================================================================== */

/**
 * After integrating this pattern, verify:
 * 
 * [ ] rsIO.c: LPUartQueueBuffWrite() has safe pattern (ring ops after PRIMASK restore)
 * [ ] rsIO.c: LPUartTxCpltCallback() reads from ring without disabling IRQs
 * [ ] rsIO.c: LPUartDMAWrite() sets UTIL_LPM_SLEEP_MODE before DMA start
 * [ ] app_rtos.c: Calls ring_register_cs_callbacks() after ThreadX init
 * [ ] app_rtos.c: Calls elog_register_mutex_callbacks() after ThreadX init
 * [ ] main(): Early logs (before MX_ThreadX_Init) don't hang
 * [ ] main(): RTOS logs (after ThreadX) are thread-safe with no corruption
 * [ ] Power: Current draw is ~1-3mA during logging (SLEEP mode, not STOP1)
 * [ ] Performance: No blocking between independent ring buffers
 */

/* ========================================================================== */
/* Typical main() Flow                                                      */
/* ========================================================================== */

/**
 * @brief Typical application main() showing full integration
 * 
 * This pseudo-code shows the complete flow:
 * 
 * int main(void) {
 *     // Hardware init
 *     HAL_Init();
 *     SystemClock_Config();
 *     MX_GPIO_Init();
 *     MX_LPUART1_UART_Init(&hlpuart1);
 * 
 *     // Early boot logging setup (NO RTOS yet)
 *     debug_interface_init(&hlpuart1);      // Ring buffer only, no mutexes
 *     LOG_INIT_WITH_CONSOLE_AUTO();         // eLog console subscriber
 * 
 *     // Early boot logs are safe (use PRIMASK-only critical sections)
 *     ELOG_INFO(ELOG_MD_DEFAULT, "=== RSSensorV35 Firmware Start ===");
 *     ELOG_INFO(ELOG_MD_DEFAULT, "Early boot logging works!");
 * 
 *     // Start ThreadX (RTOS scheduler)
 *     MX_ThreadX_Init();  // Blocks here; scheduler runs
 *     
 *     // After ThreadX returns (never returns in normal operation)
 *     return 0;
 * }
 * 
 * Inside tx_application_define():
 *     // After ThreadX threads created, before scheduler starts
 *     setup_rtos_logging_callbacks();  // Register mutex callbacks
 *     
 *     // Now all logging is fully thread-safe with per-instance mutexes
 *     ELOG_INFO(ELOG_MD_MAIN, "RTOS ready - thread-safe logging enabled");
 */

#endif  // UART_DMA_RING_ELOG_EXAMPLE_C
