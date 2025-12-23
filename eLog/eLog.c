/***********************************************************
 * @file	eLog.c
 * @author	Andy Chen (clgm216@gmail.com)
 * @version	0.05
 * @date	2024-09-10
 * @brief  Enhanced logging system implementation inspired by uLog
 *         INDEPENDENT IMPLEMENTATION - No external dependencies
 * **********************************************************
 * @copyright Copyright (c) 2025 TTK. All rights reserved.
 *
 ************************************************************/

#include "eLog.h"
#include "tx_api.h"    // Include ThreadX API for _tx_thread_system_state
#include "tx_thread.h" // Include ThreadX thread header for _tx_thread_system_state
#include <inttypes.h>  // For PRIu32 macro
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* ========================================================================== */
/* Enhanced Logging Internal State */
/* ========================================================================== */

#if defined(ELOG_RTOS_TYPE)
volatile bool RTOS_READY = false; // Flag to indicate if RTOS is ready
#endif

/**
 * @brief Subscriber entry structure
 */
typedef struct
{
  log_subscriber_t fn;
  elog_level_t threshold;
  int active;
} subscriber_entry_t;

/* Static storage for subscribers */
static subscriber_entry_t s_subscribers[ELOG_MAX_SUBSCRIBERS];
static int s_num_subscribers = 0;

/* Static message buffer for formatting */
static char s_message_buffer[ELOG_MAX_MESSAGE_LENGTH];

#if (ELOG_THREAD_SAFE == 1)
/* Mutex for thread safety */
static elog_mutex_t s_log_mutex;
static int s_mutex_initialized = 0;
#endif

typedef struct
{
  elog_module_t module;
  elog_level_t threshold;
} module_log_level_entry_t;

static module_log_level_entry_t module_log_levels[ELOG_MD_MAX];
static int module_log_level_count = 0;

/* ========================================================================== */
/* Enhanced Logging Core Implementation */
/* ========================================================================== */

/**
 * @brief Initialize the enhanced logging system
 */
void elog_init(void)
{
  /* Clear all subscribers */
  for (int i = 0; i < ELOG_MAX_SUBSCRIBERS; i++)
  {
    s_subscribers[i].fn = NULL;
    s_subscribers[i].threshold = ELOG_LEVEL_ALWAYS;
    s_subscribers[i].active = 0;
  }
  s_num_subscribers = 0;
  
  /* Clear module log levels */
  for (int i = 0; i < ELOG_MD_MAX; i++)
  {
    module_log_levels[i].module = (elog_module_t)i;
    module_log_levels[i].threshold = ELOG_DEFAULT_THRESHOLD;
  }
  module_log_level_count = ELOG_MD_MAX;
}

/**
 * @brief Get human-readable name for log level
 * @param level: Log level
 * @return String representation of level
 */
const char *elog_level_name(elog_level_t level)
{
  switch (level)
  {
  case ELOG_LEVEL_TRACE: return "T";    // Trace
  case ELOG_LEVEL_DEBUG: return "D";    // Debug
  case ELOG_LEVEL_INFO: return "I";     // Info
  case ELOG_LEVEL_WARNING: return "W";  // Warning
  case ELOG_LEVEL_ERROR: return "E";    // Error
  case ELOG_LEVEL_CRITICAL: return "C"; // Critical
  case ELOG_LEVEL_ALWAYS: return "A";   // Always
  default: return "U";                 // Unknown
  }
}

/**
 * @brief Get the automatically calculated threshold level
 * @return The ELOG_DEFAULT_THRESHOLD value
 */
elog_level_t elog_get_auto_threshold(void) { return ELOG_DEFAULT_THRESHOLD; }

/* ========================================================================== */
/* Built-in Console Subscriber */
/* ========================================================================== */

/**
 * @brief Built-in console subscriber with color support
 * @param level: Severity level of the message
 * @param msg: Formatted message string
 */
