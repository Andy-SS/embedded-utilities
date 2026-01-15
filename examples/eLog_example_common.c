/**
 * @file eLog_example_common.c
 * @author Andy Chen (clgm216@gmail.com)
 * @version 0.06
 * @date 2025-01-15
 * @brief Enhanced logging examples with unified mutex callbacks via common utilities
 * 
 * This example demonstrates:
 * - eLog initialization with common utilities
 * - Thread-safe logging in multi-task environment
 * - Per-module log level configuration
 * - Multiple subscribers
 * - Error code usage
 * - eLog integration with ThreadX
 */

#include "eLog.h"
#include "mutex_common.h"
#include <stdio.h>

/* ========================================================================== */
/* Example 1: Basic eLog Initialization with Common Utilities */
/* ========================================================================== */

/**
 * @brief Initialize eLog with unified mutex callbacks
 * 
 * This demonstrates the standard initialization pattern using
 * unified mutex callbacks from common utilities
 */
void example_elog_basic_init(void) {
    // Initialize eLog with console output
    LOG_INIT_WITH_CONSOLE();
    
    ELOG_INFO(ELOG_MD_MAIN, "eLog initialized with console output");
    ELOG_DEBUG(ELOG_MD_MAIN, "Debug mode enabled");
}

/**
 * @brief Basic logging with different levels
 */
void example_basic_logging(void) {
    ELOG_TRACE(ELOG_MD_MAIN, "Application trace message");
    ELOG_DEBUG(ELOG_MD_MAIN, "Application debug message");
    ELOG_INFO(ELOG_MD_MAIN, "Application info message");
    ELOG_WARNING(ELOG_MD_MAIN, "Application warning message");
    ELOG_ERROR(ELOG_MD_MAIN, "Application error message");
    ELOG_CRITICAL(ELOG_MD_MAIN, "Application critical message");
    ELOG_ALWAYS(ELOG_MD_MAIN, "Application always message");
}

/* ========================================================================== */
/* Example 2: Thread-Safe Logging in Multi-Task Environment */
/* ========================================================================== */

/**
 * @brief Task 1 - Performs sensor operations
 * 
 * All eLog calls are automatically thread-safe via unified mutex callbacks
 * from common utilities. No explicit synchronization needed.
 */
void sensor_task_entry(ULONG arg) {
    uint32_t sensor_id = (uint32_t)arg;
    uint32_t iteration = 0;
    
    ELOG_INFO(ELOG_MD_SENSOR, "Sensor task %lu started", sensor_id);
    
    while (1) {
        // All these log calls are thread-safe
        // (protected by the unified mutex from common utilities)
        
        if (iteration % 10 == 0) {
            ELOG_INFO(ELOG_MD_SENSOR, "Sensor %lu: iteration %lu", 
                     sensor_id, iteration);
        }
        
        ELOG_DEBUG(ELOG_MD_SENSOR, "Reading sensor %lu...", sensor_id);
        
        // Simulate sensor read
        int sensor_value = 100 + (iteration % 50);
        ELOG_DEBUG(ELOG_MD_SENSOR, "Sensor %lu value: %d", 
                  sensor_id, sensor_value);
        
        if (sensor_value > 140) {
            ELOG_WARNING(ELOG_MD_SENSOR, "Sensor %lu high reading: %d", 
                        sensor_id, sensor_value);
        }
        
        iteration++;
        tx_thread_sleep(TX_MS_TO_TICKS(500));
    }
}

/**
 * @brief Task 2 - Performs communication operations
 * 
 * This task logs independently, thread-safe logging works without
 * any explicit coordination with other tasks
 */
void comm_task_entry(ULONG arg) {
    uint32_t msg_count = 0;
    
    ELOG_INFO(ELOG_MD_COMM, "Communication task started");
    
    while (1) {
        ELOG_DEBUG(ELOG_MD_COMM, "Processing message %lu", msg_count);
        
        // Simulate message processing
        if (msg_count % 5 == 0) {
            ELOG_INFO(ELOG_MD_COMM, "Status: %lu messages processed", msg_count);
        }
        
        msg_count++;
        tx_thread_sleep(TX_MS_TO_TICKS(250));
    }
}

/**
 * @brief Task 3 - Performs UI operations
 * 
 * Even with three tasks logging simultaneously, output is clean and ungarbled
 * thanks to the unified mutex callbacks from common utilities
 */
