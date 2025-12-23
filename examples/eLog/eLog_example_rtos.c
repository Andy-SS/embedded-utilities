/***********************************************************
* @file	eLog_example_rtos.c
* @author	Andy Chen (clgm216@gmail.com)
* @version	0.05
* @date	2025-12-02
* @brief  Enhanced logging system examples with RTOS threading support
* **********************************************************
* @copyright Copyright (c) 2025 TTK. All rights reserved.
* 
************************************************************/

#include "eLog.h"
#include <stdio.h>

/* ========================================================================== */
/* Example Custom Subscribers */
/* ========================================================================== */

void file_subscriber(elog_level_t level, const char *msg) {
  printf("FILE[%s]: %s\n", elog_level_name(level), msg);
}

void memory_subscriber(elog_level_t level, const char *msg) {
  printf("MEM[%s]: %s\n", elog_level_name(level), msg);
}

void network_subscriber(elog_level_t level, const char *msg) {
  printf("NET[%s]: %s\n", elog_level_name(level), msg);
}

/* ========================================================================== */
/* Basic Examples */
/* ========================================================================== */

void basic_logging_example(void) {
  printf("\n=== Basic Logging Example ===\n");
  LOG_INIT_WITH_CONSOLE_AUTO();

  ELOG_TRACE(ELOG_MD_MAIN, "This is a trace message");
  ELOG_DEBUG(ELOG_MD_MAIN, "Debug: Variable x = %d", 42);
  ELOG_INFO(ELOG_MD_MAIN, "System initialization completed");
  ELOG_WARNING(ELOG_MD_MAIN, "Low memory warning: %d%% used", 85);
  ELOG_ERROR(ELOG_MD_MAIN, "Communication error: code 0x%02X", ELOG_COMM_ERR_UART);
  ELOG_CRITICAL(ELOG_MD_MAIN, "Critical system failure!");
  ELOG_ALWAYS(ELOG_MD_MAIN, "System startup message");

  printf("Basic logging complete.\n");
}

/* ========================================================================== */
/* Per-Module Log Threshold Example */
/* ========================================================================== */

void per_module_threshold_example(void) {
  printf("\n=== Per-Module Log Threshold Example ===\n");
  elog_set_module_threshold(ELOG_MD_MAIN, ELOG_LEVEL_WARNING);

  ELOG_INFO(ELOG_MD_MAIN, "This info message will NOT be shown (threshold too high)");
  ELOG_WARNING(ELOG_MD_MAIN, "This warning message WILL be shown");
  ELOG_ERROR(ELOG_MD_MAIN, "This error message WILL be shown");
  ELOG_CRITICAL(ELOG_MD_MAIN, "This critical message WILL be shown");

  printf("Per-module log threshold demonstration complete.\n");
}

/* ========================================================================== */
/* Multiple Subscribers Example */
/* ========================================================================== */

void multiple_subscribers_example(void) {
  printf("\n=== Multiple Subscribers Example ===\n");
  LOG_INIT();

  LOG_SUBSCRIBE(elog_console_subscriber, ELOG_LEVEL_DEBUG);
  LOG_SUBSCRIBE(file_subscriber, ELOG_LEVEL_DEBUG);
  LOG_SUBSCRIBE(memory_subscriber, ELOG_LEVEL_ERROR);

  ELOG_DEBUG(ELOG_MD_MAIN, "Debug message - only console should see this");
  ELOG_INFO(ELOG_MD_MAIN, "Info message - console should see this");
  ELOG_WARNING(ELOG_MD_MAIN, "Warning message - console and file should see this");
  ELOG_ERROR(ELOG_MD_MAIN, "Error message - all subscribers should see this");

  printf("Multiple subscribers complete.\n");
}

/* ========================================================================== */
/* Subscriber Management Example */
/* ========================================================================== */

void subscriber_management_example(void) {
  printf("\n=== Subscriber Management Example ===\n");
  LOG_INIT();

  LOG_SUBSCRIBE(elog_console_subscriber, ELOG_LEVEL_DEBUG);
  LOG_SUBSCRIBE(network_subscriber, ELOG_LEVEL_ERROR);

  ELOG_ERROR(ELOG_MD_MAIN, "Error before unsubscribing network");

  LOG_UNSUBSCRIBE(network_subscriber);

  ELOG_ERROR(ELOG_MD_MAIN, "Error after unsubscribing network - should only go to console");

  printf("Subscriber management complete.\n");
}

/* ========================================================================== */
/* Error Codes Example */
/* ========================================================================== */

