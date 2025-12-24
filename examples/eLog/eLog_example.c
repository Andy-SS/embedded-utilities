/***********************************************************
 * @file	eLog_example.c
 * @author	Andy Chen (clgm216@gmail.com)
 * @version	0.05
 * @date	2024-09-10
 * @brief  Enhanced logging system usage examples
 *         Demonstrates both legacy and enhanced logging APIs
 * **********************************************************
 * @copyright Copyright (c) 2025 TTK. All rights reserved.
 *
 ************************************************************/

#include "eLog.h"
#include <stdio.h>

/* ========================================================================== */
/* Enhanced Logging Examples */
/* ========================================================================== */

/**
 * @brief  Demonstrate per-module log threshold usage (O(1) direct indexing)
 * @note   Module threshold lookup is optimized via direct array indexing
 * @retval None
 */
void perModuleThresholdExample(void) {
    // Set log threshold for this module - instant O(1) operation
    elog_set_module_threshold(ELOG_MD_MAIN, ELOG_LEVEL_DEBUG);

    ELOG_INFO(ELOG_MD_MAIN, "This info message will be shown if threshold allows");
    ELOG_DEBUG(ELOG_MD_MAIN, "This debug message will be shown due to per-module threshold");
    ELOG_TRACE(ELOG_MD_MAIN, "This trace message will NOT be shown (threshold too high)");
}

/**
 * @brief  Demonstrate basic enhanced logging usage
 * @retval None
 */
void enhancedLoggingBasicExample(void) {
  LOG_INIT_WITH_CONSOLE_AUTO();

  ELOG_INFO(ELOG_MD_MAIN, "Enhanced logging system initialized successfully");
  ELOG_DEBUG(ELOG_MD_MAIN, "Debug information: value=%d, pointer=%p", 42, (void *)0x12345678);
  ELOG_WARNING(ELOG_MD_MAIN, "This is a warning message");
  ELOG_ERROR(ELOG_MD_MAIN, "Error occurred: code=0x%02X", 0xAB);
  ELOG_CRITICAL(ELOG_MD_MAIN, "Critical system failure detected!");
  ELOG_ALWAYS(ELOG_MD_MAIN, "This message is always logged");

  ELOG_INFO_STR(ELOG_MD_MAIN, "Simple info message");
  ELOG_ERROR_STR(ELOG_MD_MAIN, "Simple error message");
}

/**
 * @brief  Demonstrate legacy logging compatibility
 * @retval None
 */
void legacyLoggingExample(void) {
  printIF(ELOG_MD_MAIN, "Information message using legacy printIF");
  printLOG(ELOG_MD_MAIN, "Debug message using legacy printLOG: value=%d", 123);
  printWRN(ELOG_MD_MAIN, "Warning message using legacy printWRN");
  printERR(ELOG_MD_MAIN, "Error message using legacy printERR: status=0x%04X", 0x1234);

  printIF_STR(ELOG_MD_MAIN, "Simple info using legacy printIF_STR");
  printERR_STR(ELOG_MD_MAIN, "Simple error using legacy printERR_STR");
}

/**
 * @brief  Custom subscriber example
 * @param  level: Log level
 * @param  msg: Formatted message
 */
void customFileSubscriber(elog_level_t level, const char *msg) {
  printf("[FILE] %s: %s\n", elog_level_name(level), msg);
}

/**
 * @brief  Custom memory buffer subscriber
 * @param  level: Log level
 * @param  msg: Formatted message
 */
void customMemorySubscriber(elog_level_t level, const char *msg) {
  static int message_count = 0;
  message_count++;
  printf("[MEM #%d] %s: %s\n", message_count, elog_level_name(level), msg);
}

/**
 * @brief  Demonstrate multiple subscribers (simplified management)
 * @note   Subscriber entries are actively removed on unsubscribe (no 'soft delete')
 * @retval None
 */