void elog_console_subscriber(elog_level_t level, const char *msg)
{
#if ELOG_USE_COLOR
  /* Color codes for different log levels */
  const char *colors[] = {
      [ELOG_LEVEL_TRACE] = LOG_COLOR(LOG_COLOR_BLUE),    /* Blue for trace */
      [ELOG_LEVEL_DEBUG] = LOG_COLOR(LOG_COLOR_CYAN),    /* Cyan for debug */
      [ELOG_LEVEL_INFO] = LOG_COLOR(LOG_COLOR_GREEN),    /* Green for info */
      [ELOG_LEVEL_WARNING] = LOG_COLOR(LOG_COLOR_BROWN), /* Brown/Yellow for warning */
      [ELOG_LEVEL_ERROR] = LOG_COLOR(LOG_COLOR_RED),     /* Red for error */
      [ELOG_LEVEL_CRITICAL] = LOG_BOLD(LOG_COLOR_RED),   /* Bold Red for critical */
      [ELOG_LEVEL_ALWAYS] = LOG_BOLD("37")               /* Bold White for always */
  };

  if (level >= ELOG_LEVEL_TRACE && level <= ELOG_LEVEL_ALWAYS)
  {
    printf("%s%s: %s%s\n", colors[level], elog_level_name(level), msg, LOG_RESET_COLOR);
  }
  else
  {
    printf("%s: %s\n", elog_level_name(level), msg);
  }
#else
  /* No color version */
  printf("%s: %s\n", elog_level_name(level), msg);
#endif
}

/* ========================================================================== */
/* Thread Safety Implementation */
/* ========================================================================== */

/**
 * @brief Check if RTOS is ready
 * @return true if RTOS is ready, false otherwise
 */
static bool elog_is_RTOS_ready(void)
{
#if (ELOG_THREAD_SAFE == 1)
  return RTOS_READY;
#else
  return false; // No RTOS in bare metal, always false
#endif
}

/**
 * @brief Create a mutex for logging synchronization
 * @param mutex: Pointer to mutex handle
 * @return Thread operation result
 */
static elog_thread_result_t elog_mutex_create(elog_mutex_t *mutex)
{
  if (!mutex) { return ELOG_THREAD_ERROR; }

#if (ELOG_RTOS_TYPE == ELOG_RTOS_FREERTOS)
#ifdef INC_FREERTOS_H
  *mutex = xSemaphoreCreateMutex();
  return (*mutex != NULL) ? ELOG_THREAD_OK : ELOG_THREAD_ERROR;
#else
  (void)mutex;
  return ELOG_THREAD_NOT_SUPPORTED;
#endif
#elif (ELOG_RTOS_TYPE == ELOG_RTOS_THREADX)
#ifdef TX_API_H
  UINT status = tx_mutex_create(mutex, "eLog_Mutex", TX_NO_INHERIT);
  return (status == TX_SUCCESS) ? ELOG_THREAD_OK : ELOG_THREAD_ERROR;
#else
  (void)mutex;
  return ELOG_THREAD_NOT_SUPPORTED;
#endif
#elif (ELOG_RTOS_TYPE == ELOG_RTOS_CMSIS)
#ifdef CMSIS_OS_H_
  const osMutexAttr_t mutex_attr = {.name = "eLog_Mutex",
                                    .attr_bits = osMutexRecursive | osMutexPrioInherit,
                                    .cb_mem = NULL,
                                    .cb_size = 0};
  *mutex = osMutexNew(&mutex_attr);
  return (*mutex != NULL) ? ELOG_THREAD_OK : ELOG_THREAD_ERROR;
#else
  (void)mutex;
  return ELOG_THREAD_NOT_SUPPORTED;
#endif
#else
  /* Bare metal - no mutex needed */
  (void)mutex;
  return ELOG_THREAD_OK;
#endif
}

/**
 * @brief Take/lock a mutex with timeout
 * @param mutex: Pointer to mutex handle
 * @param timeout_ms: Timeout in milliseconds
 * @return Thread operation result
 */
static elog_thread_result_t elog_mutex_take(elog_mutex_t *mutex, uint32_t timeout_ms)
{
  if (!mutex) { return ELOG_THREAD_ERROR; }

  // Check if the system state is 0
  if (!elog_is_RTOS_ready()) { return ELOG_THREAD_OK; }

#if (ELOG_RTOS_TYPE == ELOG_RTOS_FREERTOS)
#ifdef INC_FREERTOS_H
  BaseType_t result = xSemaphoreTake(*mutex, pdMS_TO_TICKS(timeout_ms));
  return (result == pdTRUE) ? ELOG_THREAD_OK : ELOG_THREAD_TIMEOUT;
#else
  (void)mutex;
  (void)timeout_ms;
  return ELOG_THREAD_NOT_SUPPORTED;
#endif
#elif (ELOG_RTOS_TYPE == ELOG_RTOS_THREADX)
#ifdef TX_API_H
  UINT status = tx_mutex_get(mutex, timeout_ms * TX_TIMER_TICKS_PER_SECOND / 1000);
  if (status == TX_SUCCESS) { return ELOG_THREAD_OK; }
  else if (status == TX_NOT_AVAILABLE) { return ELOG_THREAD_TIMEOUT; }
  else
  {
    return ELOG_THREAD_ERROR;
  }
#else
  (void)mutex;
  (void)timeout_ms;
  return ELOG_THREAD_NOT_SUPPORTED;
#endif
#elif (ELOG_RTOS_TYPE == ELOG_RTOS_CMSIS)
#ifdef CMSIS_OS_H_
  osStatus_t status = osMutexAcquire(*mutex, timeout_ms);
  if (status == osOK) { return ELOG_THREAD_OK; }
  else if (status == osErrorTimeout) { return ELOG_THREAD_TIMEOUT; }
  else
  {
    return ELOG_THREAD_ERROR;
  }
#else
  (void)mutex;
  (void)timeout_ms;
  return ELOG_THREAD_NOT_SUPPORTED;
#endif
#else
  /* Bare metal - no mutex needed */
  (void)mutex;
  (void)timeout_ms;
  return ELOG_THREAD_OK;
#endif
}