void error_codes_example(void) {
  printf("\n=== Unified Error Codes Example (0x00-0xFF) ===\n");
  LOG_INIT_WITH_CONSOLE_AUTO();

  /* Logging System Errors (0x00-0x0F) */
  ELOG_INFO(ELOG_MD_MAIN, "--- Logging System Errors (0x00-0x0F) ---");
  ELOG_ERROR(ELOG_MD_MAIN, "Invalid log level: 0x%02X", ELOG_ERR_INVALID_LEVEL);
  ELOG_ERROR(ELOG_MD_MAIN, "Subscribers exceeded: 0x%02X", ELOG_ERR_SUBSCRIBERS_EXCEEDED);
  ELOG_WARNING(ELOG_MD_MAIN, "Not subscribed: 0x%02X", ELOG_ERR_NOT_SUBSCRIBED);

  /* System Errors (0x10-0x1F) */
  ELOG_INFO(ELOG_MD_MAIN, "--- System Errors (0x10-0x1F) ---");
  ELOG_ERROR(ELOG_MD_MAIN, "System init failed: 0x%02X", ELOG_SYS_ERR_INIT);
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
  ELOG_WARNING(ELOG_MD_SENSOR, "Sensor range error: 0x%02X", ELOG_SENSOR_ERR_RANGE);
  ELOG_ERROR(ELOG_MD_SENSOR, "Gyroscope calibration failed: 0x%02X", ELOG_GYRO_ERR);

  /* Power Management Errors (0x60-0x7F) */
  ELOG_INFO(ELOG_MD_MAIN, "--- Power Management Errors (0x60-0x7F) ---");
  ELOG_CRITICAL(ELOG_MD_MAIN, "Low voltage detected: 0x%02X", ELOG_PWR_ERR_LOW_VOLTAGE);
  ELOG_ERROR(ELOG_MD_MAIN, "Overcurrent detected: 0x%02X", ELOG_PWR_ERR_OVERCURRENT);
  ELOG_ERROR(ELOG_MD_MAIN, "Thermal shutdown: 0x%02X", ELOG_PWR_ERR_THERMAL);

  /* Storage Errors (0x80-0x9F) */
  ELOG_INFO(ELOG_MD_MAIN, "--- Storage Errors (0x80-0x9F) ---");
  ELOG_ERROR(ELOG_MD_MAIN, "Flash write error: 0x%02X", ELOG_STORAGE_ERR_WRITE);
  ELOG_ERROR(ELOG_MD_MAIN, "Storage full: 0x%02X", ELOG_STORAGE_ERR_FULL);
  ELOG_WARNING(ELOG_MD_MAIN, "Flash read timeout: 0x%02X", ELOG_STORAGE_ERR_READ);

  /* RTOS Errors (0xE0-0xEF) */
  ELOG_INFO(ELOG_MD_MAIN, "--- RTOS Errors (0xE0-0xEF) ---");
  ELOG_ERROR(ELOG_MD_MAIN, "Task creation failed: 0x%02X", ELOG_RTOS_ERR_TASK);
  ELOG_ERROR(ELOG_MD_MAIN, "Mutex error: 0x%02X", ELOG_RTOS_ERR_MUTEX);
  ELOG_ERROR(ELOG_MD_MAIN, "Semaphore error: 0x%02X", ELOG_RTOS_ERR_SEMAPHORE);
  ELOG_WARNING(ELOG_MD_MAIN, "Queue overflow: 0x%02X", ELOG_RTOS_ERR_QUEUE);

  /* Critical System Errors (0xF0-0xFF) */
  ELOG_INFO(ELOG_MD_MAIN, "--- Critical System Errors (0xF0-0xFF) ---");
  ELOG_CRITICAL(ELOG_MD_MAIN, "Stack overflow detected: 0x%02X", ELOG_CRITICAL_ERR_STACK);
  ELOG_CRITICAL(ELOG_MD_MAIN, "Hard fault: 0x%02X", ELOG_CRITICAL_ERR_HARDFAULT);
  ELOG_CRITICAL(ELOG_MD_MAIN, "Heap corruption: 0x%02X", ELOG_CRITICAL_ERR_HEAP);
  ELOG_CRITICAL(ELOG_MD_MAIN, "Assertion failure: 0x%02X", ELOG_CRITICAL_ERR_ASSERT);

  printf("Unified error codes demonstration complete.\n");
}

/* ========================================================================== */
/* Legacy Compatibility Example */
/* ========================================================================== */

