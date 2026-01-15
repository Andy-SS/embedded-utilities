# Embedded C Utilities

A collection of lightweight, modular C utilities for embedded firmware development with unified RTOS synchronization support.

## Features

- **eLog** - Thread-safe logging system with configurable levels and multiple subscribers
- **Ring** - Per-instance circular buffer with RTOS-aware critical sections (v0.02 - unified mutex interface)
- **Bit** - Bit manipulation utilities
- **Mutex Common** - Unified mutex callback interface for consistent RTOS integration across all modules
- **Common Utilities** - Shared utility functions for thread-safe mutex operations
- **ThreadX Integration** - Unified memory allocation via byte pools
- Additional utilities for common firmware tasks

## Getting Started

### Building

Using CMake:
```bash
mkdir build
cd build
cmake ..
make
```

### Unified Mutex Support with common.c

All modules now use a unified `mutex_callbacks_t` interface defined in `mutex_common.h` with common utility functions in `common.c`:

```c
/* Unified mutex callback structure - shared by eLog and Ring */
typedef struct {
  void* (*create)(void);                           /* Create mutex */
  void (*destroy)(void *mutex);                    /* Destroy mutex */
  mutex_result_t (*acquire)(void *mutex, uint32_t timeout_ms);  /* Lock with timeout */
  mutex_result_t (*release)(void *mutex);          /* Unlock */
} mutex_callbacks_t;
```

Common utility functions provided by `common.c`:

```c
/**
 * @brief Register RTOS mutex callbacks for all utilities
 * @param callbacks Pointer to mutex callback structure
 */
void utilities_register_cs_cbs(const mutex_callbacks_t *callbacks);

/**
 * @brief Check if RTOS is ready for mutex operations
 * @return true if RTOS scheduler is running
 */
bool utilities_is_RTOS_ready(void);

/**
 * @brief Set RTOS ready state
 * @param status true when ThreadX scheduler starts
 */
void utilities_set_RTOS_ready(bool status);

/**
 * @brief Create a mutex using registered callbacks
 * @return MUTEX_OK on success
 */
mutex_result_t utilities_mutex_create(void* mutex);

/**
 * @brief Acquire (lock) a mutex with timeout
 * @param mutex Pointer to mutex
 * @param timeout_ms Timeout in milliseconds
 * @return MUTEX_OK, MUTEX_TIMEOUT, or MUTEX_ERROR
 */
mutex_result_t utilities_mutex_take(void *mutex, uint32_t timeout_ms);

/**
 * @brief Release (unlock) a mutex
 * @param mutex Pointer to mutex
 * @return MUTEX_OK or MUTEX_ERROR
 */
mutex_result_t utilities_mutex_give(void *mutex);

/**
 * @brief Delete a mutex
 * @param mutex Pointer to mutex
 * @return MUTEX_OK or MUTEX_ERROR
 */
mutex_result_t utilities_mutex_delete(void *mutex);
```

This allows both eLog and Ring to share the same callback implementation!

### ThreadX Memory Allocation Functions

The utilities integrate with ThreadX byte pool memory management:

```c
/**
 * @brief Allocate memory from the ThreadX byte pool
 *        Only use after ThreadX scheduler has started
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* byte_allocate(size_t size);

/**
 * @brief Release memory back to the ThreadX byte pool
 *        Only use after ThreadX scheduler has started
 * @param ptr Pointer to memory allocated via byte_allocate()
 */
void byte_release(void *ptr);
```

### Quick Usage

**Mutex Registration (in tx_application_define):**
```c
#include "mutex_common.h"

extern const mutex_callbacks_t mutex_callbacks;

void tx_application_define(VOID *first_unused_memory) {
    // Register unified callbacks for all utilities
    utilities_register_cs_cbs(&mutex_callbacks);
    
    // Set RTOS ready flag after scheduler starts
    utilities_set_RTOS_ready(true);
    
    // ... rest of initialization ...
}
```

**Memory Allocation (ThreadX Integration):**
```c
#include "rtos.h"  // Provides byte_allocate() and byte_release()

void app_task(void) {
    // Allocate buffer from ThreadX byte pool
    uint8_t *buffer = (uint8_t *)byte_allocate(1024);
    if (buffer == NULL) {
        // Handle allocation failure
        return;
    }
    
    // Use buffer...
    
    // Return memory to pool
    byte_release(buffer);
}
```

**Logging with eLog (Thread-Safe):**
```c
#include "eLog.h"
#include "mutex_common.h"

int main(void) {
    LOG_INIT_WITH_CONSOLE();
    
    ELOG_INFO(ELOG_MD_MAIN, "System started");
    ELOG_DEBUG(ELOG_MD_MAIN, "Debug message");
    ELOG_ERROR(ELOG_MD_MAIN, "Error occurred");
    
    return 0;
}
```

