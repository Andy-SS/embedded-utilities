/***********************************************************
* @file	eLog.h
* @author	Andy Chen (clgm216@gmail.com)
* @version	0.06
* @date	2024-09-10
* @brief  Enhanced logging system inspired by uLog with multiple subscribers
*         INDEPENDENT HEADER - No external dependencies (except standard C)
* **********************************************************
* @copyright Copyright (c) 2025 TTK. All rights reserved.
* 
************************************************************/
#ifndef APP_DEBUG_H_
#define APP_DEBUG_H_

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "bit_utils.h"

/* ========================================================================== */
/* Enhanced Logging Configuration */
/* ========================================================================== */

/* Supported RTOS types - Define these first before using them */
#define ELOG_RTOS_NONE       0      /* No RTOS - bare metal */
#define ELOG_RTOS_FREERTOS   1      /* FreeRTOS */
#define ELOG_RTOS_THREADX    2      /* Azure ThreadX */
#define ELOG_RTOS_CMSIS      3      /* CMSIS-RTOS */

// /* Default eLog Configuration - Override these in your project if needed */
#define ELOG_THREAD_SAFE 1
#define ELOG_RTOS_TYPE ELOG_RTOS_THREADX
#define ELOG_MUTEX_TIMEOUT_MS 500

// Module list configuration
typedef enum {
  ELOG_MD_DEFAULT = 0,
  ELOG_MD_ERROR,
  ELOG_MD_IF,
  ELOG_MD_BLE,
  ELOG_MD_SENSOR,
  ELOG_MD_UI,
  ELOG_MD_MAIN,
  ELOG_MD_TASK_A,
  ELOG_MD_TASK_B,
  ELOG_MD_TASK_C,
  ELOG_MD_COMM,
  ELOG_MD_MAX
} elog_module_t;

/* Local definitions for independence from app_conf.h */
#ifndef YES
#define YES (0x01)
#endif
#ifndef NO
#define NO  (0x00)
#endif
#ifndef NULL
#define NULL ((void*)0)
#endif

/* Local utility function for filename extraction (independent implementation) */
static inline const char *debug_get_filename(const char *fullpath) {
  const char *ret = fullpath;
  const char *p;
  
  /* Find last occurrence of '/' */
  p = fullpath;
  while (*p) {
    if (*p == '/' || *p == '\\') {
      ret = p + 1;
    }
    p++;
  }
  
  return ret;
}

/* Debug Configuration - Modify these for different build types */
#define ELOG_DEBUG_INFO_ON YES      /* Information messages (GREEN) */
#define ELOG_DEBUG_WARN_ON YES      /* Warning messages (BROWN) */
#define ELOG_DEBUG_ERR_ON  YES      /* Error messages (RED) */
#define ELOG_DEBUG_LOG_ON  YES      /* Debug messages (CYAN) */
#define ELOG_DEBUG_TRACE_ON YES      /* Trace messages (BLUE) - very verbose, usually disabled in production */
#define ELOG_DEBUG_CRITICAL_ON YES  /* Critical error messages (RED BOLD) */
#define ELOG_DEBUG_ALWAYS_ON YES    /* Always logged messages (WHITE BOLD) */

/* Filename Support Configuration */
#define ENABLE_DEBUG_MESSAGES_WITH_MODULE 1

/* Auto-calculate the lowest enabled log level for enhanced logging */
#if (ELOG_DEBUG_TRACE_ON == YES)
#define ELOG_DEFAULT_THRESHOLD ELOG_LEVEL_TRACE
#elif (ELOG_DEBUG_LOG_ON == YES)
#define ELOG_DEFAULT_THRESHOLD ELOG_LEVEL_DEBUG
#elif (ELOG_DEBUG_INFO_ON == YES)
#define ELOG_DEFAULT_THRESHOLD ELOG_LEVEL_DEBUG
#elif (ELOG_DEBUG_WARN_ON == YES)
#define ELOG_DEFAULT_THRESHOLD ELOG_LEVEL_DEBUG
#elif (ELOG_DEBUG_ERR_ON == YES)
#define ELOG_DEFAULT_THRESHOLD ELOG_LEVEL_ERROR
#elif (ELOG_DEBUG_CRITICAL_ON == YES)
#define ELOG_DEFAULT_THRESHOLD ELOG_LEVEL_CRITICAL
#elif (ELOG_DEBUG_ALWAYS_ON == YES)
#define ELOG_DEFAULT_THRESHOLD ELOG_LEVEL_ALWAYS
#else
#define ELOG_DEFAULT_THRESHOLD ELOG_LEVEL_ALWAYS  /* Fallback if all disabled */
#endif

