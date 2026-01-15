/**
 * @file common_example.c
 * @author Andy Chen (clgm216@gmail.com)
 * @version 0.01
 * @date 2025-01-15
 * @brief Example usage of common utilities for unified mutex operations
 *        with eLog and Ring buffer integration
 * 
 * This example demonstrates:
 * - Registering unified mutex callbacks
 * - Managing RTOS ready state
 * - Using common utilities functions
 * - Integration with eLog for thread-safe logging
 * - Integration with Ring buffer for thread-safe data buffering
 */

#include "mutex_common.h"
#include "eLog.h"
#include "ring.h"
#include <stdio.h>

/*
  Mutex callback implementations would be provided by the RTOS layer,
  e.g., ThreadX, FreeRTOS, etc. For this example, we assume they are defined
  elsewhere and linked appropriately.
  Some sample function prototypes as follows:
*/

/***********************************************************
 * @brief  The function allocates memory from the ThreadX byte pool
 *         Only used after tx task scheduler has started
 * @param size 
 * @return void* 
************************************************************/
void* byte_allocate(size_t size)
{
  void *ptr = NULL;
  UINT result;
  result = tx_byte_allocate(&tx_app_byte_pool, &ptr, size, TX_NO_WAIT);
  if (result != TX_SUCCESS)
  {
    printERR(ELOG_MD_DEFAULT, "Failed to allocate %u bytes from ThreadX byte pool, result: 0x%02X", (unsigned int)size, result);
    uint32_t unused_bytes;
    tx_byte_pool_info_get(&tx_app_byte_pool, NULL, &unused_bytes, NULL, NULL, NULL, NULL);
    printERR(ELOG_MD_DEFAULT, "ThreadX byte pool has %u unused bytes", (unsigned int)unused_bytes);
    ptr = NULL;
  }
  printIF(ELOG_MD_DEFAULT, "Allocated %u bytes from ThreadX byte pool at %p", (unsigned int)size, ptr);
  return ptr;
}

/***********************************************************
 * @brief  The function releases memory back to the ThreadX byte pool
 *         Only used after tx task scheduler has started
 * @param ptr 
************************************************************/
void byte_release(void *ptr)
{
  if (ptr != NULL)
  {
    if (tx_byte_release(ptr) != TX_SUCCESS)
    {
      printERR(ELOG_MD_DEFAULT, "Failed to release memory at %p to ThreadX byte pool", ptr);
    }
  }
}

#include "mutex_common.h"
/* Create a new mutex for a ring buffer instance */
void* threadx_mutex_create(void) {
  TX_MUTEX *mutex = (TX_MUTEX *)byte_allocate(sizeof(TX_MUTEX));
  if (mutex == NULL) {
    return NULL;
  }
  
  UINT status = tx_mutex_create(mutex, "Mutex", TX_NO_INHERIT);
  if (status != TX_SUCCESS) {
    byte_release(mutex);
    return NULL;
  }
  
  return (void *)mutex;
}

/* Destroy a ring buffer's mutex */
mutex_result_t threadx_mutex_destroy(void *mutex) {
  if (mutex == NULL) return MUTEX_ERROR;
  
  TX_MUTEX *tx_mutex = (TX_MUTEX *)mutex;
  UINT status = tx_mutex_delete(tx_mutex);
  byte_release(tx_mutex);
  return (status == TX_SUCCESS) ? MUTEX_OK : MUTEX_ERROR;
}

/* Lock a specific mutex */
mutex_result_t threadx_mutex_enter(void *mutex, uint32_t timeout_ms) {
  if (mutex == NULL) {
    return MUTEX_ERROR;
  }
  
  TX_MUTEX *tx_mutex = (TX_MUTEX *)mutex;
  UINT timeout_ticks = (timeout_ms == UINT32_MAX) ? TX_WAIT_FOREVER : 
                       (timeout_ms * TX_TIMER_TICKS_PER_SECOND) / 1000;
  UINT status = tx_mutex_get(tx_mutex, timeout_ticks);
  
  if (status == TX_SUCCESS) {
    return MUTEX_OK;
  } else if (status == TX_NOT_AVAILABLE) {
    return MUTEX_TIMEOUT;
  }
  return MUTEX_ERROR;
}

/* Unlock a specific mutex */
mutex_result_t threadx_mutex_exit(void *mutex) {
  if (mutex == NULL) {
    return MUTEX_ERROR;
  }
  
  TX_MUTEX *tx_mutex = (TX_MUTEX *)mutex;
  UINT status = tx_mutex_put(tx_mutex);
  return (status == TX_SUCCESS) ? MUTEX_OK : MUTEX_ERROR;
}

