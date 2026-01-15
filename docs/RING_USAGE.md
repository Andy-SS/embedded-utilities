# Ring Buffer Usage Guide (v0.02)

## Quick Start

### Basic Initialization

```c
#include "ring.h"
#include "mutex_common.h"

// Static allocation with pre-allocated buffer
uint8_t buffer[256];
ring_t rx_ring;
ring_init(&rx_ring, buffer, 256, sizeof(uint8_t));

// Dynamic allocation (automatically managed)
ring_t tx_ring;
if (!ring_init_dynamic(&tx_ring, 512, sizeof(uint8_t))) {
    // Handle allocation failure
}
```

### Thread-Safe Usage with ThreadX

Register callbacks once at startup:

```c
#include "rtos.h"
#include "ring.h"
#include "mutex_common.h"

void app_init(void) {
    // ThreadX callbacks automatically available from rtos.c
    extern const mutex_callbacks_t mutex_callbacks;
    
    // Register callbacks - enables automatic mutex creation per ring buffer
    ring_register_cs_callbacks(&mutex_callbacks);
    
    // Create ring buffers - each gets its own mutex
    ring_t data_ring1;
    ring_init_dynamic(&data_ring1, 256, sizeof(float32_t));
    
    ring_t data_ring2;
    ring_init_dynamic(&data_ring2, 512, sizeof(uint16_t));
    
    // Each ring buffer is independently synchronized
}
```

## v0.02 Changes - Unified Mutex Interface

### What Changed

Ring buffer now uses the unified `mutex_callbacks_t` interface instead of ring-specific callbacks:

#### Before (v0.01)
```c
typedef struct {
    ring_cs_result_t (*enter)(void *mutex);
    ring_cs_result_t (*exit)(void *mutex);
    void* (*create)(void);
    void (*destroy)(void *mutex);
} ring_cs_callbacks_t;

ring_register_cs_callbacks(&threadx_cs_callbacks);
```

#### After (v0.02)
```c
typedef struct {
    void* (*create)(void);
    void (*destroy)(void *mutex);
    mutex_result_t (*acquire)(void *mutex, uint32_t timeout_ms);
    mutex_result_t (*release)(void *mutex);
} mutex_callbacks_t;

ring_register_cs_callbacks(&mutex_callbacks);  // Same as eLog!
```

### Benefits of v0.02

1. **Unified Interface** - Same `mutex_callbacks_t` used by both eLog and Ring
2. **Timeout Support** - `acquire()` now supports millisecond timeouts
3. **Shared Implementation** - Use same ThreadX callbacks for both modules
4. **ThreadX Memory Integration** - Callbacks use `byte_allocate()/byte_release()`
5. **Cleaner API** - Consistent error handling via `mutex_result_t` enum

## Migration Guide (v0.01 → v0.02)

### Step 1: Update Callback Registration

**Old Code (v0.01):**
```c
ring_register_cs_callbacks(&threadx_cs_callbacks);
```

**New Code (v0.02):**
```c
extern const mutex_callbacks_t mutex_callbacks;
ring_register_cs_callbacks(&mutex_callbacks);
```

### Step 2: Callback Implementation Changes

**Old ThreadX Callbacks (v0.01):**
```c
ring_cs_result_t threadx_cs_enter(void *mutex) {
    TX_MUTEX *tx_mutex = (TX_MUTEX *)mutex;
    UINT status = tx_mutex_get(tx_mutex, TX_WAIT_FOREVER);
    return (status == TX_SUCCESS) ? RING_CS_OK : RING_CS_ERROR;
}

ring_cs_result_t threadx_cs_exit(void *mutex) {
    TX_MUTEX *tx_mutex = (TX_MUTEX *)mutex;
    UINT status = tx_mutex_put(tx_mutex);
    return (status == TX_SUCCESS) ? RING_CS_OK : RING_CS_ERROR;
}

void* threadx_cs_create(void) {
    TX_MUTEX *mutex = (TX_MUTEX *)malloc(sizeof(TX_MUTEX));
    tx_mutex_create(mutex, "Ring_Mutex", TX_NO_INHERIT);
    return (void *)mutex;
}

void threadx_cs_destroy(void *mutex) {
    if (mutex == NULL) return;
    TX_MUTEX *tx_mutex = (TX_MUTEX *)mutex;
    tx_mutex_delete(tx_mutex);
    free(tx_mutex);
}

const ring_cs_callbacks_t threadx_cs_callbacks = {
    .enter = threadx_cs_enter,
    .exit = threadx_cs_exit,
    .create = threadx_cs_create,
    .destroy = threadx_cs_destroy
};
```

