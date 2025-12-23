/***********************************************************
 * @file	ring.c
 * @author	Andy Chen (andy.chen@respiree.com)
 * @version	0.01
 * @date	2025-04-09
 * @brief
 * **********************************************************
 * @copyright Copyright (c) 2025 Respiree. All rights reserved.
 *
 ************************************************************/
#include "tx_api.h"
#include "ring.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


// ESP-IDF Critical Section Implementation
// #ifndef ESP_IDF_VERSION
// #define ESP_IDF_VERSION
// #endif

#ifdef ESP_IDF_VERSION
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// For ESP32 dual-core systems, use portMUX_TYPE for thread-safe operations

// ISR-safe critical sections for BLE data processing
#define ENTER_CRITICAL_SECTION(mutex) do { \
    if (xPortInIsrContext()) { \
        portENTER_CRITICAL_ISR(mutex); \
    } else { \
        portENTER_CRITICAL(mutex); \
    } \
} while(0)

#define EXIT_CRITICAL_SECTION(mutex) do { \
    if (xPortInIsrContext()) { \
        portEXIT_CRITICAL_ISR(mutex); \
    } else { \
        portEXIT_CRITICAL(mutex); \
    } \
} while(0)

// Debug version with timeout detection
#ifdef CONFIG_RING_BUFFER_DEBUG
#define RING_BUFFER_CRITICAL_TIMEOUT_MS 1000
extern uint32_t ring_buffer_critical_start_time;

#define ENTER_CRITICAL_SECTION_DEBUG() do { \
    ring_buffer_critical_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS; \
    ENTER_CRITICAL_SECTION(); \
} while(0)

#define EXIT_CRITICAL_SECTION_DEBUG() do { \
    EXIT_CRITICAL_SECTION(); \
    uint32_t duration = (xTaskGetTickCount() * portTICK_PERIOD_MS) - ring_buffer_critical_start_time; \
    if (duration > RING_BUFFER_CRITICAL_TIMEOUT_MS) { \
        ESP_LOGW("RingBuffer", "Critical section held for %d ms", duration); \
    } \
} while(0)
#endif
#elif defined(TX_API_H) // ThreadX Critical Section Implementation
#include "stm32wbxx.h"
// Simple critical section using CMSIS for STM32
// This disables interrupts and saves the interrupt state for restoration
#if defined(ENTER_CRITICAL_SECTION)
#undef ENTER_CRITICAL_SECTION
#endif
#if defined(EXIT_CRITICAL_SECTION)
#undef EXIT_CRITICAL_SECTION
#endif
// ThreadX Critical Section Implementation
#include "tx_api.h"

// ThreadX mutex-based critical sections with proper error handling
#if defined(ENTER_CRITICAL_SECTION)
#undef ENTER_CRITICAL_SECTION
#endif
#if defined(EXIT_CRITICAL_SECTION)
#undef EXIT_CRITICAL_SECTION
#endif

/**
 * @brief Acquire mutex with timeout
 * @param mutex Pointer to TX_MUTEX
 * @return true if acquired successfully, false on timeout
 */
static inline bool _ring_mutex_lock(void *mutex) {
    if (mutex == NULL) {
        return false;
    }
    UINT status = tx_mutex_get((TX_MUTEX *)mutex, TX_WAIT_FOREVER);
    return (status == TX_SUCCESS);
}

/**
 * @brief Release mutex
 * @param mutex Pointer to TX_MUTEX
 * @return true if released successfully
 */
static inline bool _ring_mutex_unlock(void *mutex) {
    if (mutex == NULL) {
        return false;
    }
    UINT status = tx_mutex_put((TX_MUTEX *)mutex);
    return (status == TX_SUCCESS);
}

#define ENTER_CRITICAL_SECTION(mutex) _ring_mutex_lock(mutex)
#define EXIT_CRITICAL_SECTION(mutex) _ring_mutex_unlock(mutex)