/* Register callbacks with ring buffer */
const mutex_callbacks_t mutex_callbacks = {
  .create = threadx_mutex_create,
  .destroy = threadx_mutex_destroy,
  .acquire = threadx_mutex_enter,
  .release = threadx_mutex_exit
};

/* ========================================================================== */
/* Example 1: Basic Common Utilities Initialization */
/* ========================================================================== */

/**
 * @brief Initialize common utilities and register RTOS callbacks
 * 
 * This function should be called in tx_application_define() after ThreadX
 * scheduler setup is complete.
 */
void common_utilities_init(void) {
    // Get the ThreadX mutex callbacks defined in rtos.c
    extern const mutex_callbacks_t mutex_callbacks;
    
    // Register the unified mutex callbacks for all utilities
    utilities_register_cs_cbs(&mutex_callbacks);
    
    // Signal to utilities that RTOS is ready
    utilities_set_RTOS_ready(true);
    
    // Initialize eLog
    LOG_INIT_WITH_CONSOLE();
    
    ELOG_INFO(ELOG_MD_MAIN, "Common utilities initialized");
    ELOG_INFO(ELOG_MD_MAIN, "RTOS ready: %s", 
              utilities_is_RTOS_ready() ? "YES" : "NO");
}

/* ========================================================================== */
/* Example 2: Manual Mutex Creation (Advanced Usage) */
/* ========================================================================== */

/**
 * @brief Example of manually creating and using a mutex via common utilities
 * 
 * Note: In typical usage, eLog and Ring handle their own mutexes.
 *       This example is for special cases requiring manual mutex management.
 */
void manual_mutex_example(void) {
    void *my_custom_mutex = NULL;
    
    // Create a mutex using the registered callbacks
    mutex_result_t result = utilities_mutex_create(&my_custom_mutex);
    
    if (result != MUTEX_OK) {
        ELOG_ERROR(ELOG_MD_MAIN, "Failed to create custom mutex: %d", result);
        return;
    }
    
    ELOG_INFO(ELOG_MD_MAIN, "Custom mutex created successfully");
    
    // Example: Protect access to shared resource
    shared_resource_t shared_data;
    shared_data.value = 0;
    
    // Lock mutex - 500ms timeout
    result = utilities_mutex_take(my_custom_mutex, 500);
    
    if (result == MUTEX_OK) {
        // Critical section - modify shared resource
        shared_data.value++;
        ELOG_DEBUG(ELOG_MD_MAIN, "Modified shared resource: %d", 
                   shared_data.value);
        
        // Unlock mutex
        utilities_mutex_give(my_custom_mutex);
    } 
    else if (result == MUTEX_TIMEOUT) {
        ELOG_WARNING(ELOG_MD_MAIN, "Mutex timeout - possible contention");
    }
    else {
        ELOG_ERROR(ELOG_MD_MAIN, "Mutex operation failed: %d", result);
    }
    
    // Cleanup when done
    utilities_mutex_delete(my_custom_mutex);
    ELOG_INFO(ELOG_MD_MAIN, "Custom mutex deleted");
}

/* ========================================================================== */
/* Example 3: Thread-Safe eLog Logging via Common Utilities */
/* ========================================================================== */

/**
 * @brief Example task using eLog with thread-safe common utilities
 * 
 * eLog automatically uses the registered callbacks from common utilities
 */
void logging_task(ULONG arg) {
    uint32_t iteration = 0;
    
    ELOG_INFO(ELOG_MD_TASK_A, "Logging task started");
    
    while (1) {
        // All logging operations are automatically protected by
        // the mutex callbacks registered in common utilities
        ELOG_INFO(ELOG_MD_TASK_A, "Iteration %lu - RTOS ready: %s",
                  iteration,
                  utilities_is_RTOS_ready() ? "YES" : "NO");
        
        ELOG_DEBUG(ELOG_MD_TASK_A, "Task performing work");
        
        if (iteration % 10 == 0) {
            ELOG_WARNING(ELOG_MD_TASK_A, "Multiple of 10: %lu", iteration);
        }
        
        iteration++;
        
        // Delay
        tx_thread_sleep(TX_MS_TO_TICKS(1000));
    }
}

/* ========================================================================== */
/* Example 4: Ring Buffer with Common Utilities */
/* ========================================================================== */

typedef struct {
    uint32_t sensor_id;
    uint32_t timestamp;
    float temperature;
    float humidity;
} SensorReading;

ring_t sensor_ring;

