# Ring Buffer Documentation

A high-performance, thread-safe circular buffer implementation for embedded systems.

## Overview

The Ring Buffer (circular buffer) is a fixed-size data structure that efficiently stores and retrieves data in FIFO (First-In-First-Out) order. It's ideal for:
- Data streaming and buffering
- Producer-consumer patterns
- Interrupt handler data queuing
- Sensor data pipelines
- BLE communication

## Features

- **Static and Dynamic Allocation** - Choose pre-allocated or dynamic buffers
- **Fixed Capacity** - Predictable memory usage for embedded systems
- **Per-Instance Thread Safety** - Each ring buffer has its own mutex (via callbacks) for independent synchronization
- **Efficient Operations** - O(1) read/write/peek performance
- **Multiple Data Types** - Works with any data type via element_size
- **High-Performance Transfers** - Ring-to-ring copying for batch processing
- **Platform Independent** - No hard dependencies on RTOS headers

## Initialization

### Static Allocation (Pre-allocated buffer)

```c
MyData buffer[128];
ring_t ring;
ring_init(&ring, buffer, 128, sizeof(MyData));
```

**Pros:** No dynamic allocation, predictable memory usage
**Cons:** Fixed at compile time

### Dynamic Allocation (Runtime allocation)

```c
ring_t ring;
if (ring_init_dynamic(&ring, 128, sizeof(MyData))) {
    // Ring buffer ready to use
    // Remember to call ring_destroy() when done
}
```

**Pros:** Flexible sizing at runtime
**Cons:** Requires cleanup with `ring_destroy()`

## Thread-Safe Usage with Unified Mutex Callbacks (NEW!)

### Unified Callback Interface

Ring buffer now uses the unified `mutex_callbacks_t` interface from `mutex_common.h`:

```c
/* Same interface used by eLog and Ring */
typedef struct {
  void* (*create)(void);                           /* Create mutex */
  void (*destroy)(void *mutex);                    /* Destroy mutex */
  mutex_result_t (*acquire)(void *mutex, uint32_t timeout_ms);  /* Lock with timeout */
  mutex_result_t (*release)(void *mutex);          /* Unlock */
} mutex_callbacks_t;
```

### Setup with Unified Callbacks

Register the callbacks once during initialization:

```c
#include "ring.h"
#include "mutex_common.h"

// ThreadX mutex callbacks (same as eLog!)
extern const mutex_callbacks_t mutex_callbacks;

void app_init(void) {
    // Register the unified callbacks (same interface for both eLog and Ring!)
    ring_register_cs_callbacks(&mutex_callbacks);
    
    // Create ring buffers - each will automatically get its own mutex
    ring_t data_ring;
    ring_init_dynamic(&data_ring, 256, sizeof(MyData));
    
    ring_t command_ring;
    ring_init_dynamic(&command_ring, 128, sizeof(MyData));
}
```

### How Per-Instance Mutexes Work

- **One-time registration**: Call `ring_register_cs_callbacks()` once at app startup
- **Per-instance mutexes**: Each `ring_init()` or `ring_init_dynamic()` calls `create()` to get a unique mutex
- **Independent synchronization**: Each ring buffer protects its own operations without affecting others
- **Automatic cleanup**: `ring_destroy()` calls `destroy()` to clean up each mutex
- **Shared with eLog**: Use the same `mutex_callbacks_t` for both eLog and Ring!

### Available Platform Callbacks

- **ThreadX**: Implemented in `rtos.c` - `const mutex_callbacks_t mutex_callbacks`
- **FreeRTOS**: Create via platform-specific callback implementation
- **ESP32**: Create via platform-specific callback implementation
- **Bare Metal**: Pass `NULL` to `ring_register_cs_callbacks()` for non-thread-safe operation

### Usage Example (Operations Are Automatically Protected)

```c
#include "ring.h"
#include "mutex_common.h"

extern const mutex_callbacks_t mutex_callbacks;

void producer_task(void) {
    ring_t *ring = &data_ring;
    
    uint32_t data = 42;
    // Automatically locked/unlocked by ring buffer
    ring_write(ring, &data);
}

void consumer_task(void) {
    ring_t *ring = &data_ring;
    
    uint32_t data;
    // Automatically locked/unlocked by ring buffer
    if (ring_read(ring, &data)) {
        printf("Received: %u\n", data);
    }
}
```