#else
// For other platforms or testing, define empty macros
#warning "Ring buffer critical sections disabled - not thread-safe!"
#define ENTER_CRITICAL_SECTION() 
#define EXIT_CRITICAL_SECTION()
#endif

// ESP-IDF specific includes
#ifdef ESP_IDF_VERSION
#include "esp_log.h"
static const char *TAG = "RingBuffer";

// Define the mutex for critical sections
portMUX_TYPE ring_buffer_mux = portMUX_INITIALIZER_UNLOCKED;

#ifdef CONFIG_RING_BUFFER_DEBUG
uint32_t ring_buffer_critical_start_time = 0;
#endif

#elif defined(TX_API_H)
// ThreadX specific includes
#include "tx_api.h"

#endif

// Initialize the ring buffer
void RingBuffer_Init(RingBuffer_t *rb, void *buffer, uint32_t size, size_t element_size) {
  rb->buffer = buffer;
  rb->head = 0;
  rb->tail = 0;
  rb->size = size;
  rb->count = 0;          // Initialize count to 0 (empty)
  rb->element_size = element_size;
  rb->owns_buffer = false;  // External buffer, not owned by ring buffer
  #if USE_RTOS_MUTEX
  rb->mutex = NULL;
  #endif
}

// Initialize the ring buffer with dynamic allocation
bool RingBuffer_InitDynamic(RingBuffer_t *rb, uint32_t size, size_t element_size) {
  if (rb == NULL || size == 0 || element_size == 0) {
    return false;
  }
  
  // Calculate total buffer size needed
  size_t total_size = size * element_size;
  
  // Allocate memory for the buffer
  void *buffer = malloc(total_size);
  if (buffer == NULL) {
#ifdef ESP_IDF_VERSION
    ESP_LOGE(TAG, "Failed to allocate %zu bytes for ring buffer", total_size);
#endif
    return false;
  }
  
  // Initialize the ring buffer structure
  rb->buffer = buffer;
  rb->head = 0;
  rb->tail = 0;
  rb->size = size;
  rb->count = 0;          // Initialize count to 0 (empty)
  rb->element_size = element_size;
  rb->owns_buffer = true;  // We own this buffer and will free it

  #if USE_RTOS_MUTEX
  rb->mutex = NULL;
  #endif
  
#ifdef ESP_IDF_VERSION
  ESP_LOGI(TAG, "Dynamically allocated ring buffer: %u elements × %zu bytes = %zu bytes total", 
           size, element_size, total_size);
#endif
  
  return true;
}

#if USE_RTOS_MUTEX
/**
 * @brief Attaches an external RTOS mutex to the ring buffer
 */
bool RingBuffer_AttachMutex(RingBuffer_t *rb, void *mutex) {
  if (rb == NULL) {
    return false;
  }
  rb->mutex = mutex;
  return true;
}

/**
 * @brief Detaches the RTOS mutex from the ring buffer
 */
bool RingBuffer_DetachMutex(RingBuffer_t *rb) {
  if (rb == NULL) {
    return false;
  }
  if (rb->mutex == NULL) {
    return false;
  }
  rb->mutex = NULL;
  return true;
}
#endif /* USE_RTOS_MUTEX */

// Destroy ring buffer and free dynamically allocated memory
void RingBuffer_Destroy(RingBuffer_t *rb) {
  if (rb == NULL) {
    return;
  }
  
  // Only free the buffer if we own it (dynamically allocated)
  if (rb->owns_buffer && rb->buffer != NULL) {
#ifdef ESP_IDF_VERSION
    ESP_LOGI(TAG, "Freeing dynamically allocated ring buffer (%zu bytes)", 
             rb->size * rb->element_size);
#endif
    free(rb->buffer);
  }
  
  // Clear the structure
  rb->buffer = NULL;
  rb->head = 0;
  rb->tail = 0;
  rb->size = 0;
  rb->count = 0;
  rb->element_size = 0;
  rb->owns_buffer = false;
  #if USE_RTOS_MUTEX
  rb->mutex = NULL;
  #endif
}