/* Maximum number of log subscribers (console, file, memory, etc.) */
#ifndef ELOG_MAX_SUBSCRIBERS
#define ELOG_MAX_SUBSCRIBERS 6
#endif

/* Maximum length of formatted log message */
#ifndef ELOG_MAX_MESSAGE_LENGTH
#define ELOG_MAX_MESSAGE_LENGTH 256
#define ELOG_MAX_LOCATION_LENGTH 64
#endif

/* ========================================================================== */
/* Thread Safety Configuration */
/* ========================================================================== */
#if (ELOG_THREAD_SAFE == 1)
/* Mutex result codes */
typedef enum {
  ELOG_MUTEX_OK = 0,
  ELOG_MUTEX_TIMEOUT,
  ELOG_MUTEX_ERROR,
  ELOG_MUTEX_NOT_SUPPORTED
} elog_mutex_result_t;

/* Mutex Callback Function Prototype */
typedef elog_mutex_result_t (*elog_mutex_create_fn)(void);
typedef elog_mutex_result_t (*elog_mutex_take_fn)(uint32_t timeout_ms);
typedef elog_mutex_result_t (*elog_mutex_give_fn)(void);
typedef elog_mutex_result_t (*elog_mutex_delete_fn)(void);

/* Mutex callbacks structure */
typedef struct {
  elog_mutex_create_fn create;
  elog_mutex_take_fn take;
  elog_mutex_give_fn give;
  elog_mutex_delete_fn delete;
} elog_mutex_callbacks_t;

/* Mutex type definition */
typedef void* elog_mutex_t;

#endif /* ELOG_THREAD_SAFE */

/* ========================================================================== */
/* Enhanced Logging Types and Enums */
/* ========================================================================== */

/**
 * @brief Enhanced log levels (inspired by uLog)
 */
typedef enum {
  ELOG_LEVEL_TRACE = 100,    /*!< Most verbose: function entry/exit, detailed flow */
  ELOG_LEVEL_DEBUG,          /*!< Debug info: variable values, state changes */
  ELOG_LEVEL_INFO,           /*!< Informational: normal operation events */
  ELOG_LEVEL_WARNING,        /*!< Warnings: recoverable errors, performance issues */
  ELOG_LEVEL_ERROR,          /*!< Errors: serious problems that need attention */
  ELOG_LEVEL_CRITICAL,       /*!< Critical: system failure, unrecoverable errors */
  ELOG_LEVEL_ALWAYS          /*!< Always logged: essential system messages */
} elog_level_t;

/**
 * @brief Log subscriber function prototype
 * @param level: Severity level of the message
 * @param msg: Formatted message string (temporary - copy if needed)
 */
typedef void (*log_subscriber_t)(elog_level_t level, const char *msg);

/**
 * @brief Unified Error Codes Enumeration
 * Comprehensive error codes for logging system and MCU subsystems (0x00-0xFF)
 */