/**
 * @brief Give/unlock a mutex
 * @param mutex: Pointer to mutex handle
 * @return Thread operation result
 */
static elog_thread_result_t elog_mutex_give(elog_mutex_t *mutex)
{
  if (!mutex) { return ELOG_THREAD_ERROR; }
#if defined(ELOG_RTOS_TYPE)
  if (!elog_is_RTOS_ready()) { return ELOG_THREAD_OK; }
#endif

#if (ELOG_RTOS_TYPE == ELOG_RTOS_FREERTOS)
#ifdef INC_FREERTOS_H
  BaseType_t result = xSemaphoreGive(*mutex);
  return (result == pdTRUE) ? ELOG_THREAD_OK : ELOG_THREAD_ERROR;
#else
  (void)mutex;
  return ELOG_THREAD_NOT_SUPPORTED;
#endif
#elif (ELOG_RTOS_TYPE == ELOG_RTOS_THREADX)
#ifdef TX_API_H
  UINT status = tx_mutex_put(mutex);
  return (status == TX_SUCCESS) ? ELOG_THREAD_OK : ELOG_THREAD_ERROR;
#else
  (void)mutex;
  return ELOG_THREAD_NOT_SUPPORTED;
#endif
#elif (ELOG_RTOS_TYPE == ELOG_RTOS_CMSIS)
#ifdef CMSIS_OS_H_
  osStatus_t status = osMutexRelease(*mutex);
  return (status == osOK) ? ELOG_THREAD_OK : ELOG_THREAD_ERROR;
#else
  (void)mutex;
  return ELOG_THREAD_NOT_SUPPORTED;
#endif
#else
  /* Bare metal - no mutex needed */
  (void)mutex;
  return ELOG_THREAD_OK;
#endif
}

/**
 * @brief Delete a mutex
 * @param mutex: Pointer to mutex handle
 * @return Thread operation result
 */
static elog_thread_result_t elog_mutex_delete(elog_mutex_t *mutex)
{
  if (!mutex) { return ELOG_THREAD_ERROR; }

#if (ELOG_RTOS_TYPE == ELOG_RTOS_FREERTOS)
#ifdef INC_FREERTOS_H
  vSemaphoreDelete(*mutex);
  *mutex = NULL;
  return ELOG_THREAD_OK;
#else
  (void)mutex;
  return ELOG_THREAD_NOT_SUPPORTED;
#endif
#elif (ELOG_RTOS_TYPE == ELOG_RTOS_THREADX)
#ifdef TX_API_H
  UINT status = tx_mutex_delete(mutex);
  return (status == TX_SUCCESS) ? ELOG_THREAD_OK : ELOG_THREAD_ERROR;
#else
  (void)mutex;
  return ELOG_THREAD_NOT_SUPPORTED;
#endif
#elif (ELOG_RTOS_TYPE == ELOG_RTOS_CMSIS)
#ifdef CMSIS_OS_H_
  osStatus_t status = osMutexDelete(*mutex);
  *mutex = NULL;
  return (status == osOK) ? ELOG_THREAD_OK : ELOG_THREAD_ERROR;
#else
  (void)mutex;
  return ELOG_THREAD_NOT_SUPPORTED;
#endif
#else
  /* Bare metal - no mutex needed */
  (void)mutex;
  return ELOG_THREAD_OK;
#endif
}

/**
 * @brief Update the RTOS_READY flag
 * @param ready: Boolean value indicating if RTOS is ready (1) or not (0)
 */
