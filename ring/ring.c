/***********************************************************
 * @file	ring.c
 * @author	Andy Chen (clgm216@gmail.com)
 * @version	0.02
 * @date	2025-04-09
 * @brief
 * **********************************************************
 * @copyright Copyright (c) 2025 TTK. All rights reserved.
 *
 ************************************************************/
#include "ring.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#if RING_USE_RTOS_MUTEX
static mutex_callbacks_t *s_cs_callbacks = NULL;
#define MUTEX_TIMEOUT_MS 1000

/* NEW: Simple callback-based macros accepting mutex parameter */
#ifdef ENTER_CRITICAL_SECTION
#undef ENTER_CRITICAL_SECTION
#endif
#define ENTER_CRITICAL_SECTION(mutex, time_out) do { \
  if (s_cs_callbacks && s_cs_callbacks->acquire && (mutex)) { \
    s_cs_callbacks->acquire(mutex, time_out); \
  } \
} while(0)

#ifdef EXIT_CRITICAL_SECTION
#undef EXIT_CRITICAL_SECTION
#endif
#define EXIT_CRITICAL_SECTION(mutex) do { \
  if (s_cs_callbacks && s_cs_callbacks->release && (mutex)) { \
    s_cs_callbacks->release(mutex); \
  } \
} while(0)

/**
 * @brief Register critical section callbacks with ring buffer
 * @param callbacks: Pointer to callback structure (NULL for no synchronization)
 * @return true on success
 */
bool ring_register_cs_callbacks(const mutex_callbacks_t *callbacks)
{
  s_cs_callbacks = (mutex_callbacks_t *)callbacks;
  return true;
}

#else
/* No-op macros if no RTOS mutex support */
#ifndef ENTER_CRITICAL_SECTION
#define ENTER_CRITICAL_SECTION()
#endif

#ifndef EXIT_CRITICAL_SECTION
#define EXIT_CRITICAL_SECTION()
#endif

#endif /* RING_USE_RTOS_MUTEX */

// Initialize the ring buffer
void ring_init(ring_t *rb, void *buffer, uint32_t size, size_t element_size) {
  rb->buffer = buffer;
  rb->head = 0;
  rb->tail = 0;
  rb->size = size;
  rb->count = 0;          // Initialize count to 0 (empty)
  rb->element_size = element_size;
  rb->owns_buffer = false;  // External buffer, not owned by ring buffer
  #if RING_USE_RTOS_MUTEX
  rb->mutex = NULL;
  /* Create a new mutex for this ring buffer instance */
  if (s_cs_callbacks && s_cs_callbacks->create) {
    rb->mutex = s_cs_callbacks->create();
  }
  #endif
}

// Initialize the ring buffer with dynamic allocation
bool ring_init_dynamic(ring_t *rb, uint32_t size, size_t element_size) {
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

  #if RING_USE_RTOS_MUTEX
  rb->mutex = NULL;
  /* Create a new mutex for this ring buffer instance */
  if (s_cs_callbacks && s_cs_callbacks->create) {
    rb->mutex = s_cs_callbacks->create();
  }
  #endif
  
#ifdef ESP_IDF_VERSION
  ESP_LOGI(TAG, "Dynamically allocated ring buffer: %u elements × %zu bytes = %zu bytes total", 
           size, element_size, total_size);
#endif
  
  return true;
}

// Destroy ring buffer and free dynamically allocated memory
void ring_destroy(ring_t *rb) {
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
  #if RING_USE_RTOS_MUTEX
  // Destroy the mutex if it exists
  if (rb->mutex && s_cs_callbacks && s_cs_callbacks->destroy) {
    s_cs_callbacks->destroy(rb->mutex);
  }
  rb->mutex = NULL;
  #endif
}

// Check if ring buffer owns its buffer
bool ring_is_owns_buffer(const ring_t *rb) {
  if (rb == NULL) {
    return false;
  }
  return rb->owns_buffer;
}

void ring_clear(ring_t *rb) {
  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  rb->head = 0;
  rb->tail = 0;
  rb->count = 0;
  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
}

// Check if the ring buffer is empty
bool ring_is_empty(const ring_t *rb) { return rb->count == 0; }

// Check if the ring buffer is full (now uses all available slots!)
bool ring_is_full(const ring_t *rb) { return rb->count == rb->size; }

// Write an element to the ring buffer
bool ring_write(ring_t *rb, const void *data) {
  if (ring_is_full(rb)) { return false; }
  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  // Calculate the offset and copy the data
  void *dest = (uint8_t *)rb->buffer + (rb->head * rb->element_size);
  memcpy(dest, data, rb->element_size);
  rb->head = (rb->head + 1) % rb->size; // Increment head with wrap-around
  rb->count++;                          // Increment count
  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  return true;
}