void ui_task_entry(ULONG arg) {
    uint32_t update_count = 0;
    
    ELOG_INFO(ELOG_MD_UI, "UI task started");
    
    while (1) {
        ELOG_DEBUG(ELOG_MD_UI, "UI update %lu", update_count);
        
        if (update_count % 20 == 0) {
            ELOG_INFO(ELOG_MD_UI, "Display refreshed - %lu updates", update_count);
        }
        
        update_count++;
        tx_thread_sleep(TX_MS_TO_TICKS(100));
    }
}

/* ========================================================================== */
/* Example 3: Per-Module Log Level Configuration */
/* ========================================================================== */

/**
 * @brief Configure different log levels for different modules
 */
void example_per_module_levels(void) {
    ELOG_INFO(ELOG_MD_MAIN, "Configuring per-module log levels");
    
    // Set SENSOR module to DEBUG level
    elog_set_module_threshold(ELOG_MD_SENSOR, ELOG_LEVEL_DEBUG);
    ELOG_INFO(ELOG_MD_MAIN, "SENSOR module: DEBUG level");
    
    // Set COMM module to WARNING level
    elog_set_module_threshold(ELOG_MD_COMM, ELOG_LEVEL_WARNING);
    ELOG_INFO(ELOG_MD_MAIN, "COMM module: WARNING level");
    
    // Set UI module to INFO level
    elog_set_module_threshold(ELOG_MD_UI, ELOG_LEVEL_INFO);
    ELOG_INFO(ELOG_MD_MAIN, "UI module: INFO level");
    
    // Now test with different modules
    ELOG_DEBUG(ELOG_MD_SENSOR, "This DEBUG message will appear (threshold=DEBUG)");
    ELOG_DEBUG(ELOG_MD_COMM, "This DEBUG message will NOT appear (threshold=WARNING)");
    ELOG_INFO(ELOG_MD_UI, "This INFO message will appear (threshold=INFO)");
}

/**
 * @brief Dynamically change module thresholds at runtime
 */
void example_dynamic_threshold_change(void) {
    ELOG_INFO(ELOG_MD_MAIN, "Changing SENSOR threshold to TRACE");
    
    elog_set_module_threshold(ELOG_MD_SENSOR, ELOG_LEVEL_TRACE);
    
    ELOG_TRACE(ELOG_MD_SENSOR, "Now trace messages appear");
    ELOG_DEBUG(ELOG_MD_SENSOR, "Debug still appears too");
    
    // Can also query current threshold
    elog_level_t current = elog_get_module_threshold(ELOG_MD_SENSOR);
    ELOG_INFO(ELOG_MD_MAIN, "Current SENSOR threshold: %u", current);
}

/* ========================================================================== */
/* Example 4: Multiple Subscribers */
/* ========================================================================== */

/**
 * @brief Custom file subscriber (example)
 */
void file_subscriber(elog_level_t level, const char *msg) {
    printf("[FILE_%s] %s\n", elog_level_name(level), msg);
}

/**
 * @brief Custom memory subscriber (example)
 */
void memory_subscriber(elog_level_t level, const char *msg) {
    printf("[MEMORY_%s] %s\n", elog_level_name(level), msg);
}

/**
 * @brief Setup multiple subscribers
 * 
 * All subscribers receive log messages, potentially at different levels
 */
void example_multiple_subscribers(void) {
    // Initialize without subscribers
    LOG_INIT();
    
    // Subscribe console to all levels
    LOG_SUBSCRIBE(elog_console_subscriber, ELOG_LEVEL_DEBUG);
    
    // Subscribe file logger to errors only
    LOG_SUBSCRIBE(file_subscriber, ELOG_LEVEL_ERROR);
    
    // Subscribe memory logger to warnings and above
    LOG_SUBSCRIBE(memory_subscriber, ELOG_LEVEL_WARNING);
    
    ELOG_INFO(ELOG_MD_MAIN, "Configured 3 subscribers");
    
    // These messages will be handled differently by each subscriber
    ELOG_DEBUG(ELOG_MD_MAIN, "Debug - console only");
    ELOG_INFO(ELOG_MD_MAIN, "Info - console only");
    ELOG_WARNING(ELOG_MD_MAIN, "Warning - console and memory");
    ELOG_ERROR(ELOG_MD_MAIN, "Error - console, file, and memory");
}

/* ========================================================================== */
/* Example 5: Error Code Integration */
/* ========================================================================== */

/**
 * @brief Demonstrate logging with error codes
 */