void elog_update_RTOS_ready(bool ready)
{
#if (ELOG_THREAD_SAFE == 1)
/* Initialize mutex for thread safety */
if (!s_mutex_initialized)
{
  if (elog_mutex_create(&s_log_mutex) == ELOG_THREAD_OK)
  {
    s_mutex_initialized = 1;
    elog_mutex_give(&s_log_mutex); // Ensure mutex is available
    printLOG_STR(ELOG_MD_DEFAULT, "eLog mutex initialized successfully.");
  }
}

/* Update RTOS_READY flag */
RTOS_READY = ready;
#else
  (void)ready; // No RTOS, no action needed
#endif

}

/**
 * @brief Thread-safe version of log_message
 * @param level: Severity level of the message
 * @param fmt: Printf-style format string
 * @param ...: Format arguments
 */
void elog_message(elog_module_t module, elog_level_t level, const char *fmt, ...)
{
  if (level < elog_get_module_threshold(module))
  {
    return; // Skip log if below module threshold
  }

  /* If RTOS was ready then take mutex with timeout */
  if (elog_is_RTOS_ready() && elog_mutex_take(&s_log_mutex, ELOG_MUTEX_TIMEOUT_MS) != ELOG_THREAD_OK)
  {
    return; /* Skip logging if can't get mutex */
  }

  va_list args;

  /* Format the message */
  va_start(args, fmt);
  vsnprintf(s_message_buffer, sizeof(s_message_buffer), fmt, args);
  va_end(args);

  /* Send to all active subscribers */
  for (int i = 0; i < s_num_subscribers; i++)
  {
    // printf("[DEBUG] Checking subscriber %d: active=%d, threshold=%d, level=%d.\n", i,
    // s_subscribers[i].active, s_subscribers[i].threshold, level);
    if (s_subscribers[i].active && level >= s_subscribers[i].threshold)
    {
      // printf("[DEBUG] Sending message to subscriber %d.\n", i);
      s_subscribers[i].fn(level, s_message_buffer);
    }
  }

  /* Give mutex */
  if (elog_is_RTOS_ready()) elog_mutex_give(&s_log_mutex);
}

/**
 * @brief Thread-safe version of log_message_with_location
 * @param level: Severity level of the message
 * @param file: Source file name
 * @param func: Function name
 * @param line: Line number
 * @param fmt: Printf-style format string
 * @param ...: Format arguments
 */
void elog_message_with_location(elog_module_t module, elog_level_t level, const char *file, const char *func,
                                     int line, const char *fmt, ...)
{
  if (level < elog_get_module_threshold(module))
  {
    return; // Skip log if below module threshold
  }

  /* If RTOS was ready then take mutex with timeout */
  if (elog_is_RTOS_ready() && elog_mutex_take(&s_log_mutex, ELOG_MUTEX_TIMEOUT_MS) != ELOG_THREAD_OK)
  {
    return; /* Skip logging if can't get mutex */
  }

  va_list args;
  char temp_buffer[ELOG_MAX_MESSAGE_LENGTH - ELOG_MAX_LOCATION_LENGTH]; /* Reserve space for location info */

  /* Format the user message first */
  va_start(args, fmt);
  vsnprintf(temp_buffer, sizeof(temp_buffer), fmt, args);
  va_end(args);

  /* Add location information - ensure null termination */
  int written = snprintf(s_message_buffer, sizeof(s_message_buffer), "[%s][%s][%d] %s", file, func,
                         line, temp_buffer);
  if (written >= (int)sizeof(s_message_buffer))
  {
    s_message_buffer[sizeof(s_message_buffer) - 1] = '\0'; /* Ensure null termination */
  }

  /* Send to all active subscribers */
  for (int i = 0; i < s_num_subscribers; i++)
  {
    if (s_subscribers[i].active && level >= s_subscribers[i].threshold)
    {
      s_subscribers[i].fn(level, s_message_buffer);
    }
  }

  /* Give mutex */
  if (elog_is_RTOS_ready()) elog_mutex_give(&s_log_mutex);
}

/**
 * @brief Thread-safe version of log_subscribe
 * @param fn: Function to call for each log message
 * @param threshold: Minimum level to send to this subscriber
 * @return Error code
 */