/**
 * @brief Producer task - writes sensor data to ring buffer
 * 
 * The ring buffer automatically creates and uses a per-instance mutex
 * via the callbacks registered in common utilities
 */
void sensor_producer_task(ULONG arg) {
    SensorReading reading;
    
    // Ring creates its own per-instance mutex automatically
    // This mutex uses the callbacks registered in common utilities
    ring_init_dynamic(&sensor_ring, 64, sizeof(SensorReading));
    
    ELOG_INFO(ELOG_MD_MAIN, "Sensor producer task started");
    
    while (1) {
        // Simulate sensor reading
        reading.sensor_id = 1;
        reading.timestamp = tx_time_get();
        reading.temperature = 25.0f + (rand() % 5);
        reading.humidity = 60.0f + (rand() % 10);
        
        // Write to ring - automatically protected by per-instance mutex
        // (via common utilities callbacks)
        ring_result_t result = ring_write(&sensor_ring, &reading);
        
        if (result == RING_OK) {
            ELOG_DEBUG(ELOG_MD_MAIN, "Sensor data written: T=%.1f°C, H=%.1f%%",
                      reading.temperature, reading.humidity);
        } else {
            ELOG_WARNING(ELOG_MD_MAIN, "Ring buffer full or error: %d", result);
        }
        
        tx_thread_sleep(TX_MS_TO_TICKS(500));
    }
}

/**
 * @brief Consumer task - reads sensor data from ring buffer
 * 
 * Uses the same ring buffer and per-instance mutex as producer
 */
void sensor_consumer_task(ULONG arg) {
    SensorReading reading;
    
    ELOG_INFO(ELOG_MD_MAIN, "Sensor consumer task started");
    
    tx_thread_sleep(TX_MS_TO_TICKS(100));  // Wait for producer
    
    while (1) {
        // Read from ring - automatically protected by per-instance mutex
        ring_result_t result = ring_read(&sensor_ring, &reading);
        
        if (result == RING_OK) {
            ELOG_INFO(ELOG_MD_MAIN, 
                     "Sensor %lu: Temp=%.1f°C, Humidity=%.1f%%",
                     reading.sensor_id, reading.temperature, reading.humidity);
        } else if (result == RING_EMPTY) {
            ELOG_DEBUG(ELOG_MD_MAIN, "Ring buffer empty");
        } else {
            ELOG_ERROR(ELOG_MD_MAIN, "Ring read error: %d", result);
        }
        
        tx_thread_sleep(TX_MS_TO_TICKS(1000));
    }
}

/* ========================================================================== */
/* Example 5: Multi-Task Thread-Safe Logging */
/* ========================================================================== */

/**
 * @brief Multiple tasks all logging safely via common utilities
 * 
 * All eLog operations automatically use the mutex callbacks from common utilities,
 * ensuring thread-safe logging even when multiple tasks log simultaneously
 */
void multi_task_logging_example(void) {
    // All these tasks can log safely without explicit synchronization
    // because eLog uses the unified mutex callbacks from common utilities
    
    // Task 1
    ELOG_INFO(ELOG_MD_TASK_A, "Task A: Starting process");
    
    // Task 2 (would be in different thread)
    ELOG_INFO(ELOG_MD_TASK_B, "Task B: Processing data");
    
    // Task 3 (would be in different thread)
    ELOG_WARNING(ELOG_MD_TASK_C, "Task C: Warning condition detected");
    
    // Each log line is protected by the mutex from common utilities
    // Output will not be garbled or interleaved
}

/* ========================================================================== */
/* Example 6: Error Handling with Common Utilities */
/* ========================================================================== */

/**
 * @brief Demonstrate error handling when RTOS is not ready
 */
void error_handling_example(void) {
    // Check if RTOS is ready before assuming mutex availability
    if (!utilities_is_RTOS_ready()) {
        ELOG_WARNING(ELOG_MD_MAIN, "RTOS not ready - operating in bare metal mode");
        
        // Can still use utilities, but without thread safety
        ELOG_INFO(ELOG_MD_MAIN, "Continuing with single-threaded operation");
        return;
    }
    
    // RTOS is ready - safe to use all threading features
    ELOG_INFO(ELOG_MD_MAIN, "RTOS ready - thread-safe operations available");
}

/* ========================================================================== */
/* Example 7: Integration Example - Full Initialization Pattern */
/* ========================================================================== */

/**
 * @brief Complete initialization pattern for your application
 * 
 * This shows the recommended sequence for initializing common utilities
 */