// Check if ring buffer owns its buffer
bool RingBuffer_OwnsBuffer(const RingBuffer_t *rb) {
  if (rb == NULL) {
    return false;
  }
  return rb->owns_buffer;
}

void RingBuffer_Clear(RingBuffer_t *rb) {
  ENTER_CRITICAL_SECTION();
  rb->head = 0;
  rb->tail = 0;
  rb->count = 0;
  EXIT_CRITICAL_SECTION();
}

// Check if the ring buffer is empty
bool RingBuffer_IsEmpty(const RingBuffer_t *rb) { return rb->count == 0; }

// Check if the ring buffer is full (now uses all available slots!)
bool RingBuffer_IsFull(const RingBuffer_t *rb) { return rb->count == rb->size; }

// Write an element to the ring buffer
bool RingBuffer_Write(RingBuffer_t *rb, const void *data) {
  if (RingBuffer_IsFull(rb)) {
    return false; // Buffer full, write fails
  }
  // Calculate the offset and copy the data
  void *dest = (uint8_t *)rb->buffer + (rb->head * rb->element_size);
  memcpy(dest, data, rb->element_size);
  #if USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  rb->head = (rb->head + 1) % rb->size; // Increment head with wrap-around
  rb->count++;                          // Increment count
  #if USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  return true;
}

// Write an element to the ring buffer (overwrites oldest data if full)
bool RingBuffer_PushFront(RingBuffer_t *rb, const void *data) {
  if (rb == NULL) {
    return false; // Not a valid ring buffer, write fails
  }
  
  bool was_full = RingBuffer_IsFull(rb);
  
  // Ring: overwrite oldest data when full
  void *dest = (uint8_t *)rb->buffer + (rb->head * rb->element_size);
  memcpy(dest, data, rb->element_size);
  
  #if USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  rb->head = (rb->head + 1) % rb->size;
  
  if (was_full) {
    // Move tail forward to discard oldest data
    rb->tail = (rb->tail + 1) % rb->size;
    // Count stays the same (still full)
  } else {
    // Buffer wasn't full, increment count
    rb->count++;
  }
  #if USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  return true;
}

// Write multiple elements to the ring buffer (overwrites oldest data if full)
uint32_t RingBuffer_PushBackOverwriteMultiple(RingBuffer_t *rb, const void *data, uint32_t count) {
  if (rb == NULL || data == NULL || count == 0) { return 0; }

  uint32_t head = rb->head;
  uint32_t tail = rb->tail;
  uint32_t current_count = rb->count;
  uint32_t elements_to_write = count;
  uint32_t overwritten = 0;

  for (uint32_t i = 0; i < elements_to_write; ++i) {
    void *dest = (uint8_t *)rb->buffer + (head * rb->element_size);
    memcpy(dest, (const uint8_t *)data + (i * rb->element_size), rb->element_size);
    head = (head + 1) % rb->size;
    
    if (current_count == rb->size) {
      // Buffer was full, move tail forward to discard oldest data
      tail = (tail + 1) % rb->size;
      overwritten++;
    } else {
      // Buffer wasn't full, increment count
      current_count++;
    }
  }
  
  #if USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  rb->head = head;
  rb->tail = tail;
  rb->count = current_count;
  #if USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif

  return elements_to_write;
}

// Read an element from the ring buffer
bool RingBuffer_Read(RingBuffer_t *rb, void *data) {
  if (rb == NULL || RingBuffer_IsEmpty(rb)) {
    return false; // Buffer empty and ring type is static, read fails
  }
  // Calculate the offset and copy the data
  void *src = (uint8_t *)rb->buffer + (rb->tail * rb->element_size);
  memcpy(data, src, rb->element_size);
  #if USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  rb->tail = (rb->tail + 1) % rb->size; // Increment tail with wrap-around
  rb->count--;                          // Decrement count
  #if USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  return true;
}

