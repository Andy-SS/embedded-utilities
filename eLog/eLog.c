/***********************************************************
 * @file	eLog.c
 * @author	Andy Chen (clgm216@gmail.com)
 * @version	0.06
 * @date	2024-09-10
 * @brief  Enhanced logging system implementation inspired by uLog
 *         INDEPENDENT IMPLEMENTATION - No external dependencies
 * **********************************************************
 * @copyright Copyright (c) 2025 TTK. All rights reserved.
 *
 ************************************************************/

#include "eLog.h"
#include "mutex_common.h"
#include <inttypes.h>  // For PRIu32 macro
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "mutex_common.h"

/* ========================================================================== */
/* Running number */
volatile uint32_t s_log_runing_number[ELOG_MD_MAX] = {0};

static uint32_t inline get_runing_nbr(elog_module_t module)
{
  // Simple increment - no atomic operations for now to diagnose issue
  return ++s_log_runing_number[module];
}

/* ========================================================================== */
/* Enhanced Logging Internal State */
/* ========================================================================== */

/**
 * @brief Subscriber entry structure
 */
typedef struct
{
  log_subscriber_t fn;
  elog_level_t threshold;
} subscriber_entry_t;

/* Static storage for subscribers */
static subscriber_entry_t s_subscribers[ELOG_MAX_SUBSCRIBERS];
static int s_num_subscribers = 0;

/* Static message buffer for formatting */
static char s_fmt[ELOG_MAX_MESSAGE_LENGTH];
static char s_full_message_buffer[ELOG_FULL_MESSAGE_LENGTH];

/* Mutex for thread safety */
static volatile void *s_log_mutex;

typedef struct
{
  elog_level_t threshold;
} module_log_level_entry_t;

static module_log_level_entry_t module_log_levels[ELOG_MD_MAX];

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
  }
  s_num_subscribers = 0;
  
  /* Clear module log levels */
  for (int i = 0; i < ELOG_MD_MAX; i++)
  {
    module_log_levels[i].threshold = ELOG_DEFAULT_THRESHOLD;
  }
}

static bool inline elog_enter_cs(void){
  if (utilities_is_RTOS_ready()) {
    // Try to take mutex if it was successfully created
    if (s_log_mutex != NULL) {
      if (utilities_mutex_take((void *)s_log_mutex, ELOG_MUTEX_TIMEOUT_MS) == MUTEX_OK) {
        return true;
      }
    }

    // Lazy create mutex on first use
    if (s_log_mutex == NULL) {
      s_log_mutex = utilities_mutex_create();
    }
  }
  return false;
}