void multipleSubscribersExample(void) {
  LOG_INIT();

  // Subscribe multiple outputs at different thresholds
  LOG_SUBSCRIBE(elog_console_subscriber, ELOG_LEVEL_DEBUG);
  LOG_SUBSCRIBE(customFileSubscriber, ELOG_LEVEL_DEBUG);
  LOG_SUBSCRIBE(customMemorySubscriber, ELOG_LEVEL_ERROR);

  ELOG_INFO(ELOG_MD_MAIN, "=== Multiple Subscribers Demo ===");

  // Demonstrate threshold-based filtering
  ELOG_TRACE(ELOG_MD_MAIN, "This trace message won't appear anywhere (threshold too low)");
  ELOG_DEBUG(ELOG_MD_MAIN, "This debug message goes to console and file");
  ELOG_INFO(ELOG_MD_MAIN, "This info message goes to console and file");
  ELOG_WARNING(ELOG_MD_MAIN, "This warning goes to console and file");
  ELOG_ERROR(ELOG_MD_MAIN, "This error goes to console, file, and memory");
  ELOG_CRITICAL(ELOG_MD_MAIN, "This critical message goes everywhere");

  ELOG_INFO_STR(ELOG_MD_MAIN, "=== End Multiple Subscribers Demo ===");
}

/**
 * @brief  Demonstrate automatic threshold calculation
 * @retval None
 */
void autoThresholdExample(void) {
  LOG_INIT_WITH_CONSOLE_AUTO();

  ELOG_INFO(ELOG_MD_MAIN, "=== Auto Threshold Demo ===");

  elog_level_t threshold = elog_get_auto_threshold();
  ELOG_INFO(ELOG_MD_MAIN, "Current auto-threshold: %s (%d)", elog_level_name(threshold), threshold);

  ELOG_INFO(ELOG_MD_MAIN, "Based on debug flags, console subscriber will receive:");

#if (ELOG_DEBUG_TRACE_ON == YES)
  ELOG_INFO(ELOG_MD_MAIN, "- TRACE messages (ELOG_DEBUG_TRACE_ON=YES)");
#else
  ELOG_INFO(ELOG_MD_MAIN, "- No TRACE messages (ELOG_DEBUG_TRACE_ON=NO)");
#endif

#if (ELOG_DEBUG_LOG_ON == YES)
  ELOG_INFO(ELOG_MD_MAIN, "- DEBUG messages (ELOG_DEBUG_LOG_ON=YES)");
#else
  ELOG_INFO(ELOG_MD_MAIN, "- No DEBUG messages (ELOG_DEBUG_LOG_ON=NO)");
#endif

#if (ELOG_DEBUG_INFO_ON == YES)
  ELOG_INFO(ELOG_MD_MAIN, "- INFO messages (ELOG_DEBUG_INFO_ON=YES)");
#else
  ELOG_INFO(ELOG_MD_MAIN, "- No INFO messages (ELOG_DEBUG_INFO_ON=NO)");
#endif

#if (ELOG_DEBUG_WARN_ON == YES)
  ELOG_INFO(ELOG_MD_MAIN, "- WARNING messages (ELOG_DEBUG_WARN_ON=YES)");
#else
  ELOG_INFO(ELOG_MD_MAIN, "- No WARNING messages (ELOG_DEBUG_WARN_ON=NO)");
#endif

#if (ELOG_DEBUG_ERR_ON == YES)
  ELOG_INFO(ELOG_MD_MAIN, "- ERROR messages (ELOG_DEBUG_ERR_ON=YES)");
#else
  ELOG_INFO(ELOG_MD_MAIN, "- No ERROR messages (ELOG_DEBUG_ERR_ON=NO)");
#endif

#if (ELOG_DEBUG_CRITICAL_ON == YES)
  ELOG_INFO(ELOG_MD_MAIN, "- CRITICAL messages (ELOG_DEBUG_CRITICAL_ON=YES)");
#else
  ELOG_INFO(ELOG_MD_MAIN, "- No CRITICAL messages (ELOG_DEBUG_CRITICAL_ON=NO)");
#endif

#if (ELOG_DEBUG_ALWAYS_ON == YES)
  ELOG_INFO(ELOG_MD_MAIN, "- ALWAYS messages (ELOG_DEBUG_ALWAYS_ON=YES)");
#else
  ELOG_INFO(ELOG_MD_MAIN, "- No ALWAYS messages (ELOG_DEBUG_ALWAYS_ON=NO)");
#endif

  ELOG_INFO_STR(ELOG_MD_MAIN, "=== End Auto Threshold Demo ===");
}