### Multi-Module Integration

Both eLog and Ring use the same callbacks, so you only need to register once:

```c
void rtos_init(void) {
    // Register ONCE - both modules use it!
    elog_register_mutex_callbacks(&mutex_callbacks);
    ring_register_cs_callbacks(&mutex_callbacks);
    
    elog_update_RTOS_ready(true);
}
```

## Basic Operations

### Write Operations

```c
// Write single element (fails if full)
MyData value = {...};
if (!ring_write(&ring, &value)) {
    // Buffer is full, handle overflow
}

// Write with overwrite (always succeeds, discards oldest if full)
ring_push_front(&ring, &value);

// Write multiple elements with overwrite
MyData values[5] = {...};
uint32_t written = ring_push_back(&ring, values, 5);
```

### Read Operations (Destructive)

```c
// Read single element (removes from buffer)
MyData out;
if (ring_read(&ring, &out)) {
    // Successfully read oldest element
}

// Read multiple elements
MyData buffer[10];
uint32_t read_count = ring_read_multiple(&ring, buffer, 10);

// Pop from back (remove newest)
bool success = ring_pop_back(&ring);
```

### Peek Operations (Non-Destructive)

```c
// Peek oldest element without removing
MyData out;
if (ring_peek_front(&ring, &out)) {
    // Got oldest element, still in ring
}

// Peek newest element without removing
if (ring_peek_back(&ring, &out)) {
    // Got newest element, still in ring
}

// Peek multiple elements from front (oldest first)
MyData peek_array[10];
uint32_t peeked = ring_peek_front_multiple(&ring, peek_array, 10);

// Peek multiple elements from back (newest first)
uint32_t peeked_back = ring_peek_back_multiple(&ring, peek_array, 10);
```

## Status Checking

```c
// Check element count
uint32_t available = ring_available(&ring);
uint32_t free_space = ring_get_free(&ring);

// Check state
bool is_empty = ring_is_empty(&ring);
bool is_full = ring_is_full(&ring);
bool owns_buffer = ring_is_owns_buffer(&ring);

// Clear buffer
ring_clear(&ring);
```

## Ring-to-Ring Operations (High Performance)

```c
ring_t source, destination;

// Copy all data (preserve source)
uint32_t copied = ring_dump(&source, &destination, true);

// Move all data (consume source)
uint32_t moved = ring_dump(&source, &destination, false);

// Copy limited number of elements
uint32_t batch = ring_dump_count(&source, &destination, 50, false);
```

## Real-World Example: Multi-Stage Data Pipeline with Independent Mutexes

```c
typedef struct {
    uint32_t timestamp;
    uint16_t sensor_id;
    float value;
} SensorData_t;

// Create processing stages - each has independent thread safety
ring_t raw_data, filtered_data, processed_data, tx_queue;

void app_init(void) {
  // Register ThreadX critical section callbacks (one-time)
  ring_register_cs_callbacks(&threadx_cs_callbacks);
  
  // Initialize all rings - each creates its own mutex automatically
  ring_init_dynamic(&raw_data, 100, sizeof(SensorData_t));
  ring_init_dynamic(&filtered_data, 50, sizeof(SensorData_t));
  ring_init_dynamic(&processed_data, 50, sizeof(SensorData_t));
  ring_init_dynamic(&tx_queue, 20, sizeof(SensorData_t));
}

void producer_task(void *arg) {
  SensorData_t reading;
  
  while (1) {
    // Read from sensor
    sensor_read(&reading);
    
    // Write to raw_data - automatically protected by its own mutex
    ring_push_front(&raw_data, &reading);
    
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void filter_task(void *arg) {
  // Filter data and move to filtered_data
  // Each ring buffer uses its own independent mutex
  while (1) {
    uint32_t copied = ring_dump(&raw_data, &filtered_data, true);
    if (copied == 0) {
      vTaskDelay(pdMS_TO_TICKS(5));
    }
  }
}

void process_task(void *arg) {
  while (1) {
    // Process and move data
    uint32_t moved = ring_dump(&filtered_data, &processed_data, false);
    
    // Batch transmission
    if (ring_available(&processed_data) >= 10) {
      uint32_t tx_batch = ring_dump_count(&processed_data, &tx_queue, 10, false);
      transmit_batch(&tx_queue);
    }
    
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void app_cleanup(void) {
  // Cleanup destroys each buffer's individual mutex
  ring_destroy(&raw_data);
  ring_destroy(&filtered_data);
  ring_destroy(&processed_data);
  ring_destroy(&tx_queue);
}
```