static void inline elog_exit_cs(bool took_mutex){
  if (took_mutex) {
    utilities_mutex_give((void *)s_log_mutex);
  }
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
 * @brief Built-in console log subscriber
 * @param handle: Unused
 * @param buf: Message buffer
 * @param len: Length of message
 * @return Always 0
 */
int elog_console_subscriber(int handle, const char *buf, size_t len)
{
  extern int LPUartQueueBuffWrite(int handle, const char *buf, size_t bufSize);
  /* Redirect to UART Tx DMA */
  return LPUartQueueBuffWrite(handle, buf, len);
}

/* ========================================================================== */
/* Thread Safety Implementation */
/* ========================================================================== */

#if ENABLE_DEBUG_MESSAGES_WITH_LOCATION
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

  /* Try to acquire mutex if RTOS is ready and mutex exists */
  bool took_mutex = false;
  took_mutex = elog_enter_cs();
  memset(s_fmt, 0, sizeof(s_fmt));
  memset(s_full_message_buffer, 0, sizeof(s_full_message_buffer));

  /* Add location information with color support - ensure null termination */
  #if ELOG_USE_COLOR
  const char *colors[] = {
      [ELOG_LEVEL_TRACE] = LOG_COLOR(LOG_COLOR_BLUE),    /* Blue for trace */
      [ELOG_LEVEL_DEBUG] = LOG_COLOR(LOG_COLOR_CYAN),    /* Cyan for debug */
      [ELOG_LEVEL_INFO] = LOG_COLOR(LOG_COLOR_GREEN),    /* Green for info */
      [ELOG_LEVEL_WARNING] = LOG_COLOR(LOG_COLOR_BROWN), /* Brown/Yellow for warning */
      [ELOG_LEVEL_ERROR] = LOG_COLOR(LOG_COLOR_RED),     /* Red for error */
      [ELOG_LEVEL_CRITICAL] = LOG_BOLD(LOG_COLOR_RED),   /* Bold Red for critical */
      [ELOG_LEVEL_ALWAYS] = LOG_BOLD("37")               /* Bold White for always */
  };

  const char *color_code = (level >= ELOG_LEVEL_TRACE && level <= ELOG_LEVEL_ALWAYS) ? colors[level] : "";
  const char *end_color = (level >= ELOG_LEVEL_TRACE && level <= ELOG_LEVEL_ALWAYS) ? LOG_RESET_COLOR : "";

  #else
  const char *color_code = "";
  const char *end_color = "";
  #endif

  va_list args;

  /* Format the user message first */
  va_start(args, fmt);
  int fmt_len = snprintf(s_fmt, sizeof(s_fmt), 
                  "%s%s:%u,%lu:[%s][%s][%d] %s%s\n", 
                    color_code, elog_level_name(level),(uint8_t)module, 
                  get_runing_nbr(module), file, func, line, fmt, end_color);

  int final_len = vsnprintf(s_full_message_buffer, sizeof(s_full_message_buffer), s_fmt, args);
  va_end(args);



  /* Send to all subscribers */
  for (int i = 0; i < s_num_subscribers; i++)
  {
    if (level >= s_subscribers[i].threshold)
    {
      if(final_len > 0) { s_subscribers[i].fn(1, s_full_message_buffer, final_len); }
      else { s_subscribers[i].fn(1, "vsnprintf error!!!", 19); }
    }
  }

  /* Give mutex only if we took it */
  elog_exit_cs(took_mutex);
}
#else
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
  /* Try to acquire mutex if RTOS is ready and mutex exists */
  bool took_mutex = false;
  int fmt_len = 0;
  int final_len = 0;
  took_mutex = elog_enter_cs();
  memset(s_fmt, 0, sizeof(s_fmt));
  memset(s_full_message_buffer, 0, sizeof(s_full_message_buffer));
  
  #if ELOG_USE_COLOR
  /* Color codes for different log levels */
  const char *colors[] = {
      [ELOG_LEVEL_TRACE] = LOG_COLOR(LOG_COLOR_BLUE),    /* Blue for trace */
      [ELOG_LEVEL_INFO] = LOG_COLOR(LOG_COLOR_GREEN),    /* Green for info */
      [ELOG_LEVEL_DEBUG] = LOG_COLOR(LOG_COLOR_CYAN),    /* Cyan for debug */
      [ELOG_LEVEL_WARNING] = LOG_COLOR(LOG_COLOR_BROWN), /* Brown/Yellow for warning */
      [ELOG_LEVEL_ERROR] = LOG_COLOR(LOG_COLOR_RED),     /* Red for error */
      [ELOG_LEVEL_CRITICAL] = LOG_BOLD(LOG_COLOR_RED),   /* Bold Red for critical */
      [ELOG_LEVEL_ALWAYS] = LOG_BOLD("37")               /* Bold White for always */
  };
  const char *color_code = (level >= ELOG_LEVEL_TRACE && level <= ELOG_LEVEL_ALWAYS) ? colors[level] : "";
  const char *end_color = (level >= ELOG_LEVEL_TRACE && level <= ELOG_LEVEL_ALWAYS) ? LOG_RESET_COLOR : "";

  #else
  /* No color*/
  const char *color_code = "";
  const char *end_color = "";
  #endif
  va_list args;
  /* Format the message */
  va_start(args, fmt);
  fmt_len = snprintf(s_fmt, sizeof(s_fmt), 
                  "%s%s:%u,%lu: %s%s\n", 
                    color_code, elog_level_name(level),(uint8_t)module, 
                  get_runing_nbr(module), fmt, end_color);
  final_len = vsnprintf(s_full_message_buffer, sizeof(s_full_message_buffer), s_fmt, args);
  va_end(args); 

  /* Send to all subscribers */
  for (int i = 0; i < s_num_subscribers; i++)
  {
    if (level >= s_subscribers[i].threshold)
    {
      // printf("[DEBUG] Sending message to subscriber %d.\n", i);
      if (final_len > 0) { s_subscribers[i].fn(1, s_full_message_buffer, final_len); }
      else { s_subscribers[i].fn(1, "vsnprintf error!!!", 19); }
    }
  }

  /* Give mutex only if we took it */
  elog_exit_cs(took_mutex);
}
#endif
/**
 * @brief Thread-safe version of log_subscribe
 * @param fn: Function to call for each log message
 * @param threshold: Minimum level to send to this subscriber
 * @return Error code
 */
elog_err_t elog_subscribe(log_subscriber_t fn, elog_level_t threshold)
{
  /* Try to acquire mutex if RTOS is ready and mutex exists */
  bool took_mutex = false;
  took_mutex = elog_enter_cs();

  elog_err_t result = ELOG_ERR_SUBSCRIBERS_EXCEEDED;

  if (s_num_subscribers < ELOG_MAX_SUBSCRIBERS)
  {
    /* Check if already subscribed */
    for (int i = 0; i < s_num_subscribers; i++)
    {
      if (s_subscribers[i].fn == fn)
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
    s_num_subscribers++;
    result = ELOG_ERR_NONE;
  }

exit:
  /* Give mutex only if we took it */
  elog_exit_cs(took_mutex);
  return result;
}

/**
 * @brief Thread-safe version of log_unsubscribe
 * @param fn: Function to unsubscribe
 * @return Error code
 */
elog_err_t elog_unsubscribe(log_subscriber_t fn)
{
  /* Try to acquire mutex if RTOS is ready and mutex exists */
  bool took_mutex = false;
  took_mutex = elog_enter_cs();

  elog_err_t result = ELOG_ERR_NOT_SUBSCRIBED;

  for (int i = 0; i < s_num_subscribers; i++)
  {
    if (s_subscribers[i].fn == fn)
    {
      result = ELOG_ERR_NONE;
      break;
    }
  }

  /* Give mutex only if we took it */
  elog_exit_cs(took_mutex);
  return result;
}

/**
 * @brief Set log threshold for a specific module
 * @param module: Module identifier
 * @param threshold: Log level threshold
 * @return ELOG_ERR_NONE on success
 */
elog_err_t elog_set_module_threshold(elog_module_t module, elog_level_t threshold)
{
  if (module >= ELOG_MD_MAX) { return ELOG_ERR_INVALID_LEVEL; }
  module_log_levels[module].threshold = threshold;
  return ELOG_ERR_NONE;
}

/**
 * @brief Get log threshold for a specific module
 * @param module: Module identifier
 * @return threshold, or ELOG_DEFAULT_THRESHOLD if not set
 */
elog_level_t elog_get_module_threshold(elog_module_t module)
{
  if (module >= ELOG_MD_MAX) { return ELOG_DEFAULT_THRESHOLD; }
  return module_log_levels[module].threshold;
}