// Get the number of elements available in the buffer
uint32_t RingBuffer_Available(const RingBuffer_t *rb) { 
  if (rb == NULL) { return 0; }
  
  // Safety check for corrupted ring buffer
  if (rb->count > rb->size) {
#ifdef ESP_IDF_VERSION
    ESP_LOGE(TAG, "CORRUPTION DETECTED: count (%u) > size (%u)", rb->count, rb->size);
#endif
    return 0;
  }
  
  return rb->count; 
}

// Get the number of free slots in the buffer (now uses full capacity!)
uint32_t RingBuffer_Free(const RingBuffer_t *rb) {
  if (rb == NULL) { return 0; }
  
  // Safety check for corrupted ring buffer
  if (rb->count > rb->size) {
#ifdef ESP_IDF_VERSION
    ESP_LOGE(TAG, "CORRUPTION DETECTED: count (%u) > size (%u)", rb->count, rb->size);
#endif
    return 0;
  }
  
  return rb->size - rb->count;
}

// Write multiple elements to the ring buffer (DMA-friendly)
uint32_t RingBuffer_WriteMultiple(RingBuffer_t *rb, const void *data, uint32_t count) {
  if (rb == NULL || data == NULL || count == 0) { return 0; }

  uint32_t free_slots = RingBuffer_Free(rb);
  uint32_t elements_to_write = (count > free_slots) ? free_slots : count;

  if (elements_to_write == 0) { return 0; }

  uint32_t head = rb->head;
  uint32_t first_chunk = rb->size - head;
  if (first_chunk > elements_to_write) { first_chunk = elements_to_write; }
  uint32_t second_chunk = elements_to_write - first_chunk;

  // Copy first chunk (up to end of buffer or elements_to_write)
  void *dest = rb->buffer + (head * rb->element_size);
  memcpy(dest, data, first_chunk * rb->element_size);

  // Copy second chunk (if wrap-around occurs)
  if (second_chunk > 0) {
    memcpy(rb->buffer, (uint8_t *)data + (first_chunk * rb->element_size), second_chunk * rb->element_size);
  }

  // Atomic update of head and count
  #if USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  rb->head = (head + elements_to_write) % rb->size;
  rb->count += elements_to_write;
  #if USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif

  return elements_to_write;
}

// Read multiple elements from the ring buffer (DMA-friendly)
uint32_t RingBuffer_ReadMultiple(RingBuffer_t *rb, void *data, uint32_t count) {
  if (rb == NULL || data == NULL || count == 0) { return 0; }

  uint32_t available = RingBuffer_Available(rb);
  uint32_t elements_to_read = (count > available) ? available : count;

  if (elements_to_read == 0) { return 0; }

  uint32_t tail = rb->tail;
  uint32_t first_chunk = rb->size - tail;
  if (first_chunk > elements_to_read) { first_chunk = elements_to_read; }
  uint32_t second_chunk = elements_to_read - first_chunk;

  // Copy first chunk (up to end of buffer or elements_to_read)
  void *src = rb->buffer + (tail * rb->element_size);
  memcpy(data, src, first_chunk * rb->element_size);

  // Copy second chunk (if wrap-around occurs)
  if (second_chunk > 0) {
    memcpy((uint8_t *)data + (first_chunk * rb->element_size), rb->buffer, second_chunk * rb->element_size);
  }

  // Atomic update of tail and count
  #if USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  rb->tail = (tail + elements_to_read) % rb->size;
  rb->count -= elements_to_read;
  #if USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif

  return elements_to_read;
}

// Remove a single element from the back of the ring buffer
bool RingBuffer_PopBack(RingBuffer_t *rb) {
  if (rb == NULL || RingBuffer_IsEmpty(rb)) {
    return false; // Buffer empty or invalid, pop fails
  }

  // Atomic decrement of head and count
  #if USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  rb->head = (rb->head == 0) ? (rb->size - 1) : (rb->head - 1);
  rb->count--;
  #if USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  return true;
}