typedef enum {
  /* Logging System Error Codes (0x00-0x0F) */
  ELOG_ERR_NONE              = 0x00,    /*!< No error / Operation successful */
  ELOG_ERR_SUBSCRIBERS_EXCEEDED = 0x01, /*!< Maximum subscribers exceeded */
  ELOG_ERR_NOT_SUBSCRIBED    = 0x02,    /*!< Subscriber not found */
  ELOG_ERR_INVALID_LEVEL     = 0x03,    /*!< Invalid log level */
  ELOG_ERR_INVALID_PARAM     = 0x04,    /*!< Invalid parameter */
  ELOG_ERR_INVALID_STATE     = 0x05,    /*!< Invalid state for operation */

  /* System Error Codes (0x10-0x1F) */
  ELOG_SYS_ERR_INIT          = 0x10,    /*!< System initialization error */
  ELOG_SYS_ERR_CONFIG        = 0x11,    /*!< Configuration error */
  ELOG_SYS_ERR_TIMEOUT       = 0x12,    /*!< Operation timeout */
  ELOG_SYS_ERR_BUSY          = 0x13,    /*!< System busy */
  ELOG_SYS_ERR_NOT_READY     = 0x14,    /*!< System not ready */
  ELOG_SYS_ERR_INVALID_STATE = 0x15,    /*!< Invalid system state */
  ELOG_SYS_ERR_MEMORY        = 0x16,    /*!< Memory allocation error */
  ELOG_SYS_ERR_WATCHDOG      = 0x17,    /*!< Watchdog reset occurred */

  /* Communication Error Codes (0x20-0x3F) */
  ELOG_COMM_ERR_UART         = 0x20,    /*!< UART communication error */
  ELOG_COMM_ERR_I2C          = 0x21,    /*!< I2C communication error */
  ELOG_COMM_ERR_SPI          = 0x22,    /*!< SPI communication error */
  ELOG_COMM_ERR_CAN          = 0x23,    /*!< CAN communication error */
  ELOG_COMM_ERR_USB          = 0x24,    /*!< USB communication error */
  ELOG_COMM_ERR_BLE          = 0x25,    /*!< Bluetooth LE error */
  ELOG_COMM_ERR_WIFI         = 0x26,    /*!< WiFi communication error */
  ELOG_COMM_ERR_ETH          = 0x27,    /*!< Ethernet communication error */
  ELOG_COMM_ERR_CHECKSUM     = 0x28,    /*!< Data checksum error */
  ELOG_COMM_ERR_FRAME        = 0x29,    /*!< Frame format error */
  ELOG_COMM_ERR_OVERRUN      = 0x2A,    /*!< Buffer overrun */
  ELOG_COMM_ERR_UNDERRUN     = 0x2B,    /*!< Buffer underrun */

  /* Sensor Error Codes (0x40-0x5F) */
  ELOG_SENSOR_ERR_NOT_FOUND  = 0x40,    /*!< Sensor not detected */
  ELOG_SENSOR_ERR_CALIB      = 0x41,    /*!< Sensor calibration error */
  ELOG_SENSOR_ERR_RANGE      = 0x42,    /*!< Sensor value out of range */
  ELOG_SENSOR_ERR_ACCURACY   = 0x43,    /*!< Sensor accuracy degraded */
  ELOG_ACCEL_ERR             = 0x44,    /*!< Accelerometer error */
  ELOG_GYRO_ERR              = 0x45,    /*!< Gyroscope error */
  ELOG_MAG_ERR               = 0x46,    /*!< Magnetometer error */
  ELOG_PRESS_ERR             = 0x47,    /*!< Pressure sensor error */
  ELOG_HUMID_ERR             = 0x48,    /*!< Humidity sensor error */
  ELOG_LIGHT_ERR             = 0x49,    /*!< Light sensor error */

  /* Power Management Error Codes (0x60-0x7F) */
  ELOG_PWR_ERR_LOW_VOLTAGE   = 0x60,    /*!< Low voltage detected */
  ELOG_PWR_ERR_OVERVOLTAGE   = 0x61,    /*!< Overvoltage detected */
  ELOG_PWR_ERR_OVERCURRENT   = 0x62,    /*!< Overcurrent detected */
  ELOG_PWR_ERR_THERMAL       = 0x63,    /*!< Thermal shutdown */
  ELOG_PWR_ERR_CHARGER       = 0x64,    /*!< Charger error */
  ELOG_PWR_ERR_REGULATOR     = 0x65,    /*!< Voltage regulator error */
  ELOG_PWR_ERR_BROWNOUT      = 0x66,    /*!< Brownout detected */

  /* Storage Error Codes (0x80-0x9F) */
  ELOG_STORAGE_ERR_READ      = 0x80,    /*!< Storage read error */
  ELOG_STORAGE_ERR_WRITE     = 0x81,    /*!< Storage write error */
  ELOG_STORAGE_ERR_ERASE     = 0x82,    /*!< Storage erase error */
  ELOG_STORAGE_ERR_FULL      = 0x83,    /*!< Storage full */
  ELOG_STORAGE_ERR_CORRUPT   = 0x84,    /*!< Data corruption detected */
  ELOG_FLASH_ERR             = 0x85,    /*!< Flash memory error */
  ELOG_EEPROM_ERR            = 0x86,    /*!< EEPROM error */
  ELOG_SD_ERR                = 0x87,    /*!< SD card error */

  /* Application Error Codes (0xA0-0xBF) */
  ELOG_APP_ERR_INVALID_PARAM = 0xA0,    /*!< Invalid parameter */
  ELOG_RTC_ERR               = 0xA3,    /*!< Real-time clock error */
  ELOG_CRYPTO_ERR            = 0xA4,    /*!< Cryptographic operation error */
  ELOG_AUTH_ERR              = 0xA5,    /*!< Authentication error */
  ELOG_PROT_ERR              = 0xA6,    /*!< Protocol error */
  ELOG_DATA_ERR              = 0xA7,    /*!< Data validation error */
  ELOG_ALGO_ERR              = 0xA8,    /*!< Algorithm execution error */

  /* Hardware Error Codes (0xC0-0xDF) */
  ELOG_HW_ERR_GPIO           = 0xC0,    /*!< GPIO configuration error */
  ELOG_HW_ERR_CLOCK          = 0xC1,    /*!< Clock configuration error */
  ELOG_HW_ERR_DMA            = 0xC2,    /*!< DMA error */
  ELOG_HW_ERR_TIMER          = 0xC3,    /*!< Timer error */
  ELOG_HW_ERR_ADC            = 0xC4,    /*!< ADC error */
  ELOG_HW_ERR_DAC            = 0xC5,    /*!< DAC error */
  ELOG_HW_ERR_PWM            = 0xC6,    /*!< PWM error */
  ELOG_HW_ERR_IRQ            = 0xC7,    /*!< Interrupt error */

  /* RTOS Error Codes (0xE0-0xEF) */
  ELOG_RTOS_ERR_TASK         = 0xE0,    /*!< Task creation/management error */
  ELOG_RTOS_ERR_QUEUE        = 0xE1,    /*!< Queue operation error */
  ELOG_RTOS_ERR_SEMAPHORE    = 0xE2,    /*!< Semaphore error */
  ELOG_RTOS_ERR_MUTEX        = 0xE3,    /*!< Mutex error */
  ELOG_RTOS_ERR_TIMER        = 0xE4,    /*!< RTOS timer error */
  ELOG_RTOS_ERR_MEMORY       = 0xE5,    /*!< RTOS memory allocation error */

  /* Critical System Error Codes (0xF0-0xFF) */
  ELOG_CRITICAL_ERR_STACK    = 0xF0,    /*!< Stack overflow */
  ELOG_CRITICAL_ERR_HEAP     = 0xF1,    /*!< Heap corruption */
  ELOG_CRITICAL_ERR_ASSERT   = 0xF2,    /*!< Assertion failure */
  ELOG_CRITICAL_ERR_HARDFAULT = 0xF3,   /*!< Hard fault exception */
  ELOG_CRITICAL_ERR_MEMFAULT = 0xF4,    /*!< Memory management fault */
  ELOG_CRITICAL_ERR_BUSFAULT = 0xF5,    /*!< Bus fault */
  ELOG_CRITICAL_ERR_USAGE    = 0xF6,    /*!< Usage fault */
  ELOG_CRITICAL_ERR_UNKNOWN  = 0xFF     /*!< Unknown critical error */
} elog_err_t;