## Mutex Management

The ring buffer automatically creates and manages per-instance mutexes:

```c
// Register critical section callbacks at app startup
ring_register_cs_callbacks(&threadx_cs_callbacks);

// Create ring buffer - automatically creates a mutex for this instance
ring_t my_ring;
ring_init_dynamic(&my_ring, 128, sizeof(uint32_t));
// my_ring.mutex is now initialized and ready

// Use operations normally - mutex is handled internally
ring_write(&my_ring, &data);
ring_read(&my_ring, &output);

// Cleanup - automatically destroys this ring's mutex
ring_destroy(&my_ring);
// my_ring.mutex is cleaned up and set to NULL
```

### Multiple Independent Ring Buffers

Each ring buffer maintains its own mutex, allowing truly independent thread-safe access:

```c
ring_t uart_rx, uart_tx, ble_rx, ble_tx;

// Register callbacks once
ring_register_cs_callbacks(&threadx_cs_callbacks);

// Each creates its own mutex
ring_init_dynamic(&uart_rx, 256, sizeof(uint8_t));
ring_init_dynamic(&uart_tx, 256, sizeof(uint8_t));
ring_init_dynamic(&ble_rx, 512, sizeof(uint8_t));
ring_init_dynamic(&ble_tx, 512, sizeof(uint8_t));

// Independent synchronization - no contention between buffers
ring_write(&uart_rx, &data);  // Uses uart_rx.mutex
ring_write(&ble_rx, &data);   // Uses ble_rx.mutex (different lock)
```

## Memory Management

```c
// For dynamically allocated rings
ring_destroy(&ring);  // Frees the buffer and clears structure

// For static rings
// No explicit cleanup needed, buffer is managed externally
```

## Performance Characteristics

| Operation | Time Complexity | Notes |
|-----------|-----------------|-------|
| Write | O(1) | Constant time, regardless of buffer size |
| Read | O(1) | Constant time, regardless of buffer size |
| Peek | O(1) | Non-destructive, doesn't modify buffer |
| DumpToRing | O(n) | Proportional to elements copied |
| Clear | O(1) | Just resets pointers |

## Best Practices

1. **Register callbacks at app startup** - Call `ring_register_cs_callbacks()` once during initialization
2. **Each ring buffer is independent** - No mutex sharing between buffers, no contention
3. **Always destroy dynamically allocated rings** - Call `ring_destroy()` to clean up mutex and buffer memory
4. **Check return values** - Write/Read may fail if buffer is full/empty
5. **Use DumpToRing for batch operations** - More efficient than multiple individual operations
6. **No manual critical sections needed** - Mutex operations are handled automatically
7. **Supports bare metal** - Pass `NULL` callbacks for non-thread-safe operation
8. **Platform agnostic** - Switch RTOS by changing callback registration

## Limitations

- Fixed size (cannot grow dynamically after initialization)
- Element size fixed at initialization
- No built-in error recovery (overwritten data is lost)
- Requires callback registration for thread safety (but can be NULL for bare metal)

## Callback Registration and Platform Support

### Registering Critical Section Callbacks

Before using any ring buffer operations, register the platform-specific critical section callbacks:

```c
// ThreadX
#include "ring_critical_section_threadx.h"
ring_register_cs_callbacks(&threadx_cs_callbacks);

// FreeRTOS
#include "ring_critical_section_freertos.h"
ring_register_cs_callbacks(&freertos_cs_callbacks);

// ESP32 (FreeRTOS with spinlocks)
#include "ring_critical_section_esp32.h"
ring_register_cs_callbacks(&esp32_cs_callbacks);

// Bare Metal (no synchronization)
ring_register_cs_callbacks(NULL);
```

### How Callbacks Work

Each platform provides callbacks that implement:
- `create()` - Allocate and initialize a new mutex
- `destroy(mutex)` - Clean up and free a mutex
- `enter(mutex)` - Lock the mutex
- `exit(mutex)` - Unlock the mutex

When you call `ring_init()` or `ring_init_dynamic()`, the ring buffer uses `create()` to make a unique mutex for that instance. When you call `ring_destroy()`, it uses `destroy()` to clean up.

### Platform-Specific Notes