void example_error_codes(void) {
    ELOG_INFO(ELOG_MD_MAIN, "Communication error: 0x%02X", ELOG_COMM_ERR_I2C);
    ELOG_ERROR(ELOG_MD_MAIN, "UART communication failed: 0x%02X", ELOG_COMM_ERR_UART);
    
    ELOG_WARNING(ELOG_MD_MAIN, "Low voltage detected: 0x%02X", ELOG_PWR_ERR_LOW_VOLTAGE);
    
    ELOG_ERROR(ELOG_MD_SENSOR, "Sensor not found: 0x%02X", ELOG_SENSOR_ERR_NOT_FOUND);
    
    ELOG_CRITICAL(ELOG_MD_MAIN, "Stack overflow: 0x%02X", ELOG_CRITICAL_ERR_STACK);
}

/* ========================================================================== */
/* Example 6: Logging with Variable Data */
/* ========================================================================== */

/**
 * @brief Demonstrate logging with formatted data
 */
void example_formatted_logging(void) {
    uint16_t adc_value = 1234;
    float temperature = 25.67;
    uint32_t timestamp = tx_time_get();
    
    ELOG_INFO(ELOG_MD_SENSOR, "ADC reading: %u", adc_value);
    ELOG_INFO(ELOG_MD_SENSOR, "Temperature: %.2f°C", temperature);
    ELOG_DEBUG(ELOG_MD_MAIN, "Timestamp: %lu", timestamp);
    
    // Multiple values in one message
    ELOG_INFO(ELOG_MD_SENSOR, 
             "Sensor data - ADC:%u, Temp:%.2f, Time:%lu",
             adc_value, temperature, timestamp);
}

/* ========================================================================== */
/* Example 7: Legacy Code Compatibility */
/* ========================================================================== */

/**
 * @brief Demonstrate backwards compatibility with old uLog macros
 */
void example_legacy_compatibility(void) {
    // Old uLog-style macros still work
    printTRACE(ELOG_MD_MAIN, "Legacy trace message");
    printLOG(ELOG_MD_MAIN, "Legacy debug message");
    printIF(ELOG_MD_MAIN, "Legacy info message");
    printWRN(ELOG_MD_MAIN, "Legacy warning message");
    printERR(ELOG_MD_MAIN, "Legacy error message");
    printCRITICAL(ELOG_MD_MAIN, "Legacy critical message");
    printALWAYS(ELOG_MD_MAIN, "Legacy always message");
}

/* ========================================================================== */
/* Example 8: ThreadX Integration Pattern */
/* ========================================================================== */

/**
 * @brief Recommended ThreadX initialization pattern
 * 
 * This shows the proper sequence for initializing eLog with ThreadX
 * and unified mutex callbacks from common utilities
 */
void example_threadx_init_pattern(void) {
    // PHASE 1: Hardware initialization (in main.c)
    // ============================================
    // HAL_Init();
    // SystemClock_Config();
    // MX_GPIO_Init();
    // etc.
    
    // Initialize eLog EARLY for debug output
    LOG_INIT_WITH_CONSOLE();
    printLOG(ELOG_MD_DEFAULT, "Hardware initialized");
    
    // PHASE 2: RTOS kernel initialization (in app_entry.c)
    // ====================================================
    // MX_ThreadX_Init();  // Starts RTOS, calls tx_application_define
    
    // PHASE 3: Register unified mutex callbacks (in tx_application_define)
    // ===================================================================
    extern const mutex_callbacks_t mutex_callbacks;
    
    // Register the unified callbacks for eLog
    elog_register_mutex_callbacks(&mutex_callbacks);
    
    // Set RTOS ready flag
    elog_update_RTOS_ready(true);
    
    ELOG_INFO(ELOG_MD_MAIN, "eLog thread-safe mode enabled");
    
    // PHASE 4: Thread execution
    // =========================
    // Threads start, all eLog calls are automatically thread-safe
}

/* ========================================================================== */
/* Example 9: RTOS Status Checking */
/* ========================================================================== */

/**
 * @brief Check RTOS ready status from eLog
 */
void example_check_rtos_status(void) {
    // Check if RTOS is ready for thread-safe operations
    bool rtos_ready = utilities_is_RTOS_ready();
    
    if (rtos_ready) {
        ELOG_INFO(ELOG_MD_MAIN, "RTOS active - thread-safe logging enabled");
    } else {
        ELOG_WARNING(ELOG_MD_MAIN, "RTOS not ready - bare metal mode");
    }
}

/* ========================================================================== */
/* Example 10: Stress Test - Multiple Tasks Logging */
/* ========================================================================== */

volatile uint32_t log_count = 0;

/**
 * @brief High-frequency logging task 1
 */