/**
 * @brief  Demonstrate performance comparison
 * @retval None
 */
void performanceDemo(void) {
  LOG_INIT_WITH_CONSOLE_AUTO();

  ELOG_INFO(ELOG_MD_MAIN, "=== Performance Demo ===");

  ELOG_INFO(ELOG_MD_MAIN, "Active logging levels are optimized at compile time");

#if (ELOG_DEBUG_TRACE_ON == YES)
  ELOG_TRACE(ELOG_MD_MAIN, "TRACE is enabled - this message has runtime cost");
#else
  ELOG_TRACE(ELOG_MD_MAIN, "TRACE is disabled - this line compiles to: do {} while(0)");
#endif

  ELOG_INFO(ELOG_MD_MAIN, "Legacy macros also benefit from compile-time optimization:");

#if (ELOG_DEBUG_LOG_ON == YES)
  printLOG(ELOG_MD_MAIN, "printLOG is enabled - uses ELOG_DEBUG internally");
#else
  printLOG(ELOG_MD_MAIN, "printLOG is disabled - compiles to empty macro");
#endif

  ELOG_INFO_STR(ELOG_MD_MAIN, "=== End Performance Demo ===");
}

/**
 * @brief  Demonstrate unified debug flag control
 * @retval None
 */
void unifiedDebugControlDemo(void) {
  LOG_INIT_WITH_CONSOLE_AUTO();

  ELOG_INFO(ELOG_MD_MAIN, "=== Unified Debug Control Demo ===");
  ELOG_INFO(ELOG_MD_MAIN, "Single debug flags control both legacy and enhanced logging:");

  ELOG_INFO(ELOG_MD_MAIN, "Enhanced API: This uses ELOG_INFO (ELOG_DEBUG_INFO_ON flag)");
  printIF(ELOG_MD_MAIN, "Legacy API: This uses printIF (same ELOG_DEBUG_INFO_ON flag)");

  ELOG_ERROR(ELOG_MD_MAIN, "Enhanced API: This uses ELOG_ERROR (ELOG_DEBUG_ERR_ON flag)");
  printERR(ELOG_MD_MAIN, "Legacy API: This uses printERR (same ELOG_DEBUG_ERR_ON flag)");

  ELOG_DEBUG(ELOG_MD_MAIN, "Enhanced API: This uses ELOG_DEBUG (ELOG_DEBUG_LOG_ON flag)");
  printLOG(ELOG_MD_MAIN, "Legacy API: This uses printLOG (same ELOG_DEBUG_LOG_ON flag)");

  ELOG_INFO(ELOG_MD_MAIN, "Result: Consistent behavior between legacy and enhanced APIs");
  ELOG_INFO_STR(ELOG_MD_MAIN, "=== End Unified Debug Control Demo ===");
}

/**
 * @brief  Demonstrate subscriber management
 * @retval None
 */
void subscriberManagementDemo(void) {
  LOG_INIT();

  LOG_SUBSCRIBE_CONSOLE();
  ELOG_INFO(ELOG_MD_MAIN, "Console subscriber added");

  LOG_SUBSCRIBE(customMemorySubscriber, ELOG_LEVEL_DEBUG);
  ELOG_WARNING(ELOG_MD_MAIN, "Memory subscriber added - you should see this in both console and memory");

  LOG_UNSUBSCRIBE(customMemorySubscriber);
  ELOG_WARNING(ELOG_MD_MAIN, "Memory subscriber removed - you should only see this in console");

  ELOG_INFO_STR(ELOG_MD_MAIN, "Subscriber management demo complete");
}