// Write an element to the ring buffer (overwrites oldest data if full)
bool ring_push_front(ring_t *rb, const void *data) {
  if (rb == NULL) { return false; }
  
  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  bool was_full = ring_is_full(rb);
  
  // Ring: overwrite oldest data when full
  void *dest = (uint8_t *)rb->buffer + (rb->head * rb->element_size);
  memcpy(dest, data, rb->element_size);
  
  rb->head = (rb->head + 1) % rb->size;
  
  if (was_full) {
    // Move tail forward to discard oldest data
    rb->tail = (rb->tail + 1) % rb->size;
    // Count stays the same (still full)
  } else {
    // Buffer wasn't full, increment count
    rb->count++;
  }
  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  return true;
}

// Write multiple elements to the ring buffer (overwrites oldest data if full)
uint32_t ring_push_back(ring_t *rb, const void *data, uint32_t count) {
  if (rb == NULL || data == NULL || count == 0) { return 0; }

  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
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
  
  rb->head = head;
  rb->tail = tail;
  rb->count = current_count;
  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif

  return elements_to_write;
}

// Read an element from the ring buffer
bool ring_read(ring_t *rb, void *data) {
  if (rb == NULL || ring_is_empty(rb)) {
    return false; // Buffer empty and ring type is static, read fails
  }
  // Calculate the offset and copy the data
  void *src = (uint8_t *)rb->buffer + (rb->tail * rb->element_size);
  memcpy(data, src, rb->element_size);
  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  rb->tail = (rb->tail + 1) % rb->size; // Increment tail with wrap-around
  rb->count--;                          // Decrement count
  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  return true;
}

