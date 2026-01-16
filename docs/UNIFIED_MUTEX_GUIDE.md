# Unified Mutex Callback Interface Guide

## Overview

The utilities submodules (eLog and Ring) now use a **unified mutex callback interface** defined in `mutex_common.h`. This allows both modules to share the same RTOS mutex implementation without code duplication.

## What Changed

### Before (Old Separate Interfaces)
```c
// eLog had its own mutex interface
typedef elog_mutex_result_t (*elog_mutex_create_fn)(void);
typedef elog_mutex_result_t (*elog_mutex_take_fn)(uint32_t timeout_ms);
typedef elog_mutex_result_t (*elog_mutex_give_fn)(void);
typedef elog_mutex_result_t (*elog_mutex_delete_fn)(void);

// Ring had different interface
typedef ring_cs_result_t (*ring_cs_enter_fn)(void *mutex);
typedef ring_cs_result_t (*ring_cs_exit_fn)(void *mutex);
typedef void* (*ring_cs_create_fn)(void);
typedef void (*ring_cs_destroy_fn)(void *mutex);
```

### After (New Unified Interface)
```c
/* Single interface used by both eLog and Ring */
typedef struct {
  void* (*create)(void);                           /* Create mutex */
  void (*destroy)(void *mutex);                    /* Destroy mutex */
  mutex_result_t (*acquire)(void *mutex, uint32_t timeout_ms);  /* Lock with timeout */
  mutex_result_t (*release)(void *mutex);          /* Unlock */
} mutex_callbacks_t;
```

## Key Improvements

| Feature | Benefit |
|---------|---------|
| **Single Interface** | One callback struct replaces multiple definitions |
| **Code Reuse** | Same ThreadX implementation used by both eLog and Ring |
| **Per-Instance Mutexes** | Each ring buffer gets its own mutex automatically |
| **Timeout Support** | Acquire can specify timeout in milliseconds |
| **Consistent Error Codes** | Unified `mutex_result_t` enum for both modules |
| **CMake Integration** | Parent utilities directory automatically exposed |

## File Structure

```
App/utilities/
├── mutex_common.h              # Unified interface (NEW!)
├── eLog/
│   ├── eLog.h
│   ├── eLog.c
│   └── CMakeLists.txt
├── ring/
│   ├── ring.h
│   ├── ring.c
│   └── CMakeLists.txt
├── examples/
│   └── eLog/
│       ├── eLog_rtos_demo.c    # UPDATED to use unified interface
│       ├── eLog_example.c
│       ├── eLog_example_rtos.c
│       └── eLog_config_examples.h
└── docs/
    ├── ELOG.md                 # UPDATED with unified mutex info
    ├── RING.md                 # UPDATED with unified mutex info
    └── README.md               # UPDATED with usage examples
```

## Implementation in Your Project

### Step 1: Include the Unified Header

```c
#include "mutex_common.h"
```

### Step 2: Implement Callbacks (ThreadX Example)

Located in `App/rtos.c`:

```c
#include "mutex_common.h"
#include "tx_api.h"

void* threadx_mutex_create(void) {
  TX_MUTEX *mutex = (TX_MUTEX *)malloc(sizeof(TX_MUTEX));
  if (mutex == NULL) return NULL;
  
  UINT status = tx_mutex_create(mutex, "Mutex", TX_NO_INHERIT);
  return (status == TX_SUCCESS) ? (void *)mutex : NULL;
}

mutex_result_t threadx_mutex_destroy(void *mutex) {
  if (mutex == NULL) return MUTEX_ERROR;
  TX_MUTEX *tx_mutex = (TX_MUTEX *)mutex;
  UINT status = tx_mutex_delete(tx_mutex);
  free(tx_mutex);
  return (status == TX_SUCCESS) ? MUTEX_OK : MUTEX_ERROR;
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

/* Unified callbacks structure */
const mutex_callbacks_t mutex_callbacks = {
  .create = threadx_mutex_create,
  .destroy = threadx_mutex_destroy,
  .acquire = threadx_mutex_acquire,
  .release = threadx_mutex_release
};
```

### Step 3: Register Callbacks (in tx_application_define)

```c
void tx_application_define(VOID *first_unused_memory) {
    // ... other ThreadX initialization ...
    
    #if (ELOG_THREAD_SAFE == 1)
    // Register ONCE - both eLog and Ring use the same callbacks!
    elog_register_mutex_callbacks(&mutex_callbacks);
    ring_register_cs_callbacks(&mutex_callbacks);
    #endif
    
    elog_update_RTOS_ready(true);
    
    // ... rest of initialization ...
}
```

### Step 4: Use in Your Code

```c
// eLog (thread-safe automatically)
ELOG_INFO(ELOG_MD_MAIN, "This is thread-safe");

// Ring (each buffer gets its own mutex)
ring_t my_ring;
ring_init_dynamic(&my_ring, 256, sizeof(uint8_t));

uint8_t data = 42;
ring_write(&my_ring, &data);  // Automatically locked/unlocked
```

## CMake Configuration