**New Unified Callbacks (v0.02):**
```c
void* threadx_mutex_create(void) {
    TX_MUTEX *mutex = (TX_MUTEX *)byte_allocate(sizeof(TX_MUTEX));
    if (mutex == NULL) return NULL;
    
    UINT status = tx_mutex_create(mutex, "Mutex", TX_NO_INHERIT);
    return (status == TX_SUCCESS) ? (void *)mutex : NULL;
}

mutex_result_t threadx_mutex_destroy(void *mutex) {
    if (mutex == NULL) return MUTEX_ERROR;
    TX_MUTEX *tx_mutex = (TX_MUTEX *)mutex;
    UINT status = tx_mutex_delete(tx_mutex);
    byte_release(tx_mutex);
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

const mutex_callbacks_t mutex_callbacks = {
    .create = threadx_mutex_create,
    .destroy = threadx_mutex_destroy,
    .acquire = threadx_mutex_acquire,
    .release = threadx_mutex_release
};
```

**Note:** The new implementation is provided in `App/rtos.c`. You only need to register it, not reimplement it!

## Memory Allocation Integration

### ThreadX Byte Pool Functions

v0.02 integrates with ThreadX byte pool allocation:

```c
// From rtos.h - declared in rtos.c
void* byte_allocate(size_t size);   // Allocate from ThreadX byte pool
void byte_release(void *ptr);       // Release back to ThreadX byte pool
```

### Usage in Ring Buffer Operations

All internal ring buffer operations now use ThreadX memory:

```c
// Example: Internal dynamic buffer allocation within ring operations
ring_t ring;
if (ring_init_dynamic(&ring, 1024, sizeof(MyData))) {
    // Internal buffer allocated via byte_allocate()
    // Automatically managed by ring buffer
    
    MyData data = {...};
    ring_write(&ring, &data);  // Thread-safe write
    
    // Cleanup
    ring_destroy(&ring);  // Internal buffer freed via byte_release()
}
```

### Benefits for Your Application

1. **Unified Memory Management** - All allocations through ThreadX byte pool
2. **Resource Tracking** - ThreadX monitors all memory allocations
3. **Deadlock Prevention** - Byte pool has built-in timeout mechanisms
4. **Error Handling** - Clear error reporting for allocation failures

## Complete ThreadX Integration Example

Located in `App/rtos.c`:

```c
#include "tx_api.h"
#include "rtos.h"
#include "ring.h"
#include "mutex_common.h"

// Global ring buffers
ring_t sensor_data_ring;
ring_t command_queue_ring;

void tx_application_define(VOID *first_unused_memory) {
    // Register unified mutex callbacks (same for eLog and Ring)
    #if RING_USE_RTOS_MUTEX
    ring_register_cs_callbacks(&mutex_callbacks);
    #endif
    
    // Create ring buffers
    ring_init_dynamic(&sensor_data_ring, 512, sizeof(SensorData));
    ring_init_dynamic(&command_queue_ring, 256, sizeof(Command));
    
    // ... rest of ThreadX initialization ...
}

// Producer task
void sensor_task(ULONG input) {
    SensorData data;
    
    while(1) {
        // Collect sensor data
        data = read_sensor();
        
        // Write to ring buffer (automatically thread-safe)
        if (!ring_write(&sensor_data_ring, &data)) {
            // Handle buffer full condition
            printERR("Sensor data ring buffer full!");
        }
        
        tx_thread_sleep(10);
    }
}

// Consumer task
void process_task(ULONG input) {
    SensorData data;
    
    while(1) {
        // Read from ring buffer (automatically thread-safe)
        if (ring_read(&sensor_data_ring, &data)) {
            process_sensor_data(&data);
        }
        
        tx_thread_sleep(20);
    }
}

// Command queue task
void command_handler(ULONG input) {
    Command cmd;
    
    while(1) {
        // Read command with timeout support
        if (ring_read(&command_queue_ring, &cmd)) {
            execute_command(&cmd);
        }
        
        tx_thread_sleep(5);
    }
}
```

## Performance Considerations

### Per-Instance Mutexes