// Remove multiple elements from the back of the ring buffer
uint32_t RingBuffer_PopBackMultiple(RingBuffer_t *rb, uint32_t count) {
  if (rb == NULL || count == 0 || RingBuffer_IsEmpty(rb)) {
    return 0; // Invalid parameters or empty buffer
  }

  // Limit count to available elements
  uint32_t available = RingBuffer_Available(rb);
  uint32_t elements_to_remove = (count > available) ? available : count;

  // Atomic decrement of head and count
  #if USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  if (rb->head >= elements_to_remove) {
    rb->head -= elements_to_remove;
  } else {
    rb->head = rb->size - (elements_to_remove - rb->head);
  }
  rb->count -= elements_to_remove;
  #if USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  return elements_to_remove;
}

// Remove a single element from the front of the ring buffer (oldest element)
bool RingBuffer_PopFront(RingBuffer_t *rb) {
  if (rb == NULL || RingBuffer_IsEmpty(rb)) {
    return false; // Buffer empty or invalid, pop fails
  }

  // Atomic increment of tail and decrement count
  #if USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  rb->tail = (rb->tail + 1) % rb->size;
  rb->count--;
  #if USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  return true;
}

// Remove multiple elements from the front of the ring buffer (oldest elements)
uint32_t RingBuffer_PopFrontMultiple(RingBuffer_t *rb, uint32_t count) {
  if (rb == NULL || count == 0 || RingBuffer_IsEmpty(rb)) {
    return 0; // Invalid parameters or empty buffer
  }

  // Limit count to available elements
  uint32_t available = RingBuffer_Available(rb);
  uint32_t elements_to_remove = (count > available) ? available : count;

  // Atomic increment of tail and decrement count
  #if USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  rb->tail = (rb->tail + elements_to_remove) % rb->size;
  rb->count -= elements_to_remove;
  #if USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  return elements_to_remove;
}

// Peek the oldest element from the ring buffer (does not move tail)
bool RingBuffer_PeekFront(const RingBuffer_t *rb, void *data) {
  if (rb == NULL || RingBuffer_IsEmpty(rb)) { return false; }
  uint32_t pos = rb->tail;
  void *src = (uint8_t *)rb->buffer + (pos * rb->element_size);
  memcpy(data, src, rb->element_size);
  return true;
}

// Peek the newest element from the ring buffer (does not move head)
bool RingBuffer_PeekBack(const RingBuffer_t *rb, void *data) {
  if (rb == NULL || RingBuffer_IsEmpty(rb)) { return false; }
  uint32_t pos = (rb->head == 0) ? (rb->size - 1) : (rb->head - 1);
  void *src = (uint8_t *)rb->buffer + (pos * rb->element_size);
  memcpy(data, src, rb->element_size);
  return true;
}

// Peek multiple elements from the back of the ring buffer (does not move head)
// The newest element is at index 0, next newest at index 1, etc.
uint32_t RingBuffer_PeekBackMultiple(const RingBuffer_t *rb, void *data, uint32_t count) {
  if (rb == NULL || data == NULL || RingBuffer_IsEmpty(rb) || count == 0) { return 0; }
  uint32_t available = RingBuffer_Available(rb);
  if (count > available) { count = available; }

  for (uint32_t i = 0; i < count; ++i) {
    // Calculate position from back: (head - 1 - i + size) % size
    uint32_t pos = (rb->head == 0 ? rb->size : rb->head) - 1;
    pos = (pos + rb->size - i) % rb->size;
    void *src = (uint8_t *)rb->buffer + (pos * rb->element_size);
    memcpy((uint8_t *)data + (i * rb->element_size), src, rb->element_size);
  }
  return count;
}

// Peek multiple elements from the front of the ring buffer (does not move tail)
// The oldest element is at index 0, next oldest at index 1, etc.
uint32_t RingBuffer_PeekFrontMultiple(const RingBuffer_t *rb, void *data, uint32_t count) {
  if (rb == NULL || data == NULL || RingBuffer_IsEmpty(rb) || count == 0) { return 0; }
  uint32_t available = RingBuffer_Available(rb);
  if (count > available) { count = available; }

  for (uint32_t i = 0; i < count; ++i) {
    uint32_t pos = (rb->tail + i) % rb->size;
    void *src = (uint8_t *)rb->buffer + (pos * rb->element_size);
    memcpy((uint8_t *)data + (i * rb->element_size), src, rb->element_size);
  }
  return count;
}