The CMakeLists.txt files have been updated to expose `mutex_common.h`:

### App/utilities/CMakeLists.txt
```cmake
add_subdirectory(eLog)
add_subdirectory(ring)
add_subdirectory(bit)

# Make mutex_common.h available to all modules
target_include_directories(eLog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ring PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

### cmake/App/CMakeLists.txt
```cmake
target_include_directories(App INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Inc>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/App/utilities>
    # ... other includes ...
)
```

## Error Codes

All modules now use unified error codes:

```c
typedef enum {
  MUTEX_OK = 0,              /* Operation successful */
  MUTEX_TIMEOUT,             /* Timeout while waiting for mutex */
  MUTEX_ERROR,               /* General mutex error */
  MUTEX_NOT_SUPPORTED        /* RTOS/mutex not available */
} mutex_result_t;
```

## Benefits for Ring Buffer

Ring buffer now gets per-instance mutexes:

```c
ring_t uart_ring;
ring_init_dynamic(&uart_ring, 256, sizeof(uint8_t));
// uart_ring gets its own mutex automatically

ring_t sensor_ring;
ring_init_dynamic(&sensor_ring, 128, sizeof(sensor_t));
// sensor_ring gets its own SEPARATE mutex

// No mutex contention between buffers!
ring_write(&uart_ring, &data1);      // Uses uart_ring's mutex
ring_write(&sensor_ring, &data2);    // Uses sensor_ring's mutex independently
```

## Benefits for eLog

eLog now supports configurable timeout:

```c
// Before: Always waited forever
// Now: Can specify timeout
elog_register_mutex_callbacks(&mutex_callbacks);

// If mutex is held > 500ms, timeout and continue
// (see ELOG_MUTEX_TIMEOUT_MS in eLog.h)
```

## Migration Checklist

If you were using the old separate interfaces:

- ✅ Include `mutex_common.h` instead of eLog/ring specific headers
- ✅ Update ThreadX callbacks to match new function signatures
- ✅ Update `target_include_directories` in CMake if needed
- ✅ Call `ring_register_cs_callbacks(&mutex_callbacks)` instead of old API
- ✅ Use `mutex_result_t` instead of `elog_mutex_result_t` or `ring_cs_result_t`
- ✅ Verify build completes without errors

## Documentation Updates

The following documentation files have been updated:

| File | Updates |
|------|---------|
| `README.md` | Added unified mutex section and examples |
| `SUBMODULE_USAGE.md` | Updated examples to use unified interface |
| `docs/ELOG.md` | Added unified mutex callback section |
| `docs/RING.md` | Added unified mutex callback section |
| `examples/eLog/eLog_rtos_demo.c` | Rewritten with unified interface |

## Example Applications

### Bare Metal (No RTOS)

```c
#include "eLog.h"
#include "ring.h"

int main(void) {
    // Skip mutex registration for bare metal
    LOG_INIT_WITH_CONSOLE();
    
    ring_t data_ring;
    ring_init_dynamic(&data_ring, 256, sizeof(uint8_t));
    
    ELOG_INFO(ELOG_MD_MAIN, "Running without RTOS");
    
    return 0;
}
```

### ThreadX Multi-Task

```c
#include "eLog.h"
#include "ring.h"
#include "mutex_common.h"
#include "tx_api.h"

extern const mutex_callbacks_t mutex_callbacks;

void task1(ULONG input) {
    while(1) {
        ELOG_INFO(ELOG_MD_TASK_A, "Task 1 running");
        tx_thread_sleep(100);
    }
}

void task2(ULONG input) {
    ring_t my_ring = {0};
    ring_init_dynamic(&my_ring, 256, sizeof(uint8_t));
    
    while(1) {
        uint8_t data = 42;
        ring_write(&my_ring, &data);
        tx_thread_sleep(100);
    }
}

void tx_application_define(VOID *first_unused_memory) {
    // Register callbacks ONCE for both modules
    elog_register_mutex_callbacks(&mutex_callbacks);
    ring_register_cs_callbacks(&mutex_callbacks);
    elog_update_RTOS_ready(true);
    
    // Create tasks...
}
```

## Troubleshooting

### Build Error: "mutex_common.h: No such file or directory"

**Solution**: Verify CMakeLists.txt includes are correct:
```cmake
target_include_directories(eLog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
```

### Compilation Error: "incompatible pointer type"

**Solution**: Ensure your callback functions match `mutex_callbacks_t` signature exactly:
- `create`: returns `void*`
- `destroy`: takes `void *mutex`, returns nothing
- `acquire`: takes `void *mutex` and `uint32_t timeout_ms`, returns `mutex_result_t`
- `release`: takes `void *mutex`, returns `mutex_result_t`

## Summary

✅ Single unified mutex interface for both eLog and Ring
✅ Per-instance mutexes for each ring buffer
✅ Timeout support for all mutex operations
✅ Zero code duplication between modules
✅ CMake properly configured for automatic header discovery
✅ Comprehensive examples and documentation updated
✅ ThreadX implementation provided in `rtos.c`

Happy logging and buffering! 🎉
