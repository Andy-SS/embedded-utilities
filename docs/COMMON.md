# Common Utilities - Unified Mutex and RTOS Management

## Overview

`common.c` provides centralized utility functions for managing RTOS state and unified mutex operations across all embedded utilities modules (eLog, Ring, Bit). It acts as the glue layer between your RTOS implementation and the utilities.

## Core Features

- **Unified Mutex Interface** - Centralized mutex callback registration
- **RTOS Ready State Management** - Track when ThreadX scheduler is running
- **Per-Module Synchronization** - Each utility (eLog, Ring) uses the same callbacks
- **Graceful Fallback** - Operates safely in both RTOS and bare-metal modes
- **Zero Overhead** - Minimal runtime cost

## File Location

```
App/utilities/
├── common.c              # Implementation of utilities
├── common.h              # Header (included via utilities)
├── mutex_common.h        # Unified mutex interface definition
└── [eLog, Ring, Bit]/   # Modules using common utilities
```

## API Reference

### 1. Register Mutex Callbacks

```c
void utilities_register_cs_cbs(const mutex_callbacks_t *callbacks);
```

**Purpose:** Register RTOS mutex callback functions for all utilities

**Parameters:**
- `callbacks` - Pointer to `mutex_callbacks_t` structure with function pointers

**Usage:**
```c
#include "mutex_common.h"
extern const mutex_callbacks_t mutex_callbacks;  // From rtos.c

// Call once during initialization
utilities_register_cs_cbs(&mutex_callbacks);
```

**Notes:**
- Call this ONCE at application startup (in `tx_application_define`)
- Must be called before creating any eLog or Ring instances
- Both eLog and Ring will use these callbacks automatically

---

### 2. Check RTOS Ready State

```c
bool utilities_is_RTOS_ready(void);
```

**Purpose:** Determine if ThreadX scheduler is running and RTOS is safe to use

**Return Value:**
- `true` - RTOS scheduler is active, safe to use mutexes
- `false` - RTOS not ready (bare metal or scheduler not started)

**Usage:**
```c
if (utilities_is_RTOS_ready()) {
    ELOG_INFO(ELOG_MD_MAIN, "RTOS active - mutex operations available");
} else {
    ELOG_WARNING(ELOG_MD_MAIN, "RTOS not ready - using bare metal mode");
}
```

**Notes:**
- Used internally by eLog and Ring to decide whether to use mutexes
- Safe to call at any time
- Returns false until `utilities_set_RTOS_ready(true)` is called

---

### 3. Set RTOS Ready State

```c
void utilities_set_RTOS_ready(bool status);
```

**Purpose:** Signal that ThreadX scheduler has started

**Parameters:**
- `status` - `true` to enable RTOS mutex usage, `false` to disable

**Usage:**
```c
void tx_application_define(VOID *first_unused_memory) {
    // ... ThreadX initialization ...
    
    // Create ThreadX threads, mutexes, etc.
    
    // Signal to utilities that RTOS is ready
    utilities_set_RTOS_ready(true);
    
    // ... rest of application setup ...
}
```

**Notes:**
- Must be called in `tx_application_define` AFTER ThreadX scheduler initialization
- Set to `false` during shutdown if needed
- eLog and Ring check this flag before using mutexes

---

### 4. Create Mutex

```c
mutex_result_t utilities_mutex_create(void* mutex);
```

**Purpose:** Create a new mutex using registered callbacks

**Parameters:**
- `mutex` - Pointer to mutex handle (will be filled with new mutex)

**Return Value:**
```c
typedef enum {
  MUTEX_OK = 0,           // Success
  MUTEX_TIMEOUT,          // Timeout waiting for mutex
  MUTEX_ERROR,            // General error
  MUTEX_NOT_SUPPORTED     // RTOS not available
} mutex_result_t;
```

**Usage:**
```c
void *my_mutex = NULL;
mutex_result_t result = utilities_mutex_create(&my_mutex);

if (result == MUTEX_OK) {
    ELOG_INFO(ELOG_MD_MAIN, "Mutex created successfully");
} else {
    ELOG_ERROR(ELOG_MD_MAIN, "Failed to create mutex: %d", result);
}
```

**Notes:**
- Automatically uses registered callbacks
- Returns error if RTOS not ready and callbacks not registered
- Called internally by Ring buffer during initialization

---

### 5. Acquire (Lock) Mutex

```c
mutex_result_t utilities_mutex_take(void *mutex, uint32_t timeout_ms);
```