// Dump all elements from source ring buffer to destination ring buffer (direct buffer copy)
uint32_t RingBuffer_DumpToRing(RingBuffer_t *src_rb, RingBuffer_t *dst_rb, bool preserve_source) {
  if (src_rb == NULL || dst_rb == NULL) {
    return 0;
  }
  
  // Check element size compatibility
  if (src_rb->element_size != dst_rb->element_size) {
#ifdef ESP_IDF_VERSION
    ESP_LOGW(TAG, "Ring dump failed: incompatible element sizes (src=%zu, dst=%zu)", 
             src_rb->element_size, dst_rb->element_size);
#endif
    return 0;
  }
  
  // Get available elements in source and free space in destination
  uint32_t src_available = RingBuffer_Available(src_rb);
  uint32_t dst_free = RingBuffer_Free(dst_rb);
  
  if (src_available == 0) {
    return 0;  // Nothing to copy
  }
  
  // Determine how many elements we can actually copy
  uint32_t elements_to_copy = (src_available < dst_free) ? src_available : dst_free;
  
  if (elements_to_copy == 0) {
    return 0;  // Destination is full
  }
  
  uint32_t copied_count = 0;
  uint32_t src_tail = src_rb->tail;
  uint32_t dst_head = dst_rb->head;
  
  // Direct buffer-to-buffer copying in chunks to handle wrap-around
  while (copied_count < elements_to_copy) {
    // Calculate how many elements we can copy in this chunk (until wrap-around)
    uint32_t src_chunk = src_rb->size - src_tail;
    uint32_t dst_chunk = dst_rb->size - dst_head;
    uint32_t remaining = elements_to_copy - copied_count;
    
    // Take the minimum of: remaining elements, source chunk, destination chunk
    uint32_t chunk_size = remaining;
    if (chunk_size > src_chunk) chunk_size = src_chunk;
    if (chunk_size > dst_chunk) chunk_size = dst_chunk;
    
    // Direct memory copy from source buffer to destination buffer
    void *src_ptr = (uint8_t *)src_rb->buffer + (src_tail * src_rb->element_size);
    void *dst_ptr = (uint8_t *)dst_rb->buffer + (dst_head * dst_rb->element_size);
    size_t copy_bytes = chunk_size * src_rb->element_size;
    
    memcpy(dst_ptr, src_ptr, copy_bytes);
    
    // Update positions
    copied_count += chunk_size;
    src_tail = (src_tail + chunk_size) % src_rb->size;
    dst_head = (dst_head + chunk_size) % dst_rb->size;
  }
  
  // Update ring buffer states atomically
  #if USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  
  if (!preserve_source) {
    // Consume source data by updating tail and count
    src_rb->tail = src_tail;
    if (src_rb->count >= copied_count) {
      src_rb->count -= copied_count;
    } else {
      src_rb->count = 0;  // Safety check to prevent underflow
    }
  }
  
  // Always update destination head and count with overflow protection
  dst_rb->head = dst_head;
  uint32_t new_count = dst_rb->count + copied_count;
  if (new_count <= dst_rb->size) {
    dst_rb->count = new_count;
  } else {
    dst_rb->count = dst_rb->size;  // Clamp to maximum size
#ifdef ESP_IDF_VERSION
    ESP_LOGW(TAG, "Ring dump overflow detected: attempted count %u, clamped to %u", 
             new_count, dst_rb->size);
#endif
  }
  #if USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  
#ifdef ESP_IDF_VERSION
  ESP_LOGI(TAG, "Ring dump completed: %u elements copied directly (%s source)", 
           copied_count, preserve_source ? "preserved" : "consumed");
#endif
  
  return copied_count;
}