/**
 * @brief  Complete demonstration of enhanced logging features
 * @retval None
 */
void completeLoggingDemo(void) {
  printf("\n" LOG_COLOR(LOG_COLOR_CYAN) "==========================================\n");
  printf("    Enhanced Logging System Demo\n");
  printf("==========================================" LOG_RESET_COLOR "\n\n");

  enhancedLoggingBasicExample();
  printf("\n");

  legacyLoggingExample();
  printf("\n");

  autoThresholdExample();
  printf("\n");

  unifiedDebugControlDemo();
  printf("\n");

  performanceDemo();
  printf("\n");

  subscriberManagementDemo();
  printf("\n");

  multipleSubscribersExample();
  printf("\n");

  perModuleThresholdExample();
  printf("\n");

  printf(LOG_COLOR(LOG_COLOR_GREEN) "==========================================\n");
  printf("    Enhanced Logging Demo Complete!\n");
  printf("==========================================" LOG_RESET_COLOR "\n\n");
}

/**
 * @brief  Simple initialization example for real applications
 * @retval None
 */
void simpleAppInitializationExample(void) {
  LOG_INIT_WITH_CONSOLE_AUTO();

  ELOG_INFO(ELOG_MD_MAIN, "Application started successfully");
  printIF(ELOG_MD_MAIN, "Legacy logging also works");
}

/**
 * @brief Demonstrate RTOS readiness update
 * @retval None
 */
void rtosReadinessExample(void) {
    elog_update_RTOS_ready(true);
    ELOG_INFO(ELOG_MD_MAIN, "RTOS is now ready for logging");
}

/**
 * @brief Demonstrate unified error codes organized by subsystem (0x00-0xFF)
 * @retval None
 */
