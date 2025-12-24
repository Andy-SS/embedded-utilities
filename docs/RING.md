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
RingBuffer_t ring;
RingBuffer_Init(&ring, buffer, 128, sizeof(MyData));
```

**Pros:** No dynamic allocation, predictable memory usage
**Cons:** Fixed at compile time

### Dynamic Allocation (Runtime allocation)

```c
RingBuffer_t ring;
if (RingBuffer_InitDynamic(&ring, 128, sizeof(MyData))) {
    // Ring buffer ready to use
    // Remember to call RingBuffer_Destroy() when done
}
```

**Pros:** Flexible sizing at runtime
**Cons:** Requires cleanup with `RingBuffer_Destroy()`

## Thread-Safe Usage with Per-Instance Mutexes

### Setup with Callbacks

The ring buffer uses callback functions for critical sections, allowing each ring buffer to have its own independent mutex. Register the callbacks once during initialization:

```c
// Register the critical section callbacks (ThreadX example)
RingBuffer_RegisterCriticalSectionCallbacks(&threadx_cs_callbacks);

// Create ring buffers - each will automatically get its own mutex
RingBuffer_t data_ring;
RingBuffer_InitDynamic(&data_ring, 256, sizeof(MyData));

RingBuffer_t command_ring;
RingBuffer_InitDynamic(&command_ring, 128, sizeof(MyData));
```

### How It Works

- **One-time registration**: Call `RingBuffer_RegisterCriticalSectionCallbacks()` once at app startup
- **Per-instance mutexes**: Each `RingBuffer_Init()` or `RingBuffer_InitDynamic()` creates a unique mutex for that buffer
- **Independent synchronization**: Each ring buffer protects its own operations without affecting others
- **Automatic cleanup**: `RingBuffer_Destroy()` cleans up the mutex when done

### Available Platform Callbacks

- **ThreadX**: `threadx_cs_callbacks` - Create via `ring_critical_section_threadx.c`
- **FreeRTOS**: `freertos_cs_callbacks` - Create via `ring_critical_section_freertos.c`
- **ESP32**: `esp32_cs_callbacks` - Create via `ring_critical_section_esp32.c`
- **Bare Metal**: Pass `NULL` to `RingBuffer_RegisterCriticalSectionCallbacks()` for non-thread-safe operation

### Usage Example (Operations Are Automatically Protected)

```c
// No need to manually manage mutex - operations are thread-safe
uint32_t data = 42;
RingBuffer_Write(&data_ring, &data);  // Uses data_ring's mutex internally

MyData cmd = {...};
RingBuffer_Write(&command_ring, &cmd);  // Uses command_ring's mutex independently
```

## Basic Operations

### Write Operations

```c
// Write single element (fails if full)
MyData value = {...};
if (!RingBuffer_Write(&ring, &value)) {
    // Buffer is full, handle overflow
}

// Write with overwrite (always succeeds, discards oldest if full)
RingBuffer_PushFront(&ring, &value);

// Write multiple elements with overwrite
MyData values[5] = {...};
uint32_t written = RingBuffer_PushBackOverwriteMultiple(&ring, values, 5);
```

### Read Operations (Destructive)

```c
// Read single element (removes from buffer)
MyData out;
if (RingBuffer_Read(&ring, &out)) {
    // Successfully read oldest element
}

// Read multiple elements
MyData buffer[10];
uint32_t read_count = RingBuffer_ReadMultiple(&ring, buffer, 10);

// Pop from back (remove newest)
bool success = RingBuffer_PopBack(&ring);
```

### Peek Operations (Non-Destructive)

```c
// Peek oldest element without removing
MyData out;
if (RingBuffer_PeekFront(&ring, &out)) {
    // Got oldest element, still in ring
}

// Peek newest element without removing
if (RingBuffer_PeekBack(&ring, &out)) {
    // Got newest element, still in ring
}

// Peek multiple elements from front (oldest first)
MyData peek_array[10];
uint32_t peeked = RingBuffer_PeekFrontMultiple(&ring, peek_array, 10);