// Dump limited number of elements from source ring buffer to destination ring buffer (direct buffer copy)
uint32_t RingBuffer_DumpToRingLimited(RingBuffer_t *src_rb, RingBuffer_t *dst_rb, uint32_t max_count, bool preserve_source) {
  if (src_rb == NULL || dst_rb == NULL || max_count == 0) {
    return 0;
  }
  
  // Check element size compatibility
  if (src_rb->element_size != dst_rb->element_size) {
#ifdef ESP_IDF_VERSION
    ESP_LOGW(TAG, "Ring dump limited failed: incompatible element sizes (src=%zu, dst=%zu)", 
             src_rb->element_size, dst_rb->element_size);
#endif
    return 0;
  }
  
  // Get available elements in source and free space in destination
  uint32_t src_available = RingBuffer_Available(src_rb);
  uint32_t dst_free = RingBuffer_Free(dst_rb);
  
  if (src_available == 0) {
    return 0;  // Nothing to copy
  }
  
  // Determine how many elements we can actually copy (limited by max_count)
  uint32_t elements_to_copy = src_available;
  if (elements_to_copy > max_count) {
    elements_to_copy = max_count;
  }
  if (elements_to_copy > dst_free) {
    elements_to_copy = dst_free;
  }
  
  if (elements_to_copy == 0) {
    return 0;  // Nothing to copy or destination is full
  }
  
  uint32_t copied_count = 0;
  uint32_t src_tail = src_rb->tail;
  uint32_t dst_head = dst_rb->head;
  
  // Direct buffer-to-buffer copying in chunks to handle wrap-around
  while (copied_count < elements_to_copy) {
    // Calculate how many elements we can copy in this chunk (until wrap-around)
    uint32_t src_chunk = src_rb->size - src_tail;
    uint32_t dst_chunk = dst_rb->size - dst_head;
    uint32_t remaining = elements_to_copy - copied_count;
    
    // Take the minimum of: remaining elements, source chunk, destination chunk
    uint32_t chunk_size = remaining;
    if (chunk_size > src_chunk) chunk_size = src_chunk;
    if (chunk_size > dst_chunk) chunk_size = dst_chunk;
    
    // Direct memory copy from source buffer to destination buffer
    void *src_ptr = (uint8_t *)src_rb->buffer + (src_tail * src_rb->element_size);
    void *dst_ptr = (uint8_t *)dst_rb->buffer + (dst_head * dst_rb->element_size);
    size_t copy_bytes = chunk_size * src_rb->element_size;
    
    memcpy(dst_ptr, src_ptr, copy_bytes);
    
    // Update positions
    copied_count += chunk_size;
    src_tail = (src_tail + chunk_size) % src_rb->size;
    dst_head = (dst_head + chunk_size) % dst_rb->size;
  }
  
  // Update ring buffer states atomically
  #if USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  
  if (!preserve_source) {
    // Consume source data by updating tail and count
    src_rb->tail = src_tail;
    if (src_rb->count >= copied_count) {
      src_rb->count -= copied_count;
    } else {
      src_rb->count = 0;  // Safety check to prevent underflow
    }
  }
  
  // Always update destination head and count with overflow protection
  dst_rb->head = dst_head;
  uint32_t new_count = dst_rb->count + copied_count;
  if (new_count <= dst_rb->size) {
    dst_rb->count = new_count;
  } else {
    dst_rb->count = dst_rb->size;  // Clamp to maximum size
#ifdef ESP_IDF_VERSION
    ESP_LOGW(TAG, "Ring dump limited overflow detected: attempted count %u, clamped to %u", 
             new_count, dst_rb->size);
#endif
  }
  #if USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  
#ifdef ESP_IDF_VERSION
  ESP_LOGI(TAG, "Ring dump limited completed: %u/%u elements copied directly (%s source)", 
           copied_count, max_count, preserve_source ? "preserved" : "consumed");
#endif
  
  return copied_count;
}