/* ========================================================================== */
/* Enhanced Logging API (uLog-inspired) */
/* ========================================================================== */

/* Enhanced logging functions */
/**
 * @brief Initialize the enhanced logging system
 */
void elog_init(void);

/**
 * @brief Subscribe a function to receive log messages
 * @param fn: Function to call for each log message
 * @param threshold: Minimum level to send to this subscriber
 * @return Error code
 */
elog_err_t elog_subscribe(log_subscriber_t fn, elog_level_t threshold);

/**
 * @brief Unsubscribe a function from receiving log messages
 * @param fn: Function to unsubscribe
 * @return Error code
 */
elog_err_t elog_unsubscribe(log_subscriber_t fn);

/**
 * @brief Get the string name of a log level
 * @param level: Log level
 * @return String name of the log level
 */
const char *elog_level_name(elog_level_t level);

/**
 * @brief Get the automatically calculated threshold level
 * @return The ELOG_DEFAULT_THRESHOLD value
 */
elog_level_t elog_get_auto_threshold(void);

/**
 * @brief Set log threshold for a specific module
 * @param module: Module identifier
 * @param threshold: Log level threshold
 * @return ELOG_ERR_NONE on success
 */
elog_err_t elog_set_module_threshold(elog_module_t module, elog_level_t threshold);