// Peek multiple elements from back (newest first)
uint32_t peeked_back = RingBuffer_PeekBackMultiple(&ring, peek_array, 10);
```

## Status Checking

```c
// Check element count
uint32_t available = RingBuffer_Available(&ring);
uint32_t free_space = RingBuffer_Free(&ring);

// Check state
bool is_empty = RingBuffer_IsEmpty(&ring);
bool is_full = RingBuffer_IsFull(&ring);
bool owns_buffer = RingBuffer_OwnsBuffer(&ring);

// Clear buffer
RingBuffer_Clear(&ring);
```

## Ring-to-Ring Operations (High Performance)

```c
RingBuffer_t source, destination;

// Copy all data (preserve source)
uint32_t copied = RingBuffer_DumpToRing(&source, &destination, true);

// Move all data (consume source)
uint32_t moved = RingBuffer_DumpToRing(&source, &destination, false);

// Copy limited number of elements
uint32_t batch = RingBuffer_DumpToRingLimited(&source, &destination, 50, false);
```

## Real-World Example: Multi-Stage Data Pipeline with Independent Mutexes

```c
typedef struct {
    uint32_t timestamp;
    uint16_t sensor_id;
    float value;
} SensorData_t;

// Create processing stages - each has independent thread safety
RingBuffer_t raw_data, filtered_data, processed_data, tx_queue;

void app_init(void) {
  // Register ThreadX critical section callbacks (one-time)
  RingBuffer_RegisterCriticalSectionCallbacks(&threadx_cs_callbacks);
  
  // Initialize all rings - each creates its own mutex automatically
  RingBuffer_InitDynamic(&raw_data, 100, sizeof(SensorData_t));
  RingBuffer_InitDynamic(&filtered_data, 50, sizeof(SensorData_t));
  RingBuffer_InitDynamic(&processed_data, 50, sizeof(SensorData_t));
  RingBuffer_InitDynamic(&tx_queue, 20, sizeof(SensorData_t));
}

