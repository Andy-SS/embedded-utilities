/*
 * eLog_rtos_demo.c
 *
 * Demonstration of eLog RTOS threading features
 * This file shows how to integrate eLog with your RTOS application
 */

#include "eLog.h"
#include <stdio.h>

/* Example: ThreadX configuration */
#if 0
#define ELOG_THREAD_SAFE 1
#define ELOG_RTOS_TYPE ELOG_RTOS_THREADX
#include "tx_api.h"
#endif

/* Example: FreeRTOS configuration */
#if 0  
#define ELOG_THREAD_SAFE 1
#define ELOG_RTOS_TYPE ELOG_RTOS_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

/* For this demo, we'll use bare metal mode */
#define ELOG_THREAD_SAFE 0
#define ELOG_RTOS_TYPE ELOG_RTOS_NONE

/* ========================================================================== */
/* ThreadX Mutex Callback Implementations (Example) */
/* ========================================================================== */
/* Uncomment these if using ThreadX with ELOG_THREAD_SAFE=1 */

#if (ELOG_THREAD_SAFE == 1) && (ELOG_RTOS_TYPE == ELOG_RTOS_THREADX)

#include "tx_api.h"

static TX_MUTEX elog_mutex;

/**
 * @brief Create eLog mutex for ThreadX
 * @retval ELOG_MUTEX_OK on success, ELOG_MUTEX_ERROR on failure
 */
elog_mutex_result_t threadx_mutex_create(void) {
  UINT status = tx_mutex_create(&elog_mutex, "eLog_Mutex", TX_NO_INHERIT);
  return (status == TX_SUCCESS) ? ELOG_MUTEX_OK : ELOG_MUTEX_ERROR;
}

/**
 * @brief Acquire eLog mutex with timeout
 * @param timeout_ms Timeout in milliseconds
 * @retval ELOG_MUTEX_OK on success, ELOG_MUTEX_TIMEOUT on timeout, ELOG_MUTEX_ERROR on error
 */
elog_mutex_result_t threadx_mutex_take(uint32_t timeout_ms) {
  UINT ticks = (timeout_ms * TX_TIMER_TICKS_PER_SECOND) / 1000;
  UINT status = tx_mutex_get(&elog_mutex, ticks);
  
  if (status == TX_SUCCESS) 
    return ELOG_MUTEX_OK;
  else if (status == TX_NOT_AVAILABLE) 
    return ELOG_MUTEX_TIMEOUT;
  else 
    return ELOG_MUTEX_ERROR;
}

/**
 * @brief Release eLog mutex
 * @retval ELOG_MUTEX_OK on success, ELOG_MUTEX_ERROR on failure
 */
elog_mutex_result_t threadx_mutex_give(void) {
  UINT status = tx_mutex_put(&elog_mutex);
  return (status == TX_SUCCESS) ? ELOG_MUTEX_OK : ELOG_MUTEX_ERROR;
}

/**
 * @brief Delete eLog mutex
 * @retval ELOG_MUTEX_OK on success, ELOG_MUTEX_ERROR on failure
 */
elog_mutex_result_t threadx_mutex_delete(void) {
  UINT status = tx_mutex_delete(&elog_mutex);
  return (status == TX_SUCCESS) ? ELOG_MUTEX_OK : ELOG_MUTEX_ERROR;
}

/**
 * @brief ThreadX mutex callbacks structure for eLog
 */
const elog_mutex_callbacks_t threadx_mutex_callbacks = {
  .create = threadx_mutex_create,
  .take = threadx_mutex_take,
  .give = threadx_mutex_give,
  .delete = threadx_mutex_delete
};

#endif /* ELOG_THREAD_SAFE && ELOG_RTOS_TYPE == ELOG_RTOS_THREADX */

/**
 * @brief Initialize logging system for RTOS application (Phase 1: Early init before RTOS)
 * @note  Call this early in main() before RTOS starts
 * @note  Logging will work without thread safety until elog_update_RTOS_ready(true) is called
 */
void rtos_logging_init(void) {
    // Phase 1: Initialize eLog early - before RTOS starts
    LOG_INIT_WITH_CONSOLE_AUTO();
    ELOG_INFO(ELOG_MD_MAIN, "eLog initialized (non-threaded mode)");
    ELOG_INFO(ELOG_MD_MAIN, "Thread safety: %s", (ELOG_THREAD_SAFE ? "ENABLED" : "DISABLED"));
    ELOG_INFO(ELOG_MD_MAIN, "RTOS type: %d", ELOG_RTOS_TYPE);
}

/**
 * @brief Register RTOS mutex callbacks and enable thread-safe logging (Phase 3: End of tx_application_define)
 * @note  Call this at the END of tx_application_define() after all RTOS objects are created
 * @note  This enables mutex protection for all logging calls from this point forward
 */