void legacy_compatibility_example(void) {
  printf("\n=== Legacy Compatibility Example ===\n");
  LOG_INIT_WITH_CONSOLE_AUTO();

  printIF(ELOG_MD_MAIN, "Legacy info message: %s", "system ready");
  printERR(ELOG_MD_MAIN, "Legacy error: code %d", 404);
  printLOG(ELOG_MD_MAIN, "Legacy debug: value = %d", 42);
  printWRN(ELOG_MD_MAIN, "Legacy warning: %s", "low battery");
  printCRITICAL(ELOG_MD_MAIN, "Legacy critical: %s", "system failure");
  printALWAYS(ELOG_MD_MAIN, "Legacy always: %s", "important message");

  printf("Legacy compatibility complete.\n");
}

/* ========================================================================== */
/* RTOS Threading Examples */
/* ========================================================================== */

#if (ELOG_THREAD_SAFE == 1)

void thread_safety_example(void) {
  printf("\n=== Thread Safety Example ===\n");
  LOG_INIT_WITH_CONSOLE_AUTO();

  ELOG_INFO(ELOG_MD_MAIN, "Thread safety is enabled (ELOG_THREAD_SAFE=%d)", ELOG_THREAD_SAFE);
  ELOG_INFO(ELOG_MD_MAIN, "RTOS type: %d", ELOG_RTOS_TYPE);
  ELOG_INFO(ELOG_MD_MAIN, "Current task: %s (ID: 0x%08X)", elog_get_task_name(), (unsigned int)elog_get_task_id());

  elog_err_t result = log_subscribe_safe(memory_subscriber, ELOG_LEVEL_DEBUG);
  if (result == ELOG_ERR_NONE) {
    ELOG_INFO(ELOG_MD_MAIN, "Successfully subscribed memory subscriber in thread-safe mode");
  } else {
    ELOG_ERROR(ELOG_MD_MAIN, "Failed to subscribe memory subscriber: %d", result);
  }

  ELOG_WARNING(ELOG_MD_MAIN, "This message should go to both console and memory subscribers");

  printf("Thread safety demonstration complete.\n");
}

void thread_aware_logging_example(void) {
  printf("\n=== Thread-Aware Logging Example ===\n");
  LOG_INIT();
  LOG_SUBSCRIBE(elog_console_subscriber_with_thread, ELOG_LEVEL_DEBUG);

  ELOG_DEBUG(ELOG_MD_MAIN, "This message includes task name in output");
  ELOG_INFO(ELOG_MD_MAIN, "Task information: %s", elog_get_task_name());
  ELOG_WARNING(ELOG_MD_MAIN, "Multi-threaded logging demonstration");

  printf("Thread-aware logging complete.\n");
}

void simulated_multitask_example(void) {
  printf("\n=== Simulated Multi-Task Example ===\n");
  LOG_INIT_WITH_THREAD_INFO();

  ELOG_INFO(ELOG_MD_TASK_A, "Task A: Starting sensor initialization");
  ELOG_DEBUG(ELOG_MD_TASK_A, "Task A: I2C bus configured");
  ELOG_INFO(ELOG_MD_TASK_A, "Task A: Sensors online");
  ELOG_WARNING(ELOG_MD_TASK_B, "Task B: Communication timeout on UART");
  ELOG_ERROR(ELOG_MD_TASK_C, "Task C: Memory allocation failed in data processing");
  ELOG_INFO(ELOG_MD_TASK_A, "Task A: Sensor data ready");
  ELOG_INFO(ELOG_MD_TASK_B, "Task B: Retrying communication");
  ELOG_INFO(ELOG_MD_TASK_B, "Task B: Communication restored");

  printf("Simulated multi-task demonstration complete.\n");
}

void rtos_features_example(void) {
  printf("\n=== RTOS Features Example ===\n");
  LOG_INIT_WITH_CONSOLE_AUTO();

  ELOG_INFO(ELOG_MD_MAIN, "Testing RTOS integration features:");

#if (ELOG_RTOS_TYPE == ELOG_RTOS_FREERTOS)
  ELOG_INFO(ELOG_MD_MAIN, "- FreeRTOS integration enabled");
  ELOG_INFO(ELOG_MD_MAIN, "- Mutex timeout: %d ticks", ELOG_MUTEX_TIMEOUT_MS);
#elif (ELOG_RTOS_TYPE == ELOG_RTOS_THREADX)
  ELOG_INFO(ELOG_MD_MAIN, "- ThreadX integration enabled");  
  ELOG_INFO(ELOG_MD_MAIN, "- Mutex timeout: %d ticks", ELOG_MUTEX_TIMEOUT_MS);
#elif (ELOG_RTOS_TYPE == ELOG_RTOS_CMSIS)
  ELOG_INFO(ELOG_MD_MAIN, "- CMSIS-RTOS integration enabled");
  ELOG_INFO(ELOG_MD_MAIN, "- Mutex timeout: %d ms", ELOG_MUTEX_TIMEOUT_MS);
#else
  ELOG_INFO(ELOG_MD_MAIN, "- Bare metal mode (no RTOS)");
#endif

  const char *task_name = elog_get_task_name();
  uint32_t task_id = elog_get_task_id();

  ELOG_INFO(ELOG_MD_MAIN, "Current task: %s", task_name);
  ELOG_INFO(ELOG_MD_MAIN, "Task ID: 0x%08X", (unsigned int)task_id);

  elog_err_t result = log_subscribe_safe(file_subscriber, ELOG_LEVEL_DEBUG);
  ELOG_INFO(ELOG_MD_MAIN, "Subscribe result: %d", result);

  ELOG_WARNING(ELOG_MD_MAIN, "Test message to new subscriber");

  result = log_unsubscribe_safe(file_subscriber);
  ELOG_INFO(ELOG_MD_MAIN, "Unsubscribe result: %d", result);

  printf("RTOS features demonstration complete.\n");
}