**Ring Buffer with Unified Mutex (Thread-Safe):**
```c
#include "ring.h"
#include "mutex_common.h"

ring_t sensor_data_ring;

void sensor_task(void) {
    // Each ring buffer automatically gets its own mutex
    ring_init_dynamic(&sensor_data_ring, 256, sizeof(SensorData));
    
    SensorData data;
    // Write is automatically protected by per-instance mutex
    ring_write(&sensor_data_ring, &data);
    
    // Read is automatically protected by per-instance mutex
    ring_read(&sensor_data_ring, &data);
    
    ring_destroy(&sensor_data_ring);
}
```

## Module Overview

- **eLog** - See [ELOG.md](ELOG.md) for comprehensive logging documentation
- **Ring** - See [RING.md](RING.md) for circular buffer documentation
- **Unified Mutex** - See [UNIFIED_MUTEX_GUIDE.md](UNIFIED_MUTEX_GUIDE.md) for integration details
- **Common** - Utilities functions in `common.c` provide centralized mutex and RTOS state management    return 0;
}
```

**Ring Buffer (Per-Instance Mutex with Unified Callbacks):**
```c
#include "ring.h"
#include "mutex_common.h"

// Register same callbacks for ring buffer
extern const mutex_callbacks_t mutex_callbacks;
ring_register_cs_callbacks(&mutex_callbacks);

int main(void) {
    // Create ring buffers - each gets its own mutex automatically
    ring_t uart_rx_ring;
    ring_init_dynamic(&uart_rx_ring, 256, sizeof(uint8_t));
    
    ring_t data_ring;
    ring_init_dynamic(&data_ring, 512, sizeof(float32_t));
    
    // Use ring buffer - synchronization is automatic
    uint8_t byte = 0xAB;
    ring_write(&uart_rx_ring, &byte);
    
    float32_t value = 3.14f;
    ring_write(&data_ring, &value);
    
    // Each ring buffer maintains independent thread safety
    
    return 0;
}
```

**Bit Operations:**
```c
#include "bit.h"

// Efficient bit manipulation utilities
```

## Documentation

- [eLog Documentation](docs/ELOG.md) - Thread-safe logging with mutex integration
- [Ring Buffer Documentation](docs/RING.md) - Circular buffer with unified mutex callbacks
- [Unified Mutex Guide](UNIFIED_MUTEX_GUIDE.md) - Detailed interface and integration guide
- [Ring Buffer Usage](RING_USAGE.md) - Comprehensive ring buffer usage examples

## Directory Structure

```
├── eLog/                      # Thread-safe logging module
├── ring/                      # Circular buffer module (v0.02)
├── bit/                       # Bit manipulation utilities
├── docs/                      # Documentation
│   ├── ELOG.md               # eLog API documentation
│   └── RING.md               # Ring buffer API documentation
├── examples/                  # Usage examples
├── tests/                     # Unit tests
├── mutex_common.h            # Unified mutex callback interface
├── UNIFIED_MUTEX_GUIDE.md    # Integration guide for unified callbacks
├── RING_USAGE.md             # Ring buffer usage guide (NEW!)
└── README.md                 # This file
```

## Ring Buffer v0.02 - What's New

### Breaking Changes from v0.01
- Old `ring_cs_callbacks_t` interface replaced with `mutex_callbacks_t`
- Callback function names changed:
  - `ring_cs_enter()` → `acquire()` with timeout support
  - `ring_cs_exit()` → `release()`
  - `ring_cs_create()` → `create()`
  - `ring_cs_destroy()` → `destroy()`

### Migration Guide
```c
// Old code (v0.01)
ring_register_cs_callbacks(&threadx_cs_callbacks);  // Old interface

// New code (v0.02)
ring_register_cs_callbacks(&mutex_callbacks);  // Unified interface from mutex_common.h
```

See [RING_USAGE.md](RING_USAGE.md) for detailed migration examples and best practices.

## License

This project is dual-licensed under MIT and Apache 2.0 licenses. You may use this software under either license.

- [MIT License](LICENSE)
- [Apache 2.0 License](LICENSE-APACHE)

## Contributing

Contributions are welcome! Please ensure:
- Code follows the existing style
- New utilities include tests
- Documentation is updated
- License headers are maintained

## Requirements

- C99 or later compiler
- CMake 3.10+ (optional, for building)
- ThreadX (for RTOS integration)

## Author

Created for embedded firmware development.