void unifiedErrorCodesExample(void) {
  printf("\n=== Unified Error Codes by Subsystem (0x00-0xFF) ===\n");
  LOG_INIT_WITH_CONSOLE_AUTO();

  /* Logging System Errors (0x00-0x0F) */
  ELOG_INFO(ELOG_MD_MAIN, "--- Logging System Errors (0x00-0x0F) ---");
  ELOG_ERROR(ELOG_MD_MAIN, "Invalid log level: 0x%02X", ELOG_ERR_INVALID_LEVEL);
  ELOG_ERROR(ELOG_MD_MAIN, "Subscribers exceeded: 0x%02X", ELOG_ERR_SUBSCRIBERS_EXCEEDED);
  ELOG_WARNING(ELOG_MD_MAIN, "Not subscribed: 0x%02X", ELOG_ERR_NOT_SUBSCRIBED);

  /* System Errors (0x10-0x1F) */
  ELOG_INFO(ELOG_MD_MAIN, "--- System Errors (0x10-0x1F) ---");
  ELOG_ERROR(ELOG_MD_MAIN, "System initialization failed: 0x%02X", ELOG_SYS_ERR_INIT);
  ELOG_ERROR(ELOG_MD_MAIN, "Memory allocation failed: 0x%02X", ELOG_SYS_ERR_MEMORY);
  ELOG_ERROR(ELOG_MD_MAIN, "Configuration error: 0x%02X", ELOG_SYS_ERR_CONFIG);

  /* Communication Errors (0x20-0x3F) */
  ELOG_INFO(ELOG_MD_COMM, "--- Communication Errors (0x20-0x3F) ---");
  ELOG_WARNING(ELOG_MD_COMM, "UART timeout: 0x%02X", ELOG_COMM_ERR_UART);
  ELOG_ERROR(ELOG_MD_COMM, "I2C bus error: 0x%02X", ELOG_COMM_ERR_I2C);
  ELOG_ERROR(ELOG_MD_COMM, "Checksum error: 0x%02X", ELOG_COMM_ERR_CHECKSUM);
  ELOG_WARNING(ELOG_MD_COMM, "Buffer overrun: 0x%02X", ELOG_COMM_ERR_OVERRUN);

  /* Sensor Errors (0x40-0x5F) */
  ELOG_INFO(ELOG_MD_SENSOR, "--- Sensor Errors (0x40-0x5F) ---");
  ELOG_WARNING(ELOG_MD_SENSOR, "Sensor not found: 0x%02X", ELOG_SENSOR_ERR_NOT_FOUND);
  ELOG_ERROR(ELOG_MD_SENSOR, "Accelerometer error: 0x%02X", ELOG_ACCEL_ERR);
  ELOG_WARNING(ELOG_MD_SENSOR, "Sensor range exceeded: 0x%02X", ELOG_SENSOR_ERR_RANGE);
  ELOG_ERROR(ELOG_MD_SENSOR, "Gyroscope calibration failed: 0x%02X", ELOG_GYRO_ERR);

  /* Power Management Errors (0x60-0x7F) */
  ELOG_INFO(ELOG_MD_MAIN, "--- Power Management Errors (0x60-0x7F) ---");
  ELOG_CRITICAL(ELOG_MD_MAIN, "Low voltage detected: 0x%02X", ELOG_PWR_ERR_LOW_VOLTAGE);
  ELOG_ERROR(ELOG_MD_MAIN, "Overcurrent detected: 0x%02X", ELOG_PWR_ERR_OVERCURRENT);
  ELOG_ERROR(ELOG_MD_MAIN, "Thermal shutdown: 0x%02X", ELOG_PWR_ERR_THERMAL);

  /* Storage Errors (0x80-0x9F) */
  ELOG_INFO(ELOG_MD_MAIN, "--- Storage Errors (0x80-0x9F) ---");
  ELOG_ERROR(ELOG_MD_MAIN, "Flash write failed: 0x%02X", ELOG_STORAGE_ERR_WRITE);
  ELOG_ERROR(ELOG_MD_MAIN, "Storage full: 0x%02X", ELOG_STORAGE_ERR_FULL);
  ELOG_WARNING(ELOG_MD_MAIN, "Flash read timeout: 0x%02X", ELOG_STORAGE_ERR_READ);

  /* RTOS Errors (0xE0-0xEF) */
  ELOG_INFO(ELOG_MD_MAIN, "--- RTOS Errors (0xE0-0xEF) ---");
  ELOG_ERROR(ELOG_MD_MAIN, "Task creation failed: 0x%02X", ELOG_RTOS_ERR_TASK);
  ELOG_ERROR(ELOG_MD_MAIN, "Mutex operation failed: 0x%02X", ELOG_RTOS_ERR_MUTEX);
  ELOG_ERROR(ELOG_MD_MAIN, "Semaphore operation failed: 0x%02X", ELOG_RTOS_ERR_SEMAPHORE);
  ELOG_WARNING(ELOG_MD_MAIN, "Queue overflow detected: 0x%02X", ELOG_RTOS_ERR_QUEUE);

  /* Critical System Errors (0xF0-0xFF) */
  ELOG_INFO(ELOG_MD_MAIN, "--- Critical System Errors (0xF0-0xFF) ---");
  ELOG_CRITICAL(ELOG_MD_MAIN, "Stack overflow detected: 0x%02X", ELOG_CRITICAL_ERR_STACK);
  ELOG_CRITICAL(ELOG_MD_MAIN, "Hard fault exception: 0x%02X", ELOG_CRITICAL_ERR_HARDFAULT);
  ELOG_CRITICAL(ELOG_MD_MAIN, "Heap corruption: 0x%02X", ELOG_CRITICAL_ERR_HEAP);
  ELOG_CRITICAL(ELOG_MD_MAIN, "Assertion failure: 0x%02X", ELOG_CRITICAL_ERR_ASSERT);

  printf("Unified error codes demonstration complete.\n");
}

/* Updated example usage for eLog */
int main() {
    completeLoggingDemo();
    return 0;
}