**Purpose:** Lock a mutex with optional timeout

**Parameters:**
- `mutex` - Mutex handle from `utilities_mutex_create()`
- `timeout_ms` - Timeout in milliseconds (use `UINT32_MAX` for infinite wait)

**Return Value:**
```c
MUTEX_OK         // Successfully locked
MUTEX_TIMEOUT    // Timeout occurred
MUTEX_ERROR      // Mutex invalid or error
```

**Usage:**
```c
// Lock with 500ms timeout
mutex_result_t result = utilities_mutex_take(my_mutex, 500);

if (result == MUTEX_OK) {
    // Critical section - mutex is held
    shared_resource++;
    
    // Must release!
    utilities_mutex_give(my_mutex);
} else if (result == MUTEX_TIMEOUT) {
    ELOG_WARNING(ELOG_MD_MAIN, "Mutex timeout");
} else {
    ELOG_ERROR(ELOG_MD_MAIN, "Mutex error");
}
```

**Notes:**
- If RTOS not ready, returns `MUTEX_ERROR`
- Always pair with `utilities_mutex_give()`
- If timeout is `UINT32_MAX`, waits forever (use carefully!)

---

### 6. Release (Unlock) Mutex

```c
mutex_result_t utilities_mutex_give(void *mutex);
```

**Purpose:** Unlock a mutex

**Parameters:**
- `mutex` - Mutex handle previously locked with `utilities_mutex_take()`

**Return Value:**
```c
MUTEX_OK    // Successfully released
MUTEX_ERROR // Mutex error
```

**Usage:**
```c
utilities_mutex_give(my_mutex);
```

**Notes:**
- MUST be called after `utilities_mutex_take()` succeeds
- Not reentrant by default (behavior depends on RTOS implementation)
- Call exactly once for each successful `utilities_mutex_take()`

---

### 7. Delete Mutex

```c
mutex_result_t utilities_mutex_delete(void *mutex);
```

**Purpose:** Destroy and free a mutex

**Parameters:**
- `mutex` - Mutex handle from `utilities_mutex_create()`

**Return Value:**
```c
MUTEX_OK    // Successfully deleted
MUTEX_ERROR // Error deleting mutex
```

**Usage:**
```c
mutex_result_t result = utilities_mutex_delete(my_mutex);

if (result == MUTEX_OK) {
    my_mutex = NULL;  // Good practice
    ELOG_INFO(ELOG_MD_MAIN, "Mutex deleted");
}
```

**Notes:**
- Called internally by Ring buffer during `ring_destroy()`
- Mutex should not be in use (not locked by any thread)
- Must not be used after deletion

---

## Implementation Examples

### Example 1: Basic Initialization

```c
// In app_entry.c or main initialization
#include "mutex_common.h"

void application_init(void) {
    // Register mutex callbacks (provided by rtos.c)
    extern const mutex_callbacks_t mutex_callbacks;
    utilities_register_cs_cbs(&mutex_callbacks);
    
    // Enable eLog thread-safe mode
    LOG_INIT_WITH_CONSOLE();
    
    ELOG_INFO(ELOG_MD_MAIN, "Application initializing");
}

void tx_application_define(VOID *first_unused_memory) {
    // ... ThreadX setup ...
    
    // Signal RTOS ready
    utilities_set_RTOS_ready(true);
    
    ELOG_INFO(ELOG_MD_MAIN, "ThreadX started");
}
```

### Example 2: Ring Buffer with Common Mutex

```c
#include "ring.h"
#include "mutex_common.h"

typedef struct {
    uint16_t sensor_id;
    uint32_t timestamp;
    int16_t value;
} SensorData;

ring_t sensor_ring;

void sensor_task(ULONG arg) {
    SensorData data;
    
    // Ring automatically creates per-instance mutex on init
    ring_init_dynamic(&sensor_ring, 128, sizeof(SensorData));
    
    while (1) {
        // Read sensor...
        data.sensor_id = 1;
        data.timestamp = tx_time_get();
        data.value = 42;
        
        // Write is automatically protected by ring's internal mutex
        ring_write(&sensor_ring, &data);
        
        // Delay
        tx_thread_sleep(1000);
    }
}

void consumer_task(ULONG arg) {
    SensorData data;
    
    while (1) {
        // Read is automatically protected by ring's internal mutex
        if (ring_read(&sensor_ring, &data) == RING_OK) {
            ELOG_INFO(ELOG_MD_MAIN, "Sensor %d: %d", 
                     data.sensor_id, data.value);
        }
        
        tx_thread_sleep(100);
    }
}
```