/**
 * @brief Get log threshold for a specific module
 * @param module: Module identifier
 * @return threshold, or ELOG_DEFAULT_THRESHOLD if not set
 */
elog_level_t elog_get_module_threshold(elog_module_t module);

/**
 * @brief Send a formatted message to all subscribers
 * @param level: Severity level of the message
 * @param fmt: Printf-style format string
 * @param ...: Format arguments
 */
void elog_message(elog_module_t module, elog_level_t level, const char *fmt, ...);

/**
 * @brief Send a formatted message with location info to all subscribers
 * @param level: Severity level of the message
 * @param file: Source file name
 * @param func: Function name
 * @param line: Line number
 * @param fmt: Printf-style format string
 * @param ...: Format arguments
 */
void elog_message_with_location(elog_module_t module, elog_level_t level, const char *file, const char *func, int line, const char *fmt, ...);

#if (ELOG_THREAD_SAFE == 1)
/**
 * @brief Register mutex callback functions with eLog
 * @param callbacks: Pointer to callback structure (NULL to disable thread safety)
 * @return ELOG_ERR_NONE on success
 */
elog_err_t elog_register_mutex_callbacks(const elog_mutex_callbacks_t *callbacks);
#endif

/* ========================================================================== */
#define LOG_MESSAGE(module, level, ...) elog_message(module, level, __VA_ARGS__)
#define LOG_MESSAGE_WITH_LOCATION(module, level, file, func, line, ...) elog_message_with_location(module, level, file, func, line, __VA_ARGS__)
#define LOG_SUBSCRIBE_THREAD_SAFE(fn, level) elog_subscribe(fn, level)
#define LOG_UNSUBSCRIBE_THREAD_SAFE(fn) elog_unsubscribe(fn)

/* Enhanced logging core macros */
#define LOG_INIT() elog_init()
#define LOG_SUBSCRIBE(fn, level) LOG_SUBSCRIBE_THREAD_SAFE(fn, level)
#define LOG_UNSUBSCRIBE(fn) LOG_UNSUBSCRIBE_THREAD_SAFE(fn)
#define LOG_LEVEL_NAME(level) elog_level_name(level)

/* Convenience setup macro with console subscriber */
extern void elog_console_subscriber(elog_level_t level, const char *msg);
#define LOG_INIT_WITH_CONSOLE() do { \
    LOG_INIT(); \
    LOG_SUBSCRIBE(elog_console_subscriber, ELOG_DEFAULT_THRESHOLD); \
} while(0)

/* Enhanced convenience macros with automatic threshold */
#define LOG_INIT_AUTO() do { \
    LOG_INIT(); \
} while(0)

#define LOG_SUBSCRIBE_CONSOLE() LOG_SUBSCRIBE(elog_console_subscriber, ELOG_DEFAULT_THRESHOLD)
#define LOG_SUBSCRIBE_CONSOLE_LEVEL(level) LOG_SUBSCRIBE(elog_console_subscriber, level)

