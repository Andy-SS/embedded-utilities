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
- **Thread-Safe Operations** - Optional RTOS mutex support for multi-threaded access
- **Efficient Operations** - O(1) read/write/peek performance
- **Multiple Data Types** - Works with any data type via element_size
- **High-Performance Transfers** - Ring-to-ring copying for batch processing

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

## Thread-Safe Usage with RTOS Mutex

### Setup

```c
// Create ring buffer
RingBuffer_t ring;
RingBuffer_InitDynamic(&ring, 128, sizeof(MyData));

// Create ThreadX mutex
TX_MUTEX ring_mutex;
tx_mutex_create(&ring_mutex, "ring_mutex", TX_INHERIT);

// Attach mutex to ring buffer
RingBuffer_AttachMutex(&ring, &ring_mutex);
```

### Use Ring Buffer Operations Normally

Once the mutex is attached, you can use standard operations. The critical sections are handled in your code's higher-level synchronization logic:

```c
// Write data
MyData data = {...};
RingBuffer_Write(&ring, &data);

// Read data
MyData out;
RingBuffer_Read(&ring, &out);

// Peek data (non-destructive)
RingBuffer_PeekFront(&ring, &out);

// Check status
uint32_t available = RingBuffer_Available(&ring);
```

**Note:** The mutex pointer is available via `ring.mutex` for use in your own critical section macros when needed.

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

## Real-World Example: Multi-Stage Data Pipeline

```c
typedef struct {
    uint32_t timestamp;
    uint16_t sensor_id;
    float value;
} SensorData_t;

// Create processing stages
RingBuffer_t raw_data, filtered_data, processed_data, tx_queue;
TX_MUTEX data_mutex;

// Initialize all rings
RingBuffer_InitDynamic(&raw_data, 100, sizeof(SensorData_t));
RingBuffer_InitDynamic(&filtered_data, 50, sizeof(SensorData_t));
RingBuffer_InitDynamic(&processed_data, 50, sizeof(SensorData_t));
RingBuffer_InitDynamic(&tx_queue, 20, sizeof(SensorData_t));

// Attach mutex for thread safety
tx_mutex_create(&data_mutex, "data_mutex", TX_INHERIT);
RingBuffer_AttachMutex(&raw_data, &data_mutex);
RingBuffer_AttachMutex(&filtered_data, &data_mutex);
RingBuffer_AttachMutex(&processed_data, &data_mutex);

// Stage 1: Collect raw sensor data
RingBuffer_PushFront(&raw_data, &sensor_reading);

// Stage 2: Filter data (quality check)
uint32_t filtered_count = RingBuffer_DumpToRing(&raw_data, &filtered_data, true);

// Stage 3: Process filtered data
uint32_t processed = RingBuffer_DumpToRing(&filtered_data, &processed_data, false);

// Stage 4: Batch transmission
while (RingBuffer_Available(&processed_data) >= 10) {
    uint32_t batch = RingBuffer_DumpToRingLimited(&processed_data, &tx_queue, 10, false);
    // Transmit batch...
    RingBuffer_Clear(&tx_queue);
}

// Cleanup
RingBuffer_Destroy(&raw_data);
RingBuffer_Destroy(&filtered_data);
RingBuffer_Destroy(&processed_data);
RingBuffer_Destroy(&tx_queue);
```

## Mutex Management

```c
// Attach mutex to enable thread-safe critical sections
RingBuffer_AttachMutex(&ring, &my_mutex);

// Detach mutex when no longer needed
RingBuffer_DetachMutex(&ring);
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

1. **Always use mutex for multi-threaded access** - Protect all operations with critical sections
2. **Check return values** - Write/Read may fail if buffer is full/empty
3. **Use DumpToRing for batch operations** - More efficient than multiple individual operations
4. **Call Destroy for dynamic buffers** - Prevents memory leaks
5. **Size buffers appropriately** - Too small causes overflows, too large wastes memory
6. **Detach mutex before destroying** - Clean up in reverse order of creation

## Limitations

- Fixed size (cannot grow dynamically after initialization)
- Element size fixed at initialization
- No built-in error recovery (overwritten data is lost)

## See Also

- [README.md](../README.md) - Main project documentation
- [eLog Documentation](./ELOG.md) - Logging system
- [Bit Utilities Documentation](./BIT.md) - Bit manipulation