### Example 3: Thread-Safe eLog Logging

```c
#include "eLog.h"
#include "mutex_common.h"

void task_a(ULONG arg) {
    while (1) {
        ELOG_INFO(ELOG_MD_TASK_A, "Task A executing");
        tx_thread_sleep(1000);
    }
}

void task_b(ULONG arg) {
    while (1) {
        ELOG_INFO(ELOG_MD_TASK_B, "Task B executing");
        tx_thread_sleep(1500);
    }
}

void task_c(ULONG arg) {
    while (1) {
        ELOG_ERROR(ELOG_MD_TASK_C, "Task C error condition");
        tx_thread_sleep(2000);
    }
}
```

All logging calls are automatically thread-safe through common utilities!

---

## CMake Integration

### App/utilities/CMakeLists.txt

```cmake
# Create common library from common.c
add_library(common STATIC common.c)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link common to all modules
target_link_libraries(eLog PUBLIC common)
target_link_libraries(ring PUBLIC common)
```

### cmake/App/CMakeLists.txt

```cmake
# Include utilities subdirectory
add_subdirectory(${CMAKE_SOURCE_DIR}/App/utilities ${CMAKE_BINARY_DIR}/App/utilities)

# Link all libraries
target_link_libraries(App INTERFACE
    common
    eLog
    ring
    stm32cubemx
)
```

---

## Error Handling

### Handling RTOS Not Ready

```c
if (!utilities_is_RTOS_ready()) {
    // RTOS not running - log to console only
    ELOG_WARNING(ELOG_MD_MAIN, "RTOS unavailable");
    // Utilities will still work but without thread safety
    return;
}

// Safe to proceed with multi-threaded operations
```

### Handling Mutex Timeout

```c
mutex_result_t result = utilities_mutex_take(my_mutex, 500);

switch (result) {
    case MUTEX_OK:
        // Proceed with critical section
        break;
    
    case MUTEX_TIMEOUT:
        ELOG_WARNING(ELOG_MD_MAIN, "Mutex timeout - possible deadlock");
        break;
    
    case MUTEX_ERROR:
        ELOG_ERROR(ELOG_MD_MAIN, "Mutex error");
        break;
    
    case MUTEX_NOT_SUPPORTED:
        ELOG_ERROR(ELOG_MD_MAIN, "Mutexes not supported");
        break;
}
```

---

## Configuration

### Timeout Settings

Configure default timeout in `mutex_common.h`:

```c
#define MUTEX_TIMEOUT_MS 500  /* Default timeout in milliseconds */
```

### RTOS Type Selection

Select RTOS in `eLog.h`:

```c
#define ELOG_RTOS_TYPE ELOG_RTOS_THREADX  /* or FREERTOS, CMSIS, NONE */
```

---

## Best Practices

1. **Register callbacks early** - Call `utilities_register_cs_cbs()` in `tx_application_define()`
2. **Set RTOS ready flag** - Call `utilities_set_RTOS_ready(true)` after scheduler setup
3. **Let utilities manage mutexes** - Ring and eLog handle mutex creation/destruction
4. **Always release locks** - Match every `utilities_mutex_take()` with `utilities_mutex_give()`
5. **Check RTOS status** - Use `utilities_is_RTOS_ready()` before assuming mutexes available
6. **Use appropriate timeouts** - 500ms usually works; adjust for your application
7. **Test bare-metal mode** - Common utilities work without RTOS; test for graceful degradation

---

## Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| Mutex operations fail | Callbacks not registered | Call `utilities_register_cs_cbs()` in `tx_application_define()` |
| RTOS_READY always false | Flag not set | Call `utilities_set_RTOS_ready(true)` after ThreadX startup |
| Timeouts on mutex | Deadlock or high contention | Increase timeout or reduce critical section time |
| Ring operations hang | Mutex not initialized | Ensure `utilities_register_cs_cbs()` called first |
| eLog not thread-safe | RTOS mode disabled | Check `ELOG_THREAD_SAFE` compile flag |

---

## See Also

- [UNIFIED_MUTEX_GUIDE.md](UNIFIED_MUTEX_GUIDE.md) - Architecture overview
- [ELOG.md](ELOG.md) - eLog logging system
- [RING.md](RING.md) - Ring buffer implementation
- [mutex_common.h](../mutex_common.h) - Interface definitions