void logger_task_1(ULONG arg) {
    for (uint32_t i = 0; i < 100; i++) {
        ELOG_DEBUG(ELOG_MD_TASK_A, "Task A iteration %lu", i);
        log_count++;
        tx_thread_sleep(TX_MS_TO_TICKS(10));
    }
}

/**
 * @brief High-frequency logging task 2
 */
void logger_task_2(ULONG arg) {
    for (uint32_t i = 0; i < 100; i++) {
        ELOG_DEBUG(ELOG_MD_TASK_B, "Task B iteration %lu", i);
        log_count++;
        tx_thread_sleep(TX_MS_TO_TICKS(15));
    }
}

/**
 * @brief High-frequency logging task 3
 */
void logger_task_3(ULONG arg) {
    for (uint32_t i = 0; i < 100; i++) {
        ELOG_DEBUG(ELOG_MD_TASK_C, "Task C iteration %lu", i);
        log_count++;
        tx_thread_sleep(TX_MS_TO_TICKS(12));
    }
}

/**
 * @brief Run stress test
 */
void example_logging_stress_test(void) {
    ELOG_INFO(ELOG_MD_MAIN, "Starting logging stress test");
    ELOG_INFO(ELOG_MD_MAIN, "300 log messages from 3 simultaneous tasks");
    ELOG_INFO(ELOG_MD_MAIN, "Output will be clean and ungarbled via unified mutex");
    
    // In real application, these would be separate ThreadX threads
    // All would log simultaneously, but output remains clean due to
    // unified mutex callbacks from common utilities
}

/* ========================================================================== */
/* Example 11: Compile-Time Configuration */
/* ========================================================================== */

/**
 * @brief Demonstrate compile-time optimization
 * 
 * With compile-time flags, disabled log levels generate zero code:
 * 
 * In eLog.h:
 * #define ELOG_DEBUG_TRACE_ON NO      // Trace generates NO code
 * #define ELOG_DEBUG_DEBUG_ON YES     // Debug generates code
 * #define ELOG_DEBUG_INFO_ON YES      // Info generates code
 * 
 * Then this trace line generates ZERO code:
 */
void example_compile_time_optimization(void) {
    // If ELOG_DEBUG_TRACE_ON is set to NO, this generates zero code
    ELOG_TRACE(ELOG_MD_MAIN, "This trace may be compiled out (no code)");
    
    // These always generate code (if their flags are YES)
    ELOG_DEBUG(ELOG_MD_MAIN, "Debug message (code generated)");
    ELOG_INFO(ELOG_MD_MAIN, "Info message (code generated)");
}

/* ========================================================================== */
/* Example 12: Full Application Logging Pattern */
/* ========================================================================== */

/**
 * @brief Complete realistic application pattern
 */
void example_full_app_pattern(void) {
    // Initial startup message
    ELOG_INFO(ELOG_MD_MAIN, "=== Application Started ===");
    
    // System status
    ELOG_INFO(ELOG_MD_MAIN, "RTOS Ready: %s", 
              utilities_is_RTOS_ready() ? "YES" : "NO");
    
    // Module-specific logging
    ELOG_INFO(ELOG_MD_SENSOR, "Sensor module initialized");
    ELOG_INFO(ELOG_MD_COMM, "Communication module initialized");
    ELOG_INFO(ELOG_MD_UI, "UI module initialized");
    
    // Debug information
    ELOG_DEBUG(ELOG_MD_MAIN, "Build info: version 1.0.0");
    
    // Application ready
    ELOG_INFO(ELOG_MD_MAIN, "=== Application Ready ===");
}

/* ========================================================================== */
/* Key Takeaways */
/* ========================================================================== */

/**
 * @brief Summary of eLog + common utilities integration
 * 
 * 1. eLog uses unified mutex callbacks registered via common utilities
 * 
 * 2. All eLog operations are automatically thread-safe
 * 
 * 3. No explicit synchronization needed in your code
 * 
 * 4. Multiple tasks can log simultaneously without contention
 * 
 * 5. Per-module log levels for fine-grained control
 * 
 * 6. Compile-time optimization eliminates disabled log levels
 * 
 * 7. Backwards compatible with legacy uLog macros
 * 
 * 8. Graceful fallback to bare-metal mode if RTOS not available
 * 
 * 9. Zero overhead when RTOS not ready or logging disabled
 * 
 * 10. Clean, ungarbled output even with many simultaneous tasks
 */

#endif  // ELOG_EXAMPLE_COMMON_C_
