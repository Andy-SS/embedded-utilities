# Enhanced eLog - Advanced Logging System for Embedded MCU Projects

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-ARM%20Cortex--M-blue.svg)](https://developer.arm.com/architectures/cpu-architecture/m-profile)
[![C Standard](https://img.shields.io/badge/C-C99-green.svg)](https://en.wikipedia.org/wiki/C99)
[![Version](https://img.shields.io/badge/Version-0.05-blue.svg)]

A comprehensive, lightweight, and feature-rich logging system designed specifically for embedded microcontroller projects. Inspired by uLog but significantly enhanced with modern features, RTOS threading support, unified error codes, and backwards compatibility.

## 🚀 Features

### ✨ Core Capabilities
- **Multiple Subscribers**: Support for up to 6 concurrent logging outputs (console, file, memory buffer, network, etc.)
- **Compile-time Optimization**: Individual log levels can be disabled at compile time for minimal footprint
- **Color Support**: Built-in ANSI color coding for better terminal debugging experience
- **Location Information**: Optional file/function/line information for debugging
- **Zero External Dependencies**: Only requires standard C library (stdio.h, stdarg.h, string.h)

### 🔒 RTOS Threading Support (NEW!)
- **Thread-Safe Operations**: Mutex-protected logging operations for multi-threaded environments
- **Multiple RTOS Support**: FreeRTOS, Azure ThreadX, CMSIS-RTOS compatibility
- **Task Information**: Automatic task name and ID integration in log messages
- **Timeout Protection**: Configurable mutex timeouts to prevent blocking
- **Graceful Fallback**: Automatic fallback to non-threaded mode when RTOS unavailable
- **Zero Overhead**: Thread safety can be completely disabled for single-threaded applications

### 🎯 Embedded-Specific Features
- **Per-Module Log Thresholds**: Set log levels for individual modules at runtime for fine-grained control
- **Memory Efficient**: 128-byte message buffer, optimized for resource-constrained systems
- **RTOS Ready**: Thread-safe design suitable for FreeRTOS, ThreadX, and other RTOSs
- **MCU Error Codes**: Comprehensive set of error codes for common MCU subsystems
- **Auto-threshold Calculation**: Intelligent threshold detection based on enabled debug levels

### 📊 Performance Benefits
- **Compile-time Elimination**: Disabled log levels generate zero code
- **Subscriber Pattern**: Efficient message distribution to multiple outputs
- **Minimal Runtime Overhead**: Optimized for real-time embedded applications

## 🏆 Comparison with Original uLog

| Feature                   | eLog V0.05        | uLog                | Advantage         |
|---------------------------|-------------------|---------------------|-------------------|
| **Thread Safety**         | ✅ Multi-RTOS      | ❌ None              | **eLog**          |
| **Compile-time Optimization** | ✅ Per-level   | ❌ All-or-nothing    | **eLog**          |
| **Backwards Compatibility**   | ✅ Full        | ❌ None              | **eLog**          |
| **Built-in Console Colors**   | ✅ ANSI colors | ❌ User provides     | **eLog**          |
| **MCU Error Codes**           | ✅ Comprehensive| ❌ None              | **eLog**          |
| **Auto-threshold**            | ✅ Smart detection | ❌ Manual only   | **eLog**          |
| **Location Info**             | ✅ Module/file/line/func | ❌ User adds | **eLog**          |
| **Legacy Integration**        | ✅ Seamless    | ❌ Requires migration| **eLog**          |
| **Task Information**          | ✅ Auto-detect | ❌ None              | **eLog**          |
| **Per-Module Log Thresholds** | ✅ Runtime set | ❌ None              | **eLog**          |
| **Memory Usage**              | 128B buffer    | 120B buffer          | Tie               |
| **Max Subscribers**           | 6 (configurable)| 6 (configurable)    | Tie               |

## 📦 Quick Start

### Installation
Simply copy `eLog.h`, `eLog.c`, and `bit_utils.h` to your project and include them in your build system.

```c
#include "eLog.h"
#include "bit_utils.h"  // For bit manipulation utilities

int main() {
    // Initialize with console output and automatic threshold
    LOG_INIT_WITH_CONSOLE_AUTO();
    
    // Your existing code works unchanged!
    printIF(ELOG_MD_MAIN, "System initialized");           // Legacy macro
    printERR(ELOG_MD_MAIN, "Error code: 0x%02X", ELOG_COMM_ERR_I2C);
    
    // Or use enhanced logging
    ELOG_INFO(ELOG_MD_MAIN, "Battery level: %d%%", battery_level);
    ELOG_ERROR(ELOG_MD_MAIN, "Sensor failure: 0x%02X", ELOG_SENSOR_ERR_NOT_FOUND);
    
    return 0;
}
```

### Configuration
Configure debug levels in `eLog.h`:

```c
#define ELOG_DEBUG_INFO_ON YES      /* Information messages */
#define ELOG_DEBUG_WARN_ON YES      /* Warning messages */
#define ELOG_DEBUG_ERR_ON  YES      /* Error messages */
#define ELOG_DEBUG_LOG_ON  YES      /* Debug messages */
#define ELOG_DEBUG_TRACE_ON NO      /* Trace messages (verbose) */
#define ELOG_DEBUG_CRITICAL_ON YES  /* Critical errors */
#define ELOG_DEBUG_ALWAYS_ON YES    /* Always logged messages */
```

### Per-Module Log Level Example

You can set log levels for specific modules at runtime:

```c
#include "eLog.h"

void sensorInit(void) {
    // Set threshold for SENSOR module - instant O(1) operation
    elog_set_module_threshold(ELOG_MD_SENSOR, ELOG_LEVEL_DEBUG);
    
    // Enable debug logs for SENSOR module only
    ELOG_INFO(ELOG_MD_SENSOR, "Sensor initialized");
    ELOG_DEBUG(ELOG_MD_SENSOR, "Sensor debug info visible");
}
```

## ⚙️ Advanced Usage

### Per-Module Log Threshold API

```c
elog_err_t elog_set_module_threshold(elog_module_t module, elog_level_t threshold);
elog_level_t elog_get_module_threshold(elog_module_t module);
```
**Performance**: O(1) direct array indexing for 12-module logging system (previously O(n) linear search).

Use these functions to control logging verbosity for each module at runtime:

### Custom Subscribers
```c
void my_file_logger(elog_level_t level, const char *msg) {
    FILE *log_file = fopen("system.log", "a");
    fprintf(log_file, "[%s] %s\n", log_level_name(level), msg);
    fclose(log_file);
}

// Subscribe custom logger for ERROR and above
LOG_SUBSCRIBE(my_file_logger, ELOG_LEVEL_ERROR);
```

### Multiple Output Destinations
```c
LOG_INIT();

// Console for all messages
LOG_SUBSCRIBE(elog_console_subscriber, ELOG_LEVEL_DEBUG);

// File for errors only
LOG_SUBSCRIBE(my_file_logger, ELOG_LEVEL_ERROR);

// Network for critical alerts
LOG_SUBSCRIBE(my_network_logger, ELOG_LEVEL_CRITICAL);

// Memory buffer for debugging
LOG_SUBSCRIBE(my_memory_logger, ELOG_LEVEL_TRACE);
```

### Error Code Integration
```c
// Comprehensive MCU error codes included
if (i2c_status != HAL_OK) {
    ELOG_ERROR("I2C communication failed: 0x%02X", ELOG_COMM_ERR_I2C);
}

if (battery_voltage < MIN_VOLTAGE) {
    ELOG_WARNING("Low battery: 0x%02X", ELOG_PWR_ERR_LOW_VOLTAGE);
}

// Critical system errors
if (stack_overflow_detected) {
    ELOG_CRITICAL("Stack overflow detected: 0x%02X", ELOG_CRITICAL_ERR_STACK);
}
```

## 📚 Documentation

### Log Levels
- `ELOG_LEVEL_TRACE` (100): Function entry/exit, detailed flow
- `ELOG_LEVEL_DEBUG` (101): Variable values, state changes  
- `ELOG_LEVEL_INFO` (102): Normal operation events
- `ELOG_LEVEL_WARNING` (103): Recoverable errors, performance issues
- `ELOG_LEVEL_ERROR` (104): Serious problems requiring attention
- `ELOG_LEVEL_CRITICAL` (105): System failures, unrecoverable errors
- `ELOG_LEVEL_ALWAYS` (106): Essential system messages

### Available Macros

#### Enhanced Logging
```c
ELOG_TRACE(ELOG_MD_MAIN, "Function entry: %s", __func__);
ELOG_DEBUG(ELOG_MD_MAIN, "Variable x = %d", x);
ELOG_INFO(ELOG_MD_MAIN, "System ready");
ELOG_WARNING(ELOG_MD_MAIN, "Performance degraded");  
ELOG_ERROR(ELOG_MD_MAIN, "Operation failed: 0x%02X", error_code);
ELOG_CRITICAL(ELOG_MD_MAIN, "System failure");
ELOG_ALWAYS(ELOG_MD_MAIN, "Boot complete");
```

#### Legacy Compatibility
```c
printTRACE(ELOG_MD_MAIN, "Trace message");
printLOG(ELOG_MD_MAIN, "Debug message");
printIF(ELOG_MD_MAIN, "Info message");
printWRN(ELOG_MD_MAIN, "Warning message");
printERR(ELOG_MD_MAIN, "Error message");
printCRITICAL(ELOG_MD_MAIN, "Critical message");
printALWAYS(ELOG_MD_MAIN, "Always logged");
```

### Error Code Categories
- **System Errors** (0x10-0x1F): Core system operations
- **Communication** (0x20-0x3F): UART, I2C, SPI, CAN, BLE, WiFi, Ethernet
- **Sensors** (0x40-0x5F): Accelerometer, gyroscope, pressure, temperature
- **Power Management** (0x60-0x7F): Voltage, current, thermal, charging
- **Storage** (0x80-0x9F): Flash, EEPROM, SD card operations
- **Application** (0xA0-0xBF): App-specific functionality
- **Hardware** (0xC0-0xDF): GPIO, timers, ADC, DAC, PWM
- **RTOS** (0xE0-0xEF): Tasks, queues, semaphores, mutexes
- **Critical** (0xF0-0xFF): Stack, heap, hard faults, assertions

## 🔌 RTOS Threading Configuration

### Unified Mutex Callbacks (NEW!)

eLog now uses the unified `mutex_callbacks_t` interface from `mutex_common.h`:

```c
/* Unified mutex callback structure - same for eLog and Ring */
typedef struct {
  void* (*create)(void);                           /* Create mutex */
  void (*destroy)(void *mutex);                    /* Destroy mutex */
  mutex_result_t (*acquire)(void *mutex, uint32_t timeout_ms);  /* Lock with timeout */
  mutex_result_t (*release)(void *mutex);          /* Unlock */
} mutex_callbacks_t;
```

### Thread Safety Options
```c
/* Enable/disable thread safety */
#define ELOG_THREAD_SAFE 1              /* 1=enabled, 0=disabled */

/* RTOS selection */
#define ELOG_RTOS_TYPE ELOG_RTOS_THREADX
// Options: ELOG_RTOS_THREADX, ELOG_RTOS_FREERTOS, ELOG_RTOS_CMSIS, ELOG_RTOS_NONE

/* Mutex timeout configuration */
#define ELOG_MUTEX_TIMEOUT_MS 500       /* Timeout in milliseconds */
```

### ThreadX Integration with Unified Mutex

The ThreadX mutex implementation is provided in `rtos.c`:

```c
/* ThreadX mutex callbacks (implementation example) */
void* threadx_mutex_create(void) {
  TX_MUTEX *mutex = (TX_MUTEX *)malloc(sizeof(TX_MUTEX));
  if (mutex == NULL) return NULL;
  
  UINT status = tx_mutex_create(mutex, "eLog Mutex", TX_NO_INHERIT);
  return (status == TX_SUCCESS) ? (void *)mutex : NULL;
}

mutex_result_t threadx_mutex_acquire(void *mutex, uint32_t timeout_ms) {
  if (mutex == NULL) return MUTEX_ERROR;
  
  TX_MUTEX *tx_mutex = (TX_MUTEX *)mutex;
  UINT timeout_ticks = (timeout_ms == UINT32_MAX) ? TX_WAIT_FOREVER : TX_MS_TO_TICKS(timeout_ms);
  UINT status = tx_mutex_get(tx_mutex, timeout_ticks);
  
  if (status == TX_SUCCESS) return MUTEX_OK;
  if (status == TX_NOT_AVAILABLE) return MUTEX_TIMEOUT;
  return MUTEX_ERROR;
}

mutex_result_t threadx_mutex_release(void *mutex) {
  if (mutex == NULL) return MUTEX_ERROR;
  
  TX_MUTEX *tx_mutex = (TX_MUTEX *)mutex;
  UINT status = tx_mutex_put(tx_mutex);
  return (status == TX_SUCCESS) ? MUTEX_OK : MUTEX_ERROR;
}

mutex_result_t threadx_mutex_destroy(void *mutex) {
  if (mutex == NULL) return MUTEX_ERROR;
  
  TX_MUTEX *tx_mutex = (TX_MUTEX *)mutex;
  UINT status = tx_mutex_delete(tx_mutex);
  free(tx_mutex);
  return (status == TX_SUCCESS) ? MUTEX_OK : MUTEX_ERROR;
}

/* Unified callbacks structure */
const mutex_callbacks_t mutex_callbacks = {
  .create = threadx_mutex_create,
  .destroy = threadx_mutex_destroy,
  .acquire = threadx_mutex_acquire,
  .release = threadx_mutex_release
};
```

### RTOS Integration Examples

#### ThreadX Setup (Recommended)

In `tx_application_define()`:

```c
void tx_application_define(VOID *first_unused_memory) {
    // ... other initialization ...
    
    #if (ELOG_THREAD_SAFE == 1)
    // Register unified mutex callbacks with eLog
    elog_register_mutex_callbacks(&mutex_callbacks);
    elog_update_RTOS_ready(true);
    #endif
    
    // Now eLog is thread-safe for all tasks
}
```

In your task code:

```c
void task_entry(ULONG thread_input) {
    ELOG_INFO(ELOG_MD_MAIN, "Task started");
    
    for(;;) {
        ELOG_DEBUG(ELOG_MD_MAIN, "Task running - mutex protected");
        tx_thread_sleep(TX_MS_TO_TICKS(1000));
    }
}
```

#### FreeRTOS Setup
```c
#define ELOG_THREAD_SAFE 1
#define ELOG_RTOS_TYPE ELOG_RTOS_FREERTOS

#include "FreeRTOS.h"
#include "eLog.h"
#include "mutex_common.h"

/* FreeRTOS mutex implementation */
extern const mutex_callbacks_t freertos_callbacks;

void app_init(void) {
    // Register unified mutex callbacks
    elog_register_mutex_callbacks(&freertos_callbacks);
    elog_update_RTOS_ready(true);
    
    LOG_INIT_WITH_CONSOLE();
}

void task1(void *pvParameters) {
    for(;;) {
        ELOG_INFO(ELOG_MD_MAIN, "FreeRTOS task running");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### Quick Start - ThreadX Integration

The recommended pattern for ThreadX projects:

```c
// ============================================
// IN main.c - Early Hardware Initialization
// ============================================
int main(void) {
    // Hardware setup
    HAL_Init();
    SystemClock_Config();
    
    // *** Initialize eLog EARLY - before RTOS starts ***
    LOG_INIT_WITH_CONSOLE_AUTO();
    printLOG(ELOG_MD_DEFAULT, "Hardware initialized, starting BLE stack...");
    
    // BLE/Platform initialization
    MX_APPE_Init();
    
    printLOG(ELOG_MD_DEFAULT, "Starting ThreadX RTOS...");
    MX_ThreadX_Init();  // This starts RTOS - doesn't return
    
    // Never reached
    while(1);
}

// ============================================
// IN rtos.c - RTOS Kernel Initialization
// ============================================

// Define ThreadX mutex callbacks
#if (ELOG_THREAD_SAFE == 1)
static TX_MUTEX elog_mutex;

mutex_result_t threadx_mutex_create(void) {
  UINT status = tx_mutex_create(&elog_mutex, "eLog_Mutex", TX_NO_INHERIT);
  return (status == TX_SUCCESS) ? ELOG_MUTEX_OK : ELOG_MUTEX_ERROR;
}

mutex_result_t threadx_mutex_take(uint32_t timeout_ms) {
  UINT status = tx_mutex_get(&elog_mutex, timeout_ms * TX_TIMER_TICKS_PER_SECOND / 1000);
  if (status == TX_SUCCESS) return ELOG_MUTEX_OK;
  else if (status == TX_NOT_AVAILABLE) return ELOG_MUTEX_TIMEOUT;
  else return ELOG_MUTEX_ERROR;
}

mutex_result_t threadx_mutex_give(void) {
  UINT status = tx_mutex_put(&elog_mutex);
  return (status == TX_SUCCESS) ? ELOG_MUTEX_OK : ELOG_MUTEX_ERROR;
}

mutex_result_t threadx_mutex_delete(void) {
  UINT status = tx_mutex_delete(&elog_mutex);
  return (status == TX_SUCCESS) ? ELOG_MUTEX_OK : ELOG_MUTEX_ERROR;
}

const mutex_callbacks_t threadx_mutex_callbacks = {
  .create = threadx_mutex_create,
  .take = threadx_mutex_take,
  .give = threadx_mutex_give,
  .delete = threadx_mutex_delete
};
#endif

// Called by ThreadX during kernel startup
VOID tx_application_define(VOID *first_unused_memory) {
    // Create byte pools
    tx_byte_pool_create(&tx_app_byte_pool, "App memory pool", 
                        tx_byte_pool_buffer, TX_APP_MEM_POOL_SIZE);
    
    // Create all RTOS objects (threads, semaphores, mutexes, etc.)
    // ... thread creation, semaphore creation, etc. ...
    
    // *** AT END: Register mutex and enable thread-safe logging ***
    #if (ELOG_THREAD_SAFE == 1)
    elog_register_mutex_callbacks(&threadx_mutex_callbacks);  // Must be BEFORE next line
    #endif
    elog_update_RTOS_ready(true);  // Enable thread safety - do this LAST
    printLOG(ELOG_MD_DEFAULT, "RTOS ready - thread-safe logging enabled");
}

// ============================================
// IN ANY THREAD - Logging is now thread-safe
// ============================================
void TaskEntry(ULONG thread_input) {
    printLOG(ELOG_MD_DEFAULT, "Task started - mutex protection active");
    
    while(1) {
        ELOG_DEBUG(ELOG_MD_MAIN, "Task executing - safe for concurrent logging");
        tx_thread_sleep(1000);
    }
}
```

**Key Initialization Points:**
1. ✅ `LOG_INIT_WITH_CONSOLE_AUTO()` - called in `main()` BEFORE RTOS starts
2. ✅ `elog_register_mutex_callbacks()` - called in `tx_application_define()` at END
3. ✅ `elog_update_RTOS_ready(true)` - called LAST in `tx_application_define()`
4. ✅ Logging works at all stages with automatic thread safety when RTOS is ready



**Proper ThreadX Integration Pattern - Key Principles:**

```
Phase 1: Early Initialization (NO RTOS YET)
├─ LOG_INIT_WITH_CONSOLE_AUTO()           ← Initialize eLog (non-threaded mode)
├─ printLOG(...)                           ← Logging works without thread safety
└─ System setup continues...

Phase 2: RTOS Setup (tx_application_define)
├─ Create byte pools
├─ Create threads/semaphores/mutexes
├─ Create RTOS objects...
└─ AT END OF FUNCTION:
   ├─ elog_register_mutex_callbacks()      ← Register ThreadX callbacks
   ├─ elog_update_RTOS_ready(true)         ← Enable thread-safe logging
   └─ ⚠️  NO logging inside this function after elog_update_RTOS_ready()

Phase 3: Runtime (ALL THREADS RUNNING)
└─ All logging calls automatically use mutex protection with 500ms timeout
```

**Detailed Implementation:**

```c
// Early in initialization (before RTOS)
void main(void) {
    HAL_Init();
    // ... device configuration ...
    
    // Initialize logging early - runs WITHOUT thread safety yet
    LOG_INIT_WITH_CONSOLE_AUTO();
    printLOG(ELOG_MD_DEFAULT, "System starting...");
    
    // ... continue setup ...
    tx_kernel_enter();  // Start ThreadX RTOS
}

// In tx_application_define (called by ThreadX kernel during startup)
VOID tx_application_define(VOID *first_unused_memory) {
    // Create byte pools
    tx_byte_pool_create(&tx_app_byte_pool, "Tx App memory pool", 
                        tx_byte_pool_buffer, TX_APP_MEM_POOL_SIZE);
    
    // Create threads, semaphores, mutexes, etc.
    tx_thread_create(&TaskIdleThread, "TaskIdle", TaskIdleEntry, 0,
                     task_stack, TASK_STACK_SIZE,
                     TASK_PRIORITY, TASK_PRIORITY,
                     TX_NO_TIME_SLICE, TX_AUTO_START);
    
    // ... create more system objects ...
    
    /* CRITICAL: Register mutex callbacks BEFORE enabling thread safety */
    #if (ELOG_THREAD_SAFE == 1)
    elog_register_mutex_callbacks(&threadx_mutex_callbacks);
    #endif
    
    /* CRITICAL: Call ONLY AT END to enable thread-safe logging */
    /* NOTE: Removed printLOG from this function to prevent deadlock during init */
    elog_update_RTOS_ready(true);
    /* Logging now protected by mutex - safe to call from any thread */
}

// In any thread (logging is now thread-safe)
void TaskIdleEntry(ULONG thread_input) {
    printLOG(ELOG_MD_DEFAULT, "Thread started - logging is protected by mutex");
    
    while(1) {
        ELOG_INFO(ELOG_MD_MAIN, "Safe to log from multiple threads");
        tx_thread_sleep(1000);
    }
}
```

**Key Points:**
1. **Early Init**: `LOG_INIT_WITH_CONSOLE_AUTO()` can be called BEFORE RTOS starts
   - Enables logging during device initialization
   - Runs without thread safety (no RTOS yet)
   
2. **Mid RTOS Init**: Build all RTOS objects normally in `tx_application_define()`
   - Logging still works without thread safety
   - Threads may log but RTOS not fully started yet
   
3. **End of RTOS Init**: At END of `tx_application_define()`:
   - First call: `elog_register_mutex_callbacks()` - registers ThreadX callbacks
   - Then call: `elog_update_RTOS_ready(true)` - enables mutex protection
   
4. **Runtime**: All logging is protected by mutex automatically
   - No changes to logging code needed
   - Safe for concurrent logging from multiple threads

**Thread Safety Behavior:**
- Before `elog_update_RTOS_ready(true)`: No mutex protection (but RTOS not running yet anyway)
- After `elog_update_RTOS_ready(true)`: All logging calls use mutex protection
- Mutex timeout: 500ms (graceful timeout prevents deadlock on severe overload)
- Timeout behavior: Logging is skipped if mutex not available (safe failure mode)
- Graceful degradation: Works with or without RTOS



#### Bare Metal (No RTOS)
```c
#define ELOG_THREAD_SAFE 0              /* Disable threading for bare metal */
#define ELOG_RTOS_TYPE ELOG_RTOS_NONE

#include "eLog.h"

int main(void) {
    LOG_INIT_WITH_CONSOLE_AUTO();
    ELOG_INFO(ELOG_MD_MAIN, "Bare metal application started");
    
    while(1) {
        // Main loop
        ELOG_DEBUG(ELOG_MD_MAIN, "Main loop iteration");
    }
}
```

### Thread-Safe API Functions
```c
/* Core thread-safe functions (automatically protect access with mutex when ELOG_THREAD_SAFE=1) */
void elog_message(elog_module_t module, elog_level_t level, const char *fmt, ...);
void elog_message_with_location(elog_module_t module, elog_level_t level, const char *file, const char *func, int line, const char *fmt, ...);
elog_err_t elog_subscribe(log_subscriber_t fn, elog_level_t threshold);
elog_err_t elog_unsubscribe(log_subscriber_t fn);

/* RTOS registration and management */
elog_err_t elog_register_mutex_callbacks(const mutex_callbacks_t *callbacks);  /* Register RTOS mutex callbacks */
void elog_update_RTOS_ready(bool ready);                                            /* Signal RTOS is ready for thread safety */

/* Mutex callback registration structure */
typedef struct {
  elog_mutex_create_fn create;   /* Create mutex callback */
  elog_mutex_take_fn take;       /* Acquire mutex callback */
  elog_mutex_give_fn give;       /* Release mutex callback */
  elog_mutex_delete_fn delete;   /* Delete mutex callback */
} mutex_callbacks_t;

/* Mutex timeout configuration */
#define ELOG_MUTEX_TIMEOUT_MS 100      /* Timeout in milliseconds for mutex operations */
```

**Thread Safety Behavior:**
- When `ELOG_THREAD_SAFE=1` and `RTOS_READY=true`: All logging calls are protected by mutex
- When `ELOG_THREAD_SAFE=1` but `RTOS_READY=false`: Logging calls proceed without mutex (RTOS not yet initialized)
- When `ELOG_THREAD_SAFE=0`: No mutex protection (bare metal or disabled threading)
- Mutex timeout is configurable - if timeout occurs, logging is skipped

### Performance Considerations
- **Thread Safety Overhead**: ~50-100 CPU cycles per log call (mutex operations)
- **Memory Overhead**: Additional ~32-64 bytes for mutex storage
- **Timeout Behavior**: Logging calls will timeout and skip if mutex cannot be acquired
- **RTOS Integration**: Minimal impact on existing RTOS task scheduling

## 🔧 New Features in V0.05

- **Unified Error Codes**: Single comprehensive `elog_error_t` enum (0x00-0xFF) consolidating logging system errors and MCU subsystem errors organized by category
- **Per-Module Log Thresholds**: Control log verbosity for each module at runtime using `elog_set_module_threshold()` and `elog_get_module_threshold()`
- **Thread-Safe Mutex Registration**: `elog_register_mutex_callbacks()` and `elog_update_RTOS_ready()` provide flexible RTOS integration without modifying eLog core
- **RTOS Ready Status Management**: Automatic fallback to non-threaded mode during RTOS initialization, switches to thread-safe mode when `elog_update_RTOS_ready(true)` is called
- **Mutex Callback Pattern**: Support for any RTOS via callback registration (FreeRTOS, ThreadX, CMSIS-RTOS, custom implementations)
- **Enhanced Examples**: New examples demonstrating unified error codes across all subsystems and ThreadX integration patterns
- **Backward Compatibility**: Legacy macros (`printLOG()`, `printIF()`, etc.) continue to work unchanged

## ⚙️ Configuration Options

```c
/* Subscriber configuration */
#define ELOG_MAX_SUBSCRIBERS 6           /* Maximum concurrent outputs */

/* Message buffer size */
#define ELOG_MAX_MESSAGE_LENGTH 128      /* Buffer size for formatted messages */

/* Color support */
#define USE_COLOR 1                     /* Enable ANSI colors in console */

/* Location information */
#define ENABLE_DEBUG_MESSAGES_WITH_MODULE 0  /* Include file/line info */
```

## 💾 Memory Usage

### Base System
- **ROM**: ~2-4KB (depending on enabled features)
- **RAM**: ~200 bytes static allocation  
- **Stack**: ~128 bytes per log call (message buffer)

### With Thread Safety (ELOG_THREAD_SAFE=1)
- **Additional ROM**: ~1-2KB (RTOS abstraction layer)
- **Additional RAM**: ~32-64 bytes (mutex storage)
- **Runtime Overhead**: ~50-100 CPU cycles per log call (mutex operations)

### Memory Optimization Tips
- Set `ELOG_THREAD_SAFE=0` for single-threaded applications
- Reduce `ELOG_MAX_MESSAGE_LENGTH` for memory-constrained systems
- Disable unused log levels at compile time
- Use `ELOG_MAX_SUBSCRIBERS` to limit subscriber array size

## 🧪 Examples

The library includes comprehensive examples in `eLog_example.c`, `eLog_example_rtos.c`, and `eLog_rtos_demo.c`:

### Basic Examples (`eLog_example.c`)
- Basic logging with all log levels
- Per-module threshold control
- Multiple subscriber setup  
- Unified error code usage by subsystem (0x00-0xFF)
- Legacy compatibility testing
- Auto-threshold calculation
- Performance testing
- Memory usage validation

### RTOS Threading Examples (`eLog_example_rtos.c`)
- Thread safety demonstration
- Multi-subscriber coordination
- Task information integration
- RTOS-specific feature testing
- Mutex timeout behavior
- Performance impact measurement
- Error codes with RTOS context

### Task-Based Examples (`eLog_rtos_demo.c`)
- Sensor task logging with error codes
- Communication task logging with error codes
- Custom subscriber implementation
- Per-module threshold in RTOS context
- Real-world RTOS integration patterns

## 🚀 Performance

- **Zero overhead** for disabled log levels (compile-time elimination)
- **Minimal runtime cost** for enabled levels
- **Efficient subscriber pattern** for multiple outputs
- **Optimized for embedded** real-time constraints

## 🤝 Contributing

Contributions are welcome! Please follow these guidelines:

1. **Code Style**: Follow existing coding conventions
2. **Documentation**: Update README for new features
3. **Testing**: Test on multiple embedded platforms
4. **Backwards Compatibility**: Maintain existing API compatibility

### Development Setup
```bash
git clone <your-repo>
cd eLog
# Include in your embedded project build system
```

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Inspired by [uLog](https://github.com/rdpoor/ulog) by R. Dunbar Poor
- Enhanced for embedded systems and production use
- Designed for STM32, ESP32, and other ARM Cortex-M platforms

## 📞 Support

- **Issues**: Please use GitHub issues for bug reports and feature requests
- **Documentation**: See examples in `eLog_example.c`
- **Community**: Contributions and feedback welcome

---

Enhanced eLog - Advanced Logging System for Embedded MCU Projects (v0.05)