elog_err_t elog_subscribe(log_subscriber_t fn, elog_level_t threshold)
{
  /* If RTOS was ready then take mutex with timeout */
  if (elog_is_RTOS_ready() && elog_mutex_take(&s_log_mutex, ELOG_MUTEX_TIMEOUT_MS) != ELOG_THREAD_OK)
  {
    return ELOG_ERR_SUBSCRIBERS_EXCEEDED; /* Return error if can't get mutex */
  }

  elog_err_t result = ELOG_ERR_SUBSCRIBERS_EXCEEDED;

  if (s_num_subscribers < ELOG_MAX_SUBSCRIBERS)
  {
    /* Check if already subscribed */
    for (int i = 0; i < s_num_subscribers; i++)
    {
      if (s_subscribers[i].fn == fn && s_subscribers[i].active)
      {
        /* Update existing subscription */
        s_subscribers[i].threshold = threshold;
        result = ELOG_ERR_NONE;
        goto exit;
      }
    }

    /* Add new subscriber */
    s_subscribers[s_num_subscribers].fn = fn;
    s_subscribers[s_num_subscribers].threshold = threshold;
    s_subscribers[s_num_subscribers].active = 1;
    s_num_subscribers++;
    result = ELOG_ERR_NONE;
  }

exit:
  /* Give mutex */
  if (elog_is_RTOS_ready()) elog_mutex_give(&s_log_mutex);
  return result;
}

/**
 * @brief Thread-safe version of log_unsubscribe
 * @param fn: Function to unsubscribe
 * @return Error code
 */
elog_err_t elog_unsubscribe(log_subscriber_t fn)
{
  /* If RTOS was ready then take mutex with timeout */
  if (elog_is_RTOS_ready() && elog_mutex_take(&s_log_mutex, ELOG_MUTEX_TIMEOUT_MS) != ELOG_THREAD_OK)
  {
    return ELOG_ERR_NOT_SUBSCRIBED; /* Return error if can't get mutex */
  }

  elog_err_t result = ELOG_ERR_NOT_SUBSCRIBED;

  for (int i = 0; i < s_num_subscribers; i++)
  {
    if (s_subscribers[i].fn == fn && s_subscribers[i].active)
    {
      s_subscribers[i].active = 0;
      result = ELOG_ERR_NONE;
      break;
    }
  }

  /* Give mutex */
  if (elog_is_RTOS_ready()) elog_mutex_give(&s_log_mutex);
  return result;
}

/**
 * @brief Get current task name (RTOS-specific)
 * @return Task name string or "UNKNOWN" if not available
 */
const char *elog_get_task_name(void)
{
#if (ELOG_RTOS_TYPE == ELOG_RTOS_FREERTOS)
#ifdef INC_FREERTOS_H
  TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
  if (current_task != NULL) { return pcTaskGetName(current_task); }
#endif
#elif (ELOG_RTOS_TYPE == ELOG_RTOS_THREADX)
#ifdef TX_API_H
  TX_THREAD *current_thread = tx_thread_identify();
  if (current_thread != NULL) { return current_thread->tx_thread_name; }
#endif
#elif (ELOG_RTOS_TYPE == ELOG_RTOS_CMSIS)
#ifdef CMSIS_OS_H_
  const char *name = osThreadGetName(osThreadGetId());
  if (name != NULL) { return name; }
#endif
#endif
  return "UNKNOWN";
}

/**
 * @brief Get current task ID (RTOS-specific)
 * @return Task ID or 0 if not available
 */
uint32_t elog_get_task_id(void)
{
#if (ELOG_RTOS_TYPE == ELOG_RTOS_FREERTOS)
#ifdef INC_FREERTOS_H
  TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
  return (uint32_t)current_task;
#endif
#elif (ELOG_RTOS_TYPE == ELOG_RTOS_THREADX)
#ifdef TX_API_H
  TX_THREAD *current_thread = tx_thread_identify();
  if (current_thread != NULL) { return (uint32_t)current_thread; }
#endif
#elif (ELOG_RTOS_TYPE == ELOG_RTOS_CMSIS)
#ifdef CMSIS_OS_H_
  osThreadId_t id = osThreadGetId();
  return (uint32_t)id;
#endif
#endif
  return 0;
}

elog_err_t elog_set_module_threshold(elog_module_t module, elog_level_t threshold)
{
  if (module >= ELOG_MD_MAX) { return ELOG_ERR_INVALID_LEVEL; }
  for (int i = 0; i < module_log_level_count; ++i)
  {
    if (module_log_levels[i].module == module)
    {
      module_log_levels[i].threshold = threshold;
      return ELOG_ERR_NONE;
    }
  }
  return ELOG_ERR_SUBSCRIBERS_EXCEEDED;
}

elog_level_t elog_get_module_threshold(elog_module_t module)
{
  if (module >= ELOG_MD_MAX) { return ELOG_DEFAULT_THRESHOLD; }
  for (int i = 0; i < module_log_level_count; ++i)
  {
    if (module_log_levels[i].module == module) { return module_log_levels[i].threshold; }
  }
  return ELOG_DEFAULT_THRESHOLD;
}