void application_full_init_example(void) {
    // ========== PHASE 1: Hardware Initialization (main.c) ==========
    // Called before RTOS starts
    
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    // Initialize eLog early for debug output
    LOG_INIT_WITH_CONSOLE();
    printLOG(ELOG_MD_DEFAULT, "Hardware initialized");
    
    // ========== PHASE 2: RTOS Kernel Start (app_entry.c) ==========
    // ThreadX scheduler will start, but threads haven't run yet
    
    // Create threads, semaphores, queues, etc.
    // (ThreadX structures created but not running)
    
    // ========== PHASE 3: RTOS Ready Signal (tx_application_define) ==========
    // ThreadX scheduler is starting - now safe to use RTOS features
    
    extern const mutex_callbacks_t mutex_callbacks;
    
    // Register unified mutex callbacks
    utilities_register_cs_cbs(&mutex_callbacks);
    
    // Signal RTOS is ready
    utilities_set_RTOS_ready(true);
    
    // Now all utilities use thread-safe mutexes
    ELOG_INFO(ELOG_MD_MAIN, "RTOS started - thread-safe mode enabled");
    
    // ========== PHASE 4: Task Execution ==========
    // ThreadX scheduler runs tasks, all logging is thread-safe
    
    // Threads start executing...
}

/* ========================================================================== */
/* Example 8: Graceful Degradation - Bare Metal Fallback */
/* ========================================================================== */

/**
 * @brief Demonstrate safe operation without RTOS
 * 
 * Common utilities and eLog work correctly even without RTOS,
 * gracefully degrading to single-threaded mode
 */
void bare_metal_example(void) {
    // Initialize without setting RTOS ready
    // utilities_set_RTOS_ready(false);  // Can explicitly disable if needed
    
    // Check status
    if (!utilities_is_RTOS_ready()) {
        ELOG_INFO(ELOG_MD_MAIN, "Operating in bare metal mode");
        
        // eLog still works, just without thread safety
        ELOG_INFO(ELOG_MD_MAIN, "Single-threaded logging available");
        
        // Ring buffers still work, just without mutex protection
        ring_t ring;
        ring_init_dynamic(&ring, 64, sizeof(uint8_t));
        
        uint8_t data = 42;
        ring_write(&ring, &data);
        
        ELOG_INFO(ELOG_MD_MAIN, "Ring buffer operations available");
        
        ring_destroy(&ring);
    }
}

/* ========================================================================== */
/* Example 9: Checking Common Utilities Status */
/* ========================================================================== */

/**
 * @brief Demonstrate status checking functions
 */
void status_check_example(void) {
    ELOG_INFO(ELOG_MD_MAIN, "=== Common Utilities Status ===");
    
    // Check RTOS status
    bool rtos_ready = utilities_is_RTOS_ready();
    ELOG_INFO(ELOG_MD_MAIN, "RTOS Ready: %s", rtos_ready ? "YES" : "NO");
    
    // All utilities automatically adapt based on this status
    ELOG_INFO(ELOG_MD_MAIN, "Thread Safety: %s", 
              rtos_ready ? "ENABLED" : "DISABLED");
    
    // eLog logging always works
    ELOG_TRACE(ELOG_MD_MAIN, "Trace logging available");
    ELOG_DEBUG(ELOG_MD_MAIN, "Debug logging available");
    ELOG_INFO(ELOG_MD_MAIN, "Info logging available");
    ELOG_WARNING(ELOG_MD_MAIN, "Warning logging available");
    ELOG_ERROR(ELOG_MD_MAIN, "Error logging available");
    ELOG_CRITICAL(ELOG_MD_MAIN, "Critical logging available");
}

/* ========================================================================== */
/* Minimal Complete Example */
/* ========================================================================== */

/**
 * @brief Minimal example showing the three key steps
 * 
 * This is the bare minimum needed to use common utilities:
 * 1. Register callbacks
 * 2. Set RTOS ready flag
 * 3. Use eLog and Ring as normal
 */
void minimal_example(void) {
    // STEP 1: Register callbacks
    extern const mutex_callbacks_t mutex_callbacks;
    utilities_register_cs_cbs(&mutex_callbacks);
    
    // STEP 2: Set RTOS ready
    utilities_set_RTOS_ready(true);
    
    // STEP 3: Everything is automatically thread-safe now!
    ELOG_INFO(ELOG_MD_MAIN, "Thread-safe logging enabled");
    
    ring_t ring;
    ring_init_dynamic(&ring, 64, sizeof(uint8_t));
    
    // Both eLog and ring are thread-safe via common utilities
}

#endif  // COMMON_EXAMPLE_C_