/* Ultimate convenience macro - initializes and subscribes console with auto threshold */
#define LOG_INIT_WITH_CONSOLE_AUTO() do { \
    LOG_INIT(); \
    LOG_SUBSCRIBE_CONSOLE(); \
} while(0)

/* Individual level macros - follow same pattern as legacy debug macros */
#if (ELOG_DEBUG_TRACE_ON == YES)
#if ENABLE_DEBUG_MESSAGES_WITH_MODULE
#define ELOG_TRACE(...) LOG_MESSAGE_WITH_LOCATION(ELOG_LEVEL_TRACE, debug_get_filename(__ASSERT_FILE_NAME), __func__, __LINE__, __VA_ARGS__)
#define ELOG_TRACE_STR(str) LOG_MESSAGE_WITH_LOCATION(ELOG_LEVEL_TRACE, debug_get_filename(__ASSERT_FILE_NAME), __func__, __LINE__, "%s", str)
#else
#define ELOG_TRACE(...) LOG_MESSAGE(ELOG_LEVEL_TRACE, __VA_ARGS__)
#define ELOG_TRACE_STR(str) LOG_MESSAGE(ELOG_LEVEL_TRACE, "%s", str)
#endif
#else
#define ELOG_TRACE(...) do {} while(0)
#define ELOG_TRACE_STR(str) do {} while(0)
#endif

#if (ELOG_DEBUG_LOG_ON == YES)
#if ENABLE_DEBUG_MESSAGES_WITH_MODULE
#define ELOG_DEBUG(module, ...) LOG_MESSAGE_WITH_LOCATION(module, ELOG_LEVEL_DEBUG, debug_get_filename(__ASSERT_FILE_NAME), __func__, __LINE__, __VA_ARGS__)
#define ELOG_DEBUG_STR(module, str) LOG_MESSAGE_WITH_LOCATION(module, ELOG_LEVEL_DEBUG, debug_get_filename(__ASSERT_FILE_NAME), __func__, __LINE__, "%s", str)
#else
#define ELOG_DEBUG(module, ...) LOG_MESSAGE(module, ELOG_LEVEL_DEBUG, __VA_ARGS__)
#define ELOG_DEBUG_STR(module, str) LOG_MESSAGE(module, ELOG_LEVEL_DEBUG, "%s", str)
#endif
#else
#define ELOG_DEBUG(module, ...) do {} while(0)
#define ELOG_DEBUG_STR(module, str) do {} while(0)
#endif

#if (ELOG_DEBUG_INFO_ON == YES)
#if ENABLE_DEBUG_MESSAGES_WITH_MODULE
#define ELOG_INFO(module, ...) LOG_MESSAGE_WITH_LOCATION(module, ELOG_LEVEL_INFO, debug_get_filename(__ASSERT_FILE_NAME), __func__, __LINE__, __VA_ARGS__)
#define ELOG_INFO_STR(module, str) LOG_MESSAGE_WITH_LOCATION(module, ELOG_LEVEL_INFO, debug_get_filename(__ASSERT_FILE_NAME), __func__, __LINE__, "%s", str)
#else
#define ELOG_INFO(module, ...) LOG_MESSAGE(module, ELOG_LEVEL_INFO, __VA_ARGS__)
#define ELOG_INFO_STR(module, str) LOG_MESSAGE(module, ELOG_LEVEL_INFO, "%s", str)
#endif
#else
#define ELOG_INFO(module, ...) do {} while(0)
#define ELOG_INFO_STR(module, str) do {} while(0)
#endif

#if (ELOG_DEBUG_WARN_ON == YES)
#if ENABLE_DEBUG_MESSAGES_WITH_MODULE
#define ELOG_WARNING(module, ...) LOG_MESSAGE_WITH_LOCATION(module, ELOG_LEVEL_WARNING, debug_get_filename(__ASSERT_FILE_NAME), __func__, __LINE__, __VA_ARGS__)
#define ELOG_WARNING_STR(module, str) LOG_MESSAGE_WITH_LOCATION(module, ELOG_LEVEL_WARNING, debug_get_filename(__ASSERT_FILE_NAME), __func__, __LINE__, "%s", str)
#else
#define ELOG_WARNING(module, ...) LOG_MESSAGE(module, ELOG_LEVEL_WARNING, __VA_ARGS__)
#define ELOG_WARNING_STR(module, str) LOG_MESSAGE(module, ELOG_LEVEL_WARNING, "%s", str)
#endif
#else
#define ELOG_WARNING(module, ...) do {} while(0)
#define ELOG_WARNING_STR(module, str) do {} while(0)
#endif