Each ring buffer maintains its own mutex:

```c
ring_t ring1, ring2, ring3;

ring_init_dynamic(&ring1, 256, sizeof(Data));
ring_init_dynamic(&ring2, 512, sizeof(Data));
ring_init_dynamic(&ring3, 128, sizeof(Data));

// Each gets independent mutex, no global lock contention
// Multiple threads can safely use different rings simultaneously
```

### Memory Overhead

Per ring buffer:
- Mutex object: `sizeof(TX_MUTEX)` (platform-dependent, typically 40-60 bytes)
- Ring structure: ~40 bytes
- User buffer: `size × element_size`

### Timeout Support

The new `acquire()` function supports timeouts:

```c
mutex_result_t result = callbacks.acquire(mutex, 100);  // 100ms timeout
if (result == MUTEX_OK) {
    // Got lock
} else if (result == MUTEX_TIMEOUT) {
    // Timeout occurred
} else {
    // Error
}
```

## Error Handling

### Allocation Failures

```c
ring_t ring;
if (!ring_init_dynamic(&ring, 1024, sizeof(Data))) {
    printERR("Failed to allocate ring buffer");
    // Handle error - probably out of memory
}
```

### Write Failures

```c
Data value = {...};
if (!ring_write(&ring, &value)) {
    printWARN("Ring buffer full, data dropped");
    // Handle overflow condition
}
```

### Read from Empty Buffer

```c
Data value;
if (!ring_read(&ring, &value)) {
    printDEBUG("Ring buffer empty, no data to read");
    // Buffer is empty, handle appropriately
}
```

## DSP Processing Example

From `App/Tasks/dspProcess.c` - uses ThreadX memory integration:

```c
// Median filter using ThreadX byte_allocate()
void medianFilterCentered(const float32_t *input, float32_t *output, int length,
                          const int window_size) {
    // Allocate temporary window from ThreadX byte pool
    float32_t *window = (float32_t *)byte_allocate(window_size * sizeof(float32_t));
    if (!window) { 
        return;  // Allocation failed
    }
    
    // Process data...
    
    // Release memory back to pool
    byte_release(window);
}

// Polynomial fit using ThreadX byte_allocate()
bool polyfit(const float32_t *t, const float32_t *y, uint32_t len, int degree, float32_t *coeffs) {
    int n_coeffs = degree + 1;
    
    // All intermediate matrices allocated via byte_allocate()
    float32_t *A_data = (float32_t *)byte_allocate(len * n_coeffs * sizeof(float32_t));
    if (!A_data) { return false; }
    
    float32_t *AT_data = (float32_t *)byte_allocate(n_coeffs * len * sizeof(float32_t));
    if (!AT_data) { 
        byte_release(A_data);
        return false; 
    }
    
    // ... matrix operations ...
    
    // Cleanup
    byte_release(A_data);
    byte_release(AT_data);
    // ... other releases ...
    
    return true;
}
```

## Troubleshooting

### Ring buffer operations failing silently
- Check that `ring_register_cs_callbacks()` was called
- Verify ThreadX scheduler has started before using ring buffers
- Check for allocation failures (enable logging)

### Mutex timeout errors
- Verify timeout value is appropriate for your system
- Check for mutex contention - may indicate design issue
- Enable ThreadX debugging to inspect lock hold times

### Memory allocation failures
- Check ThreadX byte pool size configuration
- Monitor total allocations (use `debug_memory_statistics()`)
- Verify no memory leaks - all `byte_allocate()` should have corresponding `byte_release()`

## Best Practices

1. **Register callbacks early** - Call `ring_register_cs_callbacks()` before creating any ring buffers
2. **Use dynamic allocation when size varies** - `ring_init_dynamic()` for flexibility
3. **Check return values** - All ring operations can fail
4. **Handle buffer full** - Plan for overflow conditions
5. **Cleanup on exit** - Call `ring_destroy()` to free resources
6. **Single registration** - One `ring_register_cs_callbacks()` call serves all ring buffers
7. **Share callbacks with eLog** - Use same `mutex_callbacks_t` for both modules

## Related Documentation

- [Ring Buffer API](docs/RING.md) - Complete API reference
- [Unified Mutex Guide](UNIFIED_MUTEX_GUIDE.md) - Detailed callback interface
- [eLog Documentation](docs/ELOG.md) - Shared mutex callback system
