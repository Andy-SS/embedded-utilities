/**
 * @file eLog_config_examples.h
 * @brief Configuration examples for different RTOS environments
 * @author Andy Chen (clgm216@gmail.com)
 * @date 2024-09-10
 * 
 * This file contains example configurations for eLog in different
 * RTOS environments. Copy the appropriate section to your project's
 * configuration header or modify eLog.h directly.
 */

#ifndef ELOG_CONFIG_EXAMPLES_H
#define ELOG_CONFIG_EXAMPLES_H

/* ========================================================================== */
/* Azure ThreadX Configuration Example (Default) */
/* ========================================================================== */
#if 1  /* Default: Azure ThreadX */

/* Thread safety configuration */
#define ELOG_THREAD_SAFE 1
#define ELOG_RTOS_TYPE ELOG_RTOS_THREADX
#define ELOG_MUTEX_TIMEOUT_MS 500    /* Increased from 100ms for production robustness */

/* Include ThreadX headers before eLog.h in your source files */
#include "tx_api.h"
#include "eLog.h"

/* Usage in ThreadX threads: */
void thread_entry(ULONG thread_input) {
    // Set per-module log threshold (O(1) direct array indexing - fast!)
    elog_set_module_threshold(ELOG_MD_MAIN, ELOG_LEVEL_DEBUG);

    while(1) {
        ELOG_INFO(ELOG_MD_MAIN, "Thread processing");
        ELOG_DEBUG(ELOG_MD_MAIN, "Debug info for ThreadX thread");
        tx_thread_sleep(100);
    }
}

#endif /* ThreadX Configuration */

/* ========================================================================== */
/* FreeRTOS Configuration Example */
/* ========================================================================== */
#if 0  /* Enable for FreeRTOS projects */

/* Thread safety configuration */
#define ELOG_THREAD_SAFE 1
#define ELOG_RTOS_TYPE ELOG_RTOS_FREERTOS
#define ELOG_MUTEX_TIMEOUT_MS 500    /* Increased from 100ms for production robustness */

/* Include FreeRTOS headers before eLog.h in your source files */
/* #include "FreeRTOS.h" */
/* #include "semphr.h" */
/* #include "task.h" */
/* #include "eLog.h" */

/* Usage in FreeRTOS tasks: */
/*
void vTask1(void *pvParameters) {
    // Set per-module log threshold for this task's module
    elog_set_module_threshold(ELOG_MD_TASK1, ELOG_LEVEL_DEBUG);

    for(;;) {
        ELOG_INFO(ELOG_MD_TASK1, "Task 1 running on core %d", xPortGetCoreID());
        ELOG_DEBUG(ELOG_MD_TASK1, "Debug info for FreeRTOS task");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
*/

#endif /* FreeRTOS Configuration */

/* ========================================================================== */
/* Azure ThreadX Configuration Example */
/* ========================================================================== */
#if 0  /* Enable for ThreadX projects */

/* Thread safety configuration */
#define ELOG_THREAD_SAFE 1
#define ELOG_RTOS_TYPE ELOG_RTOS_THREADX
#define ELOG_MUTEX_TIMEOUT_MS 500    /* Increased from 100ms for production robustness */

/* Include ThreadX headers before eLog.h in your source files */
/* #include "tx_api.h" */
/* #include "eLog.h" */

/* Usage in ThreadX threads: */
/*
void thread_entry(ULONG thread_input) {
    elog_set_module_threshold(ELOG_MD_THREAD, ELOG_LEVEL_DEBUG);

    while(1) {
        ELOG_INFO(ELOG_MD_THREAD, "Thread processing");
        ELOG_WARNING(ELOG_MD_THREAD, "Warning info for ThreadX thread");
        tx_thread_sleep(100);
    }
}
*/

#endif /* ThreadX Configuration */

/* ========================================================================== */
/* CMSIS-RTOS Configuration Example */
/* ========================================================================== */
#if 0  /* Enable for CMSIS-RTOS projects */

/* Thread safety configuration */
#define ELOG_THREAD_SAFE 1
#define ELOG_RTOS_TYPE ELOG_RTOS_CMSIS
#define ELOG_MUTEX_TIMEOUT_MS 500    /* Increased from 100ms for production robustness */

/* Include CMSIS-RTOS headers before eLog.h in your source files */
/* #include "cmsis_os.h" */
/* #include "eLog.h" */

/* Usage in CMSIS-RTOS threads: */
/*
void thread_function(void *argument) {
    elog_set_module_threshold(ELOG_MD_CMSIS, ELOG_LEVEL_ERROR);

    while(1) {
        ELOG_INFO(ELOG_MD_CMSIS, "CMSIS thread executing");
        ELOG_ERROR(ELOG_MD_CMSIS, "Error info for CMSIS thread");
        osDelay(1000);
    }
}
*/

#endif /* CMSIS-RTOS Configuration */

/* ========================================================================== */
/* Bare Metal Configuration Example */
/* ========================================================================== */
#if 0  /* Enable for bare metal (no RTOS) projects */

/* Disable thread safety for bare metal */
#define ELOG_THREAD_SAFE 0
#define ELOG_RTOS_TYPE ELOG_RTOS_NONE

/* No additional headers needed */
/* #include "eLog.h" */

/* Usage in bare metal applications: */
/*
int main(void) {
    LOG_INIT_WITH_CONSOLE_AUTO();
    elog_set_module_threshold(ELOG_MD_MAIN, ELOG_LEVEL_DEBUG);

    ELOG_INFO(ELOG_MD_MAIN, "Bare metal application started");
    
    while(1) {
        ELOG_DEBUG(ELOG_MD_MAIN, "Main loop iteration");
        // Your main loop code
    }
}
*/

#endif /* Bare Metal Configuration */

/* ========================================================================== */
/* Performance Tuning Options */
/* ========================================================================== */

/* Memory optimization */
// #define ELOG_MAX_MESSAGE_LENGTH 64     /* Reduce for memory-constrained systems */
// #define ELOG_MAX_SUBSCRIBERS 3         /* Reduce if fewer outputs needed */

/* Disable unused log levels for maximum performance */
// #define ELOG_DEBUG_TRACE_ON NO               /* Disable verbose tracing */
// #define ELOG_DEBUG_LOG_ON NO                 /* Disable debug messages in release */

/* Disable features for minimal footprint */
// #define USE_COLOR 0                  /* Disable colors for embedded terminals */
// #define ENABLE_DEBUG_MESSAGES_WITH_MODULE 0  /* Disable file/line info */

/* ========================================================================== */
/* Integration with Existing Projects */
/* ========================================================================== */

/* If you already have debug macros, eLog provides backwards compatibility: */
/*
 * Your existing code with printIF(), printERR(), printLOG(), etc. 
 * will continue to work unchanged. eLog enhances these macros
 * with the subscriber pattern and thread safety.
 */

/* Migration strategy: */
/*
 * 1. Add eLog.h and eLog.c to your project
 * 2. Configure ELOG_THREAD_SAFE and ELOG_RTOS_TYPE appropriately
 * 3. Set ELOG_MUTEX_TIMEOUT_MS to 500ms (production-recommended)
 * 4. Call LOG_INIT_WITH_CONSOLE_AUTO() early in your application
 * 5. Your existing debug macros work immediately
 * 6. Gradually adopt new ELOG_xxx macros for enhanced features
 * 7. Add custom subscribers for file/network/memory logging as needed
 * 8. Use elog_set_module_threshold() for per-module log level control (O(1) operation)
 * 9. Module threshold management is now simplified without 'active' flag complexity
 */

#endif /* ELOG_CONFIG_EXAMPLES_H */