#if (ELOG_DEBUG_ERR_ON == YES)
#if ENABLE_DEBUG_MESSAGES_WITH_MODULE
#define ELOG_ERROR(module, ...) LOG_MESSAGE_WITH_LOCATION(module, ELOG_LEVEL_ERROR, debug_get_filename(__ASSERT_FILE_NAME), __func__, __LINE__, __VA_ARGS__)
#define ELOG_ERROR_STR(module, str) LOG_MESSAGE_WITH_LOCATION(module, ELOG_LEVEL_ERROR, debug_get_filename(__ASSERT_FILE_NAME), __func__, __LINE__, "%s", str)
#else
#define ELOG_ERROR(module, ...) LOG_MESSAGE(module, ELOG_LEVEL_ERROR, __VA_ARGS__)
#define ELOG_ERROR_STR(module, str) LOG_MESSAGE(module, ELOG_LEVEL_ERROR, "%s", str)
#endif
#else
#define ELOG_ERROR(module, ...) do {} while(0)
#define ELOG_ERROR_STR(module, str) do {} while(0)
#endif

#if (ELOG_DEBUG_CRITICAL_ON == YES)
#if ENABLE_DEBUG_MESSAGES_WITH_MODULE
#define ELOG_CRITICAL(module, ...) LOG_MESSAGE_WITH_LOCATION(module, ELOG_LEVEL_CRITICAL, debug_get_filename(__ASSERT_FILE_NAME), __func__, __LINE__, __VA_ARGS__)
#define ELOG_CRITICAL_STR(module, str) LOG_MESSAGE_WITH_LOCATION(module, ELOG_LEVEL_CRITICAL, debug_get_filename(__ASSERT_FILE_NAME), __func__, __LINE__, "%s", str)
#else
#define ELOG_CRITICAL(module, ...) LOG_MESSAGE(module, ELOG_LEVEL_CRITICAL, __VA_ARGS__)
#define ELOG_CRITICAL_STR(module, str) LOG_MESSAGE(module, ELOG_LEVEL_CRITICAL, "%s", str)
#endif
#else
#define ELOG_CRITICAL(module, ...) do {} while(0)
#define ELOG_CRITICAL_STR(module, str) do {} while(0)
#endif

#if (ELOG_DEBUG_ALWAYS_ON == YES)
#if ENABLE_DEBUG_MESSAGES_WITH_MODULE
#define ELOG_ALWAYS(module, ...) LOG_MESSAGE_WITH_LOCATION(module, ELOG_LEVEL_ALWAYS, debug_get_filename(__ASSERT_FILE_NAME), __func__, __LINE__, __VA_ARGS__)
#define ELOG_ALWAYS_STR(module, str) LOG_MESSAGE_WITH_LOCATION(module, ELOG_LEVEL_ALWAYS, debug_get_filename(__ASSERT_FILE_NAME), __func__, __LINE__, "%s", str)
#else
#define ELOG_ALWAYS(module, ...) LOG_MESSAGE(module, ELOG_LEVEL_ALWAYS, __VA_ARGS__)
#define ELOG_ALWAYS_STR(module, str) LOG_MESSAGE(module, ELOG_LEVEL_ALWAYS, "%s", str)
#endif
#else
#define ELOG_ALWAYS(module, ...) do {} while(0)
#define ELOG_ALWAYS_STR(module, str) do {} while(0)
#endif

/* ========================================================================== */
/* Legacy Logging System (Backwards Compatibility) */
/* ==========================================================================*/