void producer_task(void *arg) {
  SensorData_t reading;
  
  while (1) {
    // Read from sensor
    sensor_read(&reading);
    
    // Write to raw_data - automatically protected by its own mutex
    RingBuffer_PushFront(&raw_data, &reading);
    
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void filter_task(void *arg) {
  // Filter data and move to filtered_data
  // Each ring buffer uses its own independent mutex
  while (1) {
    uint32_t copied = RingBuffer_DumpToRing(&raw_data, &filtered_data, true);
    if (copied == 0) {
      vTaskDelay(pdMS_TO_TICKS(5));
    }
  }
}

void process_task(void *arg) {
  while (1) {
    // Process and move data
    uint32_t moved = RingBuffer_DumpToRing(&filtered_data, &processed_data, false);
    
    // Batch transmission
    if (RingBuffer_Available(&processed_data) >= 10) {
      uint32_t tx_batch = RingBuffer_DumpToRingLimited(&processed_data, &tx_queue, 10, false);
      transmit_batch(&tx_queue);
    }
    
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void app_cleanup(void) {
  // Cleanup destroys each buffer's individual mutex
  RingBuffer_Destroy(&raw_data);
  RingBuffer_Destroy(&filtered_data);
  RingBuffer_Destroy(&processed_data);
  RingBuffer_Destroy(&tx_queue);
}
```

## Mutex Management

The ring buffer automatically creates and manages per-instance mutexes:

```c
// Register critical section callbacks at app startup
RingBuffer_RegisterCriticalSectionCallbacks(&threadx_cs_callbacks);

// Create ring buffer - automatically creates a mutex for this instance
RingBuffer_t my_ring;
RingBuffer_InitDynamic(&my_ring, 128, sizeof(uint32_t));
// my_ring.mutex is now initialized and ready

// Use operations normally - mutex is handled internally
RingBuffer_Write(&my_ring, &data);
RingBuffer_Read(&my_ring, &output);

// Cleanup - automatically destroys this ring's mutex
RingBuffer_Destroy(&my_ring);
// my_ring.mutex is cleaned up and set to NULL
```

### Multiple Independent Ring Buffers

Each ring buffer maintains its own mutex, allowing truly independent thread-safe access:

```c
RingBuffer_t uart_rx, uart_tx, ble_rx, ble_tx;

// Register callbacks once
RingBuffer_RegisterCriticalSectionCallbacks(&threadx_cs_callbacks);

// Each creates its own mutex
RingBuffer_InitDynamic(&uart_rx, 256, sizeof(uint8_t));
RingBuffer_InitDynamic(&uart_tx, 256, sizeof(uint8_t));
RingBuffer_InitDynamic(&ble_rx, 512, sizeof(uint8_t));
RingBuffer_InitDynamic(&ble_tx, 512, sizeof(uint8_t));

// Independent synchronization - no contention between buffers
RingBuffer_Write(&uart_rx, &data);  // Uses uart_rx.mutex
RingBuffer_Write(&ble_rx, &data);   // Uses ble_rx.mutex (different lock)
```

## Memory Management

```c
// For dynamically allocated rings
RingBuffer_Destroy(&ring);  // Frees the buffer and clears structure

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

1. **Register callbacks at app startup** - Call `RingBuffer_RegisterCriticalSectionCallbacks()` once during initialization
2. **Each ring buffer is independent** - No mutex sharing between buffers, no contention
3. **Always destroy dynamically allocated rings** - Call `RingBuffer_Destroy()` to clean up mutex and buffer memory
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
RingBuffer_RegisterCriticalSectionCallbacks(&threadx_cs_callbacks);

// FreeRTOS
#include "ring_critical_section_freertos.h"
RingBuffer_RegisterCriticalSectionCallbacks(&freertos_cs_callbacks);

// ESP32 (FreeRTOS with spinlocks)
#include "ring_critical_section_esp32.h"
RingBuffer_RegisterCriticalSectionCallbacks(&esp32_cs_callbacks);

// Bare Metal (no synchronization)
RingBuffer_RegisterCriticalSectionCallbacks(NULL);
```

### How Callbacks Work

Each platform provides callbacks that implement:
- `create()` - Allocate and initialize a new mutex
- `destroy(mutex)` - Clean up and free a mutex
- `enter(mutex)` - Lock the mutex
- `exit(mutex)` - Unlock the mutex

When you call `RingBuffer_Init()` or `RingBuffer_InitDynamic()`, the ring buffer uses `create()` to make a unique mutex for that instance. When you call `RingBuffer_Destroy()`, it uses `destroy()` to clean up.

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
  RingBuffer_RegisterCriticalSectionCallbacks(&threadx_cs_callbacks);
  
  // Create thread-safe ring buffers
  RingBuffer_t sensor_data;
  RingBuffer_InitDynamic(&sensor_data, 256, sizeof(sensor_reading_t));
  
  // Now all operations are thread-safe
}
```

### ESP32 Application

```c
#include "ring.h"
#include "ring_critical_section_esp32.h"

void app_init(void) {
  // Register ESP32 spinlock callbacks
  RingBuffer_RegisterCriticalSectionCallbacks(&esp32_cs_callbacks);
  
  // Create ISR-safe ring buffers
  RingBuffer_t ble_rx_ring;
  RingBuffer_InitDynamic(&ble_rx_ring, 512, sizeof(ble_packet_t));
  
  // Safe in both tasks and ISRs
}

void ble_rx_callback(uint8_t *data, uint16_t len) {
  // Called from BLE ISR
  for (uint16_t i = 0; i < len; i++) {
    RingBuffer_Write(&ble_rx_ring, &data[i]);  // ISR-safe
  }
}
```

### Bare Metal Application

```c
#include "ring.h"

void app_init(void) {
  // No callbacks needed for bare metal
  RingBuffer_RegisterCriticalSectionCallbacks(NULL);
  
  // Ring buffer is not thread-safe, but works fine
  RingBuffer_t data_buffer;
  RingBuffer_InitDynamic(&data_buffer, 128, sizeof(sensor_data_t));
}
```