// Get the number of elements available in the buffer
uint32_t ring_available(const ring_t *rb) { 
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
uint32_t ring_get_free(const ring_t *rb) {
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
uint32_t ring_write_multiple(ring_t *rb, const void *data, uint32_t count) {
  if (rb == NULL || data == NULL || count == 0) { return 0; }

  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif

  uint32_t free_slots = ring_get_free(rb);
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
  rb->head = (head + elements_to_write) % rb->size;
  rb->count += elements_to_write;
  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif

  return elements_to_write;
}

// Read multiple elements from the ring buffer (DMA-friendly)
uint32_t ring_read_multiple(ring_t *rb, void *data, uint32_t count) {
  if (rb == NULL || data == NULL || count == 0) { return 0; }

  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif

  uint32_t available = ring_available(rb);
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
  rb->tail = (tail + elements_to_read) % rb->size;
  rb->count -= elements_to_read;
  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif

  return elements_to_read;
}

// Remove a single element from the back of the ring buffer
bool ring_pop_back(ring_t *rb) {
  if (rb == NULL || ring_is_empty(rb)) {
    return false; // Buffer empty or invalid, pop fails
  }

  // Atomic decrement of head and count
  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  rb->head = (rb->head == 0) ? (rb->size - 1) : (rb->head - 1);
  rb->count--;
  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  return true;
}

// Remove multiple elements from the back of the ring buffer
uint32_t ring_pop_back_multiple(ring_t *rb, uint32_t count) {
  if (rb == NULL || count == 0 || ring_is_empty(rb)) { return 0; }

  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif

  // Limit count to available elements
  uint32_t available = ring_available(rb);
  uint32_t elements_to_remove = (count > available) ? available : count;

  // Atomic decrement of head and count
  if (rb->head >= elements_to_remove) {
    rb->head -= elements_to_remove;
  } else {
    rb->head = rb->size - (elements_to_remove - rb->head);
  }
  rb->count -= elements_to_remove;

  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  return elements_to_remove;
}

// Remove a single element from the front of the ring buffer (oldest element)
bool RingBuffer_PopFront(ring_t *rb) {
  if (rb == NULL || ring_is_empty(rb)) {
    return false; // Buffer empty or invalid, pop fails
  }

  // Atomic increment of tail and decrement count
  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  rb->tail = (rb->tail + 1) % rb->size;
  rb->count--;
  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  return true;
}

// Remove multiple elements from the front of the ring buffer (oldest elements)
uint32_t RingBuffer_PopFrontMultiple(ring_t *rb, uint32_t count) {
  if (rb == NULL || count == 0 || ring_is_empty(rb)) { return 0; }
  
  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  // Limit count to available elements
  uint32_t available = ring_available(rb);
  uint32_t elements_to_remove = (count > available) ? available : count;

  // Atomic increment of tail and decrement count
  rb->tail = (rb->tail + elements_to_remove) % rb->size;
  rb->count -= elements_to_remove;
  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  return elements_to_remove;
}

// Peek the oldest element from the ring buffer (does not move tail)
bool ring_peek_front(const ring_t *rb, void *data) {
  if (rb == NULL || ring_is_empty(rb)) { return false; }
  uint32_t pos = rb->tail;
  void *src = (uint8_t *)rb->buffer + (pos * rb->element_size);
  memcpy(data, src, rb->element_size);
  return true;
}

// Peek the newest element from the ring buffer (does not move head)
bool ring_peek_back(const ring_t *rb, void *data) {
  if (rb == NULL || ring_is_empty(rb)) { return false; }
  uint32_t pos = (rb->head == 0) ? (rb->size - 1) : (rb->head - 1);
  void *src = (uint8_t *)rb->buffer + (pos * rb->element_size);
  memcpy(data, src, rb->element_size);
  return true;
}

// Peek multiple elements from the back of the ring buffer (does not move head)
// The newest element is at index 0, next newest at index 1, etc.
uint32_t ring_peek_back_multiple(const ring_t *rb, void *data, uint32_t count) {
  if (rb == NULL || data == NULL || ring_is_empty(rb) || count == 0) { return 0; }
  uint32_t available = ring_available(rb);
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
uint32_t ring_peek_front_multiple(const ring_t *rb, void *data, uint32_t count) {
  if (rb == NULL || data == NULL || ring_is_empty(rb) || count == 0) { return 0; }
  uint32_t available = ring_available(rb);
  if (count > available) { count = available; }

  for (uint32_t i = 0; i < count; ++i) {
    uint32_t pos = (rb->tail + i) % rb->size;
    void *src = (uint8_t *)rb->buffer + (pos * rb->element_size);
    memcpy((uint8_t *)data + (i * rb->element_size), src, rb->element_size);
  }
  return count;
}

// Dump all elements from source ring buffer to destination ring buffer (direct buffer copy)
uint32_t ring_dump(ring_t *src_rb, ring_t *dst_rb, bool preserve_source) {
  if (src_rb == NULL || dst_rb == NULL) { return 0; }

  // Check element size compatibility
  if (src_rb->element_size != dst_rb->element_size) { return 0; }

  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(src_rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif

  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(dst_rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif

  // Get available elements in source and free space in destination
  uint32_t src_available = ring_available(src_rb);
  uint32_t dst_free = ring_get_free(dst_rb);
  // Determine how many elements we can actually copy
  uint32_t elements_to_copy = (src_available < dst_free) ? src_available : dst_free;
  
  // Source has no data to copy or destination is full
  if ((src_available == 0) ||(elements_to_copy == 0)) {
    #if RING_USE_RTOS_MUTEX
    EXIT_CRITICAL_SECTION(src_rb->mutex);
    #else
    EXIT_CRITICAL_SECTION();
    #endif
    #if RING_USE_RTOS_MUTEX
    EXIT_CRITICAL_SECTION(dst_rb->mutex);
    #else
    EXIT_CRITICAL_SECTION();
    #endif
    return 0;  // Nothing to copy
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
  
  // Update source ring buffer states atomically
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
  }
  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(src_rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(dst_rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  
  return copied_count;
}

// Dump limited number of elements from source ring buffer to destination ring buffer (direct buffer copy)
uint32_t ring_dump_count(ring_t *src_rb, ring_t *dst_rb, uint32_t max_count, bool preserve_source) {
  if (src_rb == NULL || dst_rb == NULL || max_count == 0) { return 0; }

  // Check element size compatibility
  if (src_rb->element_size != dst_rb->element_size) { return 0; }

  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(src_rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  #if RING_USE_RTOS_MUTEX
  ENTER_CRITICAL_SECTION(dst_rb->mutex, MUTEX_TIMEOUT_MS);
  #else
  ENTER_CRITICAL_SECTION();
  #endif
  

  // Get available elements in source and free space in destination
  uint32_t src_available = ring_available(src_rb);
  uint32_t dst_free = ring_get_free(dst_rb);
  
  // Determine how many elements we can actually copy (limited by max_count)
  uint32_t elements_to_copy = src_available;
  if (elements_to_copy > max_count) {
    elements_to_copy = max_count;
  }
  if (elements_to_copy > dst_free) {
    elements_to_copy = dst_free;
  }
  
  // Source has no data to copy or destination is full
  if ((elements_to_copy == 0) || (src_available == 0)) {
    #if RING_USE_RTOS_MUTEX
    EXIT_CRITICAL_SECTION(src_rb->mutex);
    #else
    EXIT_CRITICAL_SECTION();
    #endif
    #if RING_USE_RTOS_MUTEX
    EXIT_CRITICAL_SECTION(dst_rb->mutex);
    #else
    EXIT_CRITICAL_SECTION();
    #endif
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
  
  // Update source ring buffer states atomically
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
  }
  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(src_rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  #if RING_USE_RTOS_MUTEX
  EXIT_CRITICAL_SECTION(dst_rb->mutex);
  #else
  EXIT_CRITICAL_SECTION();
  #endif
  
  return copied_count;
}