/* Color control for enhanced logging console subscriber */
#define ELOG_USE_COLOR 1  /* Set to 0 to disable colors in elog_console_subscriber */
#define LOG_COLOR_BLACK  "30"
#define LOG_COLOR_RED    "31"
#define LOG_COLOR_GREEN  "32"
#define LOG_COLOR_BROWN  "33"
#define LOG_COLOR_BLUE   "34"
#define LOG_COLOR_PURPLE "35"
#define LOG_COLOR_CYAN   "36"
#define LOG_COLOR(COLOR) "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR)  "\033[1;" COLOR "m"
#if ELOG_USE_COLOR
#define LOG_RESET_COLOR  "\033[0m"
#define LOG_COLOR_E      LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W      LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I      LOG_COLOR(LOG_COLOR_GREEN)
#define LOG_COLOR_D      LOG_COLOR(LOG_COLOR_CYAN)
#else
#define LOG_COLOR_E      
#define LOG_COLOR_W      
#define LOG_COLOR_I      
#define LOG_COLOR_D      
#define LOG_RESET_COLOR  
#endif
#define LINE __LINE__

#ifdef __FILE_NAME__
#define __ASSERT_FILE_NAME __FILE_NAME__
#else /* __FILE_NAME__ */
#define __ASSERT_FILE_NAME __FILE__
#endif /* __FILE_NAME__ */

#define LOG_FORMAT(letter, format) \
  LOG_COLOR_##letter #letter ":" format LOG_RESET_COLOR "\n"

/* Legacy print macros now use enhanced logging system */
#if (ELOG_DEBUG_INFO_ON == YES)
#define printIF(module, format, ...) ELOG_INFO(module, format, ##__VA_ARGS__)
#define printIF_STR(module, str) ELOG_INFO_STR(module, str)
#else
#define printIF(module, format, ...)
#define printIF_STR(module, str)
#endif

#if (ELOG_DEBUG_ERR_ON == YES)
#define printERR(module, format, ...) ELOG_ERROR(module, format, ##__VA_ARGS__)
#define printERR_STR(module, str) ELOG_ERROR_STR(module, str)
#else
#define printERR(module, format, ...)
#define printERR_STR(module, str)
#endif

#if (ELOG_DEBUG_LOG_ON == YES)
#define printLOG(module, format, ...) ELOG_DEBUG(module, format, ##__VA_ARGS__)
#define printLOG_STR(module, str) ELOG_DEBUG_STR(module, str)
#else
#define printLOG(module, format, ...)
#define printLOG_STR(module, str)
#endif

#if (ELOG_DEBUG_WARN_ON == YES)
#define printWRN(module, format, ...) ELOG_WARNING(module, format, ##__VA_ARGS__)
#define printWRN_STR(module, str) ELOG_WARNING_STR(module, str)
#else
#define printWRN(module, format, ...)
#define printWRN_STR(module, str)
#endif

#if (ELOG_DEBUG_CRITICAL_ON == YES)
#define printCRITICAL(module, format, ...) ELOG_CRITICAL(module, format, ##__VA_ARGS__)
#define printCRITICAL_STR(module, str) ELOG_CRITICAL_STR(module, str)
#else
#define printCRITICAL(module, format, ...)
#define printCRITICAL_STR(module, str)
#endif

#if (ELOG_DEBUG_ALWAYS_ON == YES)
#define printALWAYS(module, format, ...) ELOG_ALWAYS(module, format, ##__VA_ARGS__)
#define printALWAYS_STR(module, str) ELOG_ALWAYS_STR(module, str)
#else
#define printALWAYS(module, format, ...)
#define printALWAYS_STR(module, str)
#endif

#if (ELOG_DEBUG_TRACE_ON == YES)
#define printTRACE(module, format, ...) ELOG_TRACE(module, format, ##__VA_ARGS__)
#define printTRACE_STR(module, str) ELOG_TRACE_STR(module, str)
#else
#define printTRACE(module, format, ...)
#define printTRACE_STR(module, str)
#endif

/**
 * @brief Update the RTOS_READY flag
 * @param ready: Boolean value indicating if RTOS is ready (1) or not (0)
 */
void elog_update_RTOS_ready(bool ready);

#endif /* APP_DEBUG_H_ */