**ThreadX:**
- Each ring buffer gets its own `TX_MUTEX`
- Supports `TX_WAIT_FOREVER` blocking
- Requires `#include "tx_api.h"` in the callback file

**FreeRTOS:**
- Each ring buffer gets its own `SemaphoreHandle_t`
- Supports both task and ISR contexts
- Use with standard FreeRTOS semphr.h

**ESP32:**
- Each ring buffer gets its own `portMUX_TYPE` spinlock
- ISR-safe operations
- Recommended for high-frequency BLE/UART operations
- Better performance than mutex for very short critical sections

**Bare Metal:**
- No mutex created or used
- Ring buffers work fine in single-threaded contexts
- Suitable for interrupt-driven systems without RTOS

## Initialization Examples

**ThreadX Callback Implementation** (`ring_critical_section_threadx.c`):

```c
#include "ring.h"
#include "tx_api.h"
#include <stdlib.h>

/* Create a new mutex for a ring buffer instance */
void* threadx_cs_create(void) {
  TX_MUTEX *mutex = (TX_MUTEX *)malloc(sizeof(TX_MUTEX));
  if (mutex == NULL) {
    return NULL;
  }
  
  UINT status = tx_mutex_create(mutex, "Ring_Mutex", TX_NO_INHERIT);
  if (status != TX_SUCCESS) {
    free(mutex);
    return NULL;
  }
  
  return (void *)mutex;
}

/* Destroy a ring buffer's mutex */
void threadx_cs_destroy(void *mutex) {
  if (mutex == NULL) return;
  
  TX_MUTEX *tx_mutex = (TX_MUTEX *)mutex;
  tx_mutex_delete(tx_mutex);
  free(tx_mutex);
}

/* Lock a specific mutex */
ring_cs_result_t threadx_cs_enter(void *mutex) {
  if (mutex == NULL) {
    return RING_CS_ERROR;
  }
  
  TX_MUTEX *tx_mutex = (TX_MUTEX *)mutex;
  UINT status = tx_mutex_get(tx_mutex, TX_WAIT_FOREVER);
  return (status == TX_SUCCESS) ? RING_CS_OK : RING_CS_ERROR;
}

/* Unlock a specific mutex */
ring_cs_result_t threadx_cs_exit(void *mutex) {
  if (mutex == NULL) {
    return RING_CS_ERROR;
  }
  
  TX_MUTEX *tx_mutex = (TX_MUTEX *)mutex;
  UINT status = tx_mutex_put(tx_mutex);
  return (status == TX_SUCCESS) ? RING_CS_OK : RING_CS_ERROR;
}

/* Register callbacks with ring buffer */
const ring_cs_callbacks_t threadx_cs_callbacks = {
  .create = threadx_cs_create,
  .destroy = threadx_cs_destroy,
  .enter = threadx_cs_enter,
  .exit = threadx_cs_exit
};
```

### ThreadX Application

```c
#include "ring.h"
#include "ring_critical_section_threadx.h"

void app_init(void) {
  // Register ThreadX callbacks
  ring_register_cs_callbacks(&threadx_cs_callbacks);
  
  // Create thread-safe ring buffers
  ring_t sensor_data;
  ring_init_dynamic(&sensor_data, 256, sizeof(sensor_reading_t));
  
  // Now all operations are thread-safe
}
```

### ESP32 Application

```c
#include "ring.h"
#include "ring_critical_section_esp32.h"

void app_init(void) {
  // Register ESP32 spinlock callbacks
  ring_register_cs_callbacks(&esp32_cs_callbacks);
  
  // Create ISR-safe ring buffers
  ring_t ble_rx_ring;
  ring_init_dynamic(&ble_rx_ring, 512, sizeof(ble_packet_t));
  
  // Safe in both tasks and ISRs
}

void ble_rx_callback(uint8_t *data, uint16_t len) {
  // Called from BLE ISR
  for (uint16_t i = 0; i < len; i++) {
    ring_write(&ble_rx_ring, &data[i]);  // ISR-safe
  }
}
```

### Bare Metal Application

```c
#include "ring.h"

void app_init(void) {
  // No callbacks needed for bare metal
  ring_register_cs_callbacks(NULL);
  
  // Ring buffer is not thread-safe, but works fine
  ring_t data_buffer;
  ring_init_dynamic(&data_buffer, 128, sizeof(sensor_data_t));
}
```