void rtos_logging_enable_thread_safety(void) {
    // Phase 3: Register callbacks and enable thread safety
    // CRITICAL: Must be called at END of tx_application_define()
    #if (ELOG_THREAD_SAFE == 1) && (ELOG_RTOS_TYPE == ELOG_RTOS_THREADX)
    // Register ThreadX mutex callbacks
    elog_register_mutex_callbacks(&threadx_mutex_callbacks);
    #endif
    
    // Enable thread-safe logging
    elog_update_RTOS_ready(true);
    // NOTE: Do NOT log after this call within tx_application_define()
    // Logging will now use mutex protection automatically
}

/**
 * @brief Example task that uses thread-safe logging with unified error codes
 * @note  Module threshold setting is O(1) - uses direct array indexing
 */
void sensor_task_example(void) {
    // Set module threshold - instant O(1) operation
    elog_set_module_threshold(ELOG_MD_SENSOR, ELOG_LEVEL_DEBUG);

    ELOG_DEBUG(ELOG_MD_SENSOR, "Sensor task starting");
    int sensor_value = 42;
    ELOG_INFO(ELOG_MD_SENSOR, "Sensor reading: %d", sensor_value);

    /* Demonstrate sensor-specific error codes (0x40-0x5F) */
    if (sensor_value > 50) {
        ELOG_WARNING(ELOG_MD_SENSOR, "Sensor range exceeded: 0x%02X", ELOG_SENSOR_ERR_RANGE);
    }
    if (sensor_value < 0) {
        ELOG_CRITICAL(ELOG_MD_SENSOR, "Sensor not responding: 0x%02X", ELOG_SENSOR_ERR_NOT_FOUND);
    }
    
    /* Example of checking sensor communication */
    int accel_status = -1;
    if (accel_status < 0) {
        ELOG_ERROR(ELOG_MD_SENSOR, "Accelerometer calibration failed: 0x%02X", ELOG_ACCEL_ERR);
    }
    
    ELOG_DEBUG(ELOG_MD_SENSOR, "Sensor task completed");
}

/**
 * @brief Example communication task with thread-safe logging and unified error codes
 * @note  Module threshold setting is O(1) - uses direct array indexing
 */
void comm_task_example(void) {
    // Set module threshold - instant O(1) operation
    elog_set_module_threshold(ELOG_MD_COMM, ELOG_LEVEL_DEBUG);

    ELOG_DEBUG(ELOG_MD_COMM, "Communication task starting");
    ELOG_INFO(ELOG_MD_COMM, "Initializing UART communication");
    ELOG_DEBUG(ELOG_MD_COMM, "Starting I2C transaction");

    /* Demonstrate communication error codes (0x20-0x3F) */
    int comm_status = -1;
    if (comm_status != 0) {
        ELOG_ERROR(ELOG_MD_COMM, "I2C communication failed: 0x%02X", ELOG_COMM_ERR_I2C);
    } else {
        ELOG_INFO(ELOG_MD_COMM, "I2C communication successful");
    }
    
    /* Example UART timeout scenario */
    int uart_result = 0;
    if (uart_result == 0) {
        ELOG_WARNING(ELOG_MD_COMM, "UART response timeout: 0x%02X", ELOG_COMM_ERR_UART);
    }
    
    /* Example checksum validation */
    int checksum_valid = 0;
    if (!checksum_valid) {
        ELOG_ERROR(ELOG_MD_COMM, "Packet checksum mismatch: 0x%02X", ELOG_COMM_ERR_CHECKSUM);
    }
    
    ELOG_DEBUG(ELOG_MD_COMM, "Communication task completed");
}

/**
 * @brief Example of using custom subscriber in RTOS environment
 */
void custom_subscriber_example(elog_level_t level, const char *msg) {
    printf("[%lu] CUSTOM[%s]: %s\n", 
           (unsigned long)0, /* Would be real timestamp */
           elog_level_name(level), 
           msg);
}

/**
 * @brief Demonstrate multi-subscriber setup for RTOS (simplified management)
 * @note  Subscribers are completely removed on unsubscribe (no 'active' flag)
 */