#endif /* ELOG_THREAD_SAFE */

/* ========================================================================== */
/* Performance and Configuration Examples */
/* ========================================================================== */

void performance_test_example(void) {
  printf("\n=== Performance Test Example ===\n");
  LOG_INIT_WITH_CONSOLE_AUTO();

  printf("Testing logging performance...\n");
  for (int i = 0; i < 10; i++) {
    ELOG_DEBUG(ELOG_MD_MAIN, "Performance test iteration %d", i);
  }

  ELOG_INFO(ELOG_MD_MAIN, "Short");
  ELOG_INFO(ELOG_MD_MAIN, "Medium length message with some data: %d", 12345);
  ELOG_INFO(ELOG_MD_MAIN, "Longer message with multiple parameters: %d, %s, 0x%08X", 42, "test", 0xDEADBEEF);

#if (ELOG_THREAD_SAFE == 1)
  printf("Testing thread-safe logging performance...\n");
  for (int i = 0; i < 5; i++) {
    elog_message_safe(ELOG_LEVEL_DEBUG, "Thread-safe performance test %d", i);
  }
#endif

  printf("Performance test complete.\n");
}

void configuration_showcase(void) {
  printf("\n=== Configuration Showcase ===\n");
  LOG_INIT();
  LOG_SUBSCRIBE_CONSOLE();

  ELOG_INFO(ELOG_MD_MAIN, "Enhanced Logging Configuration:");
  ELOG_INFO(ELOG_MD_MAIN, "- Max subscribers: %d", ELOG_MAX_SUBSCRIBERS);
  ELOG_INFO(ELOG_MD_MAIN, "- Max message length: %d bytes", ELOG_MAX_MESSAGE_LENGTH);
  ELOG_INFO(ELOG_MD_MAIN, "- Auto threshold: %d (%s)", ELOG_DEFAULT_THRESHOLD, elog_level_name(ELOG_DEFAULT_THRESHOLD));

#if (ELOG_THREAD_SAFE == 1)
  ELOG_INFO(ELOG_MD_MAIN, "- Thread safety: ENABLED");
  ELOG_INFO(ELOG_MD_MAIN, "- RTOS type: %d", ELOG_RTOS_TYPE);
  ELOG_INFO(ELOG_MD_MAIN, "- Mutex timeout: %d ms", ELOG_MUTEX_TIMEOUT_MS);
#else
  ELOG_INFO(ELOG_MD_MAIN, "- Thread safety: DISABLED");
#endif

#if USE_COLOR
  ELOG_INFO(ELOG_MD_MAIN, "- Color support: ENABLED");
#else
  ELOG_INFO(ELOG_MD_MAIN, "- Color support: DISABLED");
#endif

  printf("Configuration showcase complete.\n");
}

/* ========================================================================== */
/* Main Example Function */
/* ========================================================================== */

void complete_logging_demo(void) {
  printf("===============================================\n");
  printf("Enhanced Logging System (eLog) Demonstration\n");
  printf("Version 0.04 with RTOS Threading Support\n");
  printf("===============================================\n");

  basic_logging_example();
  per_module_threshold_example();
  multiple_subscribers_example();
  subscriber_management_example();
  error_codes_example();
  legacy_compatibility_example();

#if (ELOG_THREAD_SAFE == 1)
  thread_safety_example();
  thread_aware_logging_example();
  simulated_multitask_example();
  rtos_features_example();
#endif

  performance_test_example();
  configuration_showcase();

  printf("\n===============================================\n");
  printf("Enhanced Logging Demonstration Complete!\n");
  printf("===============================================\n");
}

int main(void) {
  complete_logging_demo();
  return 0;
}