void rtos_multi_subscriber_demo(void) {
    ELOG_INFO(ELOG_MD_MAIN, "Setting up multiple subscribers for RTOS environment");

    /* Add custom subscriber for ERROR and above */
    LOG_SUBSCRIBE(custom_subscriber_example, ELOG_LEVEL_ERROR);

    /* Test messages at different levels */
    ELOG_DEBUG(ELOG_MD_MAIN, "This goes only to console");
    ELOG_INFO(ELOG_MD_MAIN, "This also goes only to console");  
    ELOG_WARNING(ELOG_MD_MAIN, "This also goes only to console");
    ELOG_ERROR(ELOG_MD_MAIN, "This goes to BOTH console and custom subscriber");
    ELOG_CRITICAL(ELOG_MD_MAIN, "This also goes to BOTH subscribers");

    /* Per-module threshold demonstration */
    elog_set_module_threshold(ELOG_MD_MAIN, ELOG_LEVEL_WARNING);
    ELOG_INFO(ELOG_MD_MAIN, "This info message will NOT be shown (threshold too high)");
    ELOG_WARNING(ELOG_MD_MAIN, "This warning message WILL be shown");
    ELOG_ERROR(ELOG_MD_MAIN, "This error message WILL be shown");
    ELOG_CRITICAL(ELOG_MD_MAIN, "This critical message WILL be shown");

    ELOG_INFO(ELOG_MD_MAIN, "Multi-subscriber demo completed");
}

/**
 * @brief Main demo function
 */
void rtos_logging_demo(void) {
    printf("\n=== eLog RTOS Integration Demo ===\n");

    rtos_logging_init();
    sensor_task_example();
    comm_task_example();
    rtos_multi_subscriber_demo();

    printf("\n=== Demo Complete ===\n");
}

/* Usage in your RTOS application (Three-Phase Initialization):
 *
 * PHASE 1: Early Initialization (in main() before RTOS starts)
 * =============================================================
 * Step 1: Configure eLog in eLog.h:
 *    #define ELOG_THREAD_SAFE 1
 *    #define ELOG_RTOS_TYPE ELOG_RTOS_THREADX  // or FREERTOS, CMSIS
 *    #define ELOG_MUTEX_TIMEOUT_MS 500         // Production-grade timeout
 *
 * Step 2: Include RTOS headers before eLog.h in your source files
 *
 * Step 3: Call rtos_logging_init() early in main():
 *    int main(void) {
 *        HAL_Init();
 *        SystemClock_Config();
 *        rtos_logging_init();   // ← Phase 1: Initialize eLog (non-threaded)
 *        MX_APPE_Init();
 *        MX_ThreadX_Init();     // ← This starts RTOS - doesn't return
 *        while(1);              // Never reached
 *    }
 *
 * PHASE 2: RTOS Objects Setup (in tx_application_define)
 * ========================================================
 * Step 4: Create all RTOS threads, semaphores, mutexes normally:
 *    VOID tx_application_define(VOID *first_unused_memory) {
 *        // Create byte pools
 *        tx_byte_pool_create(&tx_app_byte_pool, "Tx App memory pool", 
 *                            tx_byte_pool_buffer, TX_APP_MEM_POOL_SIZE);
 *        
 *        // Create threads, semaphores, etc.
 *        tx_thread_create(&TaskIdleThread, "TaskIdle", TaskIdleEntry, 0,
 *                         task_stack, TASK_STACK_SIZE,
 *                         TASK_PRIORITY, TASK_PRIORITY,
 *                         TX_NO_TIME_SLICE, TX_AUTO_START);
 *        // ... more RTOS setup ...
 *        
 *        // Logging works without mutex protection during this phase
 *
 * PHASE 3: Enable Thread Safety (END of tx_application_define)
 * =============================================================
 * Step 5: Call rtos_logging_enable_thread_safety() at END of tx_application_define():
 *        // CRITICAL: MUST be at the very END - after all RTOS objects created
 *        rtos_logging_enable_thread_safety();  // ← Phase 3: Enable mutex protection
 *        // Do NOT log after this - to prevent deadlock during init
 *    }
 *
 * RUNTIME: Thread-Safe Logging (in any task)
 * ===========================================
 * Step 6: Use logging macros - they're automatically thread-safe:
 *    void TaskEntry(ULONG thread_input) {
 *        // Set per-module threshold (O(1) operation)
 *        elog_set_module_threshold(ELOG_MD_MAIN, ELOG_LEVEL_DEBUG);
 *        
 *        while(1) {
 *            ELOG_DEBUG(ELOG_MD_MAIN, "Thread-safe logging - mutex protected");
 *            ELOG_INFO(ELOG_MD_MAIN, "Safe to call from multiple threads");
 *            tx_thread_sleep(1000);
 *        }
 *    }
 *
 * KEY POINTS:
 * ============
 * - Phase 1 (rtos_logging_init): Logging without mutex protection
 * - Phase 2 (RTOS setup): Logging still without mutex protection
 * - Phase 3 (rtos_logging_enable_thread_safety): Enable mutex protection
 * - Runtime: All logging automatically protected by mutex
 * - Per-module thresholds use O(1) direct array indexing (very fast)
 * - Subscriber management is simplified - entries removed on unsubscribe
 * - Mutex timeout: 500ms (graceful degradation on overload)
 * - Do NOT log after rtos_logging_enable_thread_safety() in tx_application_define()
 */
