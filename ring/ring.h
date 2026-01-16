/***********************************************************
 * @file	ring.h
 * @author	Andy Chen (clgm216@gmail.com)
 * @version	0.02
 * @date	2025-04-09
 * @brief
 * **********************************************************
 * @copyright Copyright (c) 2025 TTK. All rights reserved.
 *
 ************************************************************/
#ifndef RING_H_
#define RING_H_
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define RING_USE_RTOS_MUTEX 1  /* Set to 1 to enable RTOS mutex support */

typedef struct {
  void *buffer;        // Pointer to the buffer (allocated elsewhere or dynamically)
  void *mutex;         // Per-instance mutex handle (created during Init)
  bool cs_used_mutex;  // Tracks whether enter_cs() successfully acquired the mutex
  uint32_t primask_bit; // For non-RTOS critical sections
  uint32_t head;       // Write index
  uint32_t tail;       // Read index
  uint32_t size;       // Maximum number of elements
  uint32_t count;      // Current number of elements in buffer (optimizes full/empty detection)
  size_t element_size; // Size of each element in bytes
  bool owns_buffer;    // true if buffer was dynamically allocated and should be freed
} ring_t;

#if RING_USE_RTOS_MUTEX
#include "mutex_common.h"

/**
 * @brief Register critical section callbacks with ring buffer
 * @param callbacks: Pointer to callback structure (NULL for no synchronization)
 * @return true on success
 */
bool ring_register_cs_callbacks(const mutex_callbacks_t *callbacks);
#endif /* RING_USE_RTOS_MUTEX */

/**
 * @brief Initializes a ring buffer.
 *
 * This function sets up a ring buffer structure with the provided buffer, size, 
 * and element size. It prepares the ring buffer for use by initializing its 
 * internal state.
 *
 * @param rb Pointer to the ring_t structure to initialize.
 * @param buffer Pointer to the memory buffer that will be used to store the ring buffer's data.
 *               The buffer must be pre-allocated and large enough to hold the specified size 
 *               multiplied by the element size.
 * @param size The number of elements the ring buffer can hold.
 * @param element_size The size of each element in bytes.
 *
 * @note The provided buffer must remain valid for the lifetime of the ring buffer.
 *       The caller is responsible for ensuring that the buffer is properly allocated 
 *       and freed when no longer needed.
 */
void ring_init(ring_t *rb, void *buffer, uint32_t size, size_t element_size);

/**
 * @brief Initializes a ring buffer with dynamically allocated buffer.
 *
 * This function sets up a ring buffer structure and dynamically allocates the required
 * memory buffer based on the size and element size. The allocated buffer will be automatically
 * freed when ring_destroy() is called.
 *
 * @param rb Pointer to the ring_t structure to initialize.
 * @param size The number of elements the ring buffer can hold.
 * @param element_size The size of each element in bytes.
 *
 * @return true if initialization and memory allocation succeeded.
 * @return false if memory allocation failed or invalid parameters provided.
 *
 * @note The dynamically allocated buffer will be freed automatically when ring_destroy() 
 *       is called. Do not free the buffer manually.
 * @note Call ring_destroy() to properly clean up the ring buffer and free allocated memory.
 */
bool ring_init_dynamic(ring_t *rb, uint32_t size, size_t element_size);

/**
 * @brief Writes data into the ring buffer.
 *
 * This function attempts to write the specified data into the ring buffer.
 * If the buffer has enough space, the data is written, and the function
 * returns successfully. If the buffer is full, the write operation fails.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * @param data Pointer to the data to be written into the ring buffer.
 *             The data must be valid and should not be NULL.
 * 
 * @return true if the data was successfully written to the ring buffer.
 * @return false if the ring buffer is full and the data could not be written.
 */
bool ring_write(ring_t *rb, const void *data);

/**
 * @brief Reads data from the ring buffer.
 *
 * This function attempts to read data from the ring buffer. If the buffer
 * contains data, the data is read, and the function returns successfully.
 * If the buffer is empty, the read operation fails.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * @param data Pointer to the memory location where the read data will be stored.
 *             The memory must be valid and should not be NULL.
 * 
 * @return true if the data was successfully read from the ring buffer.
 * @return false if the ring buffer is empty and no data could be read.
 */
bool ring_read(ring_t *rb, void *data);

/**
 * @brief Checks if the ring buffer is empty.
 *
 * This function checks whether the ring buffer contains any data.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * 
 * @return true if the ring buffer is empty.
 * @return false if the ring buffer contains data.
 */
bool ring_is_empty(const ring_t *rb);

/**
 * @brief Checks if the ring buffer is full.
 *
 * This function checks whether the ring buffer has reached its maximum capacity.
 * OPTIMIZATION: Now uses count-based detection allowing ALL buffer slots to be used!
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * 
 * @return true if the ring buffer is full (count == size).
 * @return false if the ring buffer has space for more data.
 */
bool ring_is_full(const ring_t *rb);

/**
 * @brief Gets the number of elements available in the ring buffer.
 *
 * This function calculates the number of elements currently stored in the ring buffer.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * 
 * @return The number of elements available in the ring buffer.
 */
uint32_t ring_available(const ring_t *rb);


/**
 * @brief Clears the ring buffer.
 *
 * This function resets the ring buffer, clearing all data and resetting the head and tail indices.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 */
void ring_clear(ring_t *rb);


/**
 * @brief Gets the free space available in the ring buffer.
 *
 * This function calculates the number of elements that can still be written to the ring buffer.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * 
 * @return The number of free elements available in the ring buffer.
 */
uint32_t ring_get_free(const ring_t *rb);

/**
 * @brief Writes multiple elements to the ring buffer.
 *
 * This function writes multiple elements to the ring buffer.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * @param data Pointer to the data to write to the ring buffer.
 * @param count The number of elements to write.
 * 
 * @return The number of elements actually written to the ring buffer.
 */
uint32_t ring_write_multiple(ring_t *rb, const void *data, uint32_t count);

/**
 * @brief Reads multiple elements from the ring buffer.
 *
 * This function attempts to read multiple elements from the ring buffer into the provided data buffer.
 * It reads up to 'count' elements, depending on how many are available in the ring buffer.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * @param data Pointer to the memory location where the read data will be stored.
 *             The memory must be valid and large enough to hold 'count' elements.
 * @param count The number of elements to read from the ring buffer.
 * 
 * @return The number of elements actually read from the ring buffer.
 */
uint32_t ring_read_multiple(ring_t *rb, void *data, uint32_t count);

/**
 * @brief Pops an element from the back of the ring buffer.
 *
 * This function removes the last element from the ring buffer and returns it.
 * If the ring buffer is empty, the operation fails.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * 
 * @return true if an element was successfully popped from the back of the ring buffer.
 * @return false if the ring buffer is empty and no element could be popped.
 */
bool ring_pop_back(ring_t *rb);

/**
 * @brief Pops multiple elements from the back of the ring buffer.
 *
 * This function removes multiple elements from the back of the ring buffer.
 * It attempts to pop 'count' elements, depending on how many are available in the ring buffer.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * @param count The number of elements to pop from the back of the ring buffer.
 * 
 * @return The number of elements actually popped from the back of the ring buffer.
 */
uint32_t ring_pop_back_multiple(ring_t *rb, uint32_t count);

/**
 * @brief Pushes an element to the front of the ring buffer (overwrites oldest if full).
 *
 * This function writes an element to the front of the ring buffer. If the buffer is full,
 * it overwrites the oldest data.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * @param data Pointer to the data to be written into the ring buffer.
 *             The data must be valid and should not be NULL.
 * 
 * @return true if the data was successfully written to the ring buffer.
 * @return false if the ring buffer pointer is NULL and the write fails.
 */
bool ring_push_front(ring_t *rb, const void *data);

/**
 * @brief Pushes multiple elements to the back of the ring buffer (overwrites oldest if full).
 *
 * This function writes multiple elements to the back of the ring buffer. If the buffer is full,
 * it overwrites the oldest data.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * @param data Pointer to the data to be written into the ring buffer.
 *             The data must be valid and should not be NULL.
 * @param count The number of elements to write to the ring buffer.
 * 
 * @return The number of elements actually written to the ring buffer.
 */
uint32_t ring_push_back(ring_t *rb, const void *data, uint32_t count);

/**
 * @brief Peeks at the oldest element in the ring buffer (does not move tail).
 *
 * This function allows you to look at the oldest element in the ring buffer without removing it.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * @param data Pointer to the memory location where the peeked data will be stored.
 *             The memory must be valid and should not be NULL.
 * 
 * @return true if the data was successfully peeked from the ring buffer.
 * @return false if the ring buffer is empty or the pointer is NULL, and no data could be peeked.
 */
bool ring_peek_front(const ring_t *rb, void *data);

/**
 * @brief Peeks at the newest element in the ring buffer (does not move head).
 *
 * This function allows you to look at the newest element in the ring buffer without removing it.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * @param data Pointer to the memory location where the peeked data will be stored.
 *             The memory must be valid and should not be NULL.
 * 
 * @return true if the data was successfully peeked from the ring buffer.
 * @return false if the ring buffer is empty or the pointer is NULL, and no data could be peeked.
 */
bool ring_peek_back(const ring_t *rb, void *data);

/**
 * @brief Peeks at multiple elements in the ring buffer starting from a specific index (does not move head or tail).
 *
 * This function allows you to look at multiple elements in the ring buffer starting from a specified index
 * without removing them. The elements are copied into the provided data buffer.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * @param data Pointer to the memory location where the peeked data will be stored.
 *             The memory must be valid and large enough to hold 'count' elements.
 * @param count The number of elements to peek at from the specified start index.
 * 
 * @return The number of elements actually peeked from the ring buffer.
 */
uint32_t ring_peek_back_multiple(const ring_t *rb, void *data, uint32_t count);

/**
 * @brief Peeks at multiple elements in the ring buffer starting from the oldest element (does not move head or tail).
 *
 * This function allows you to look at multiple elements in the ring buffer starting from the oldest element
 * without removing them. The elements are copied into the provided data buffer.
 *
 * @param rb Pointer to the ring buffer structure. Must be initialized before use.
 * @param data Pointer to the memory location where the peeked data will be stored.
 *             The memory must be valid and large enough to hold 'count' elements.
 * @param count The number of elements to peek at from the front of the ring buffer.
 * 
 * @return The number of elements actually peeked from the ring buffer.
 */
uint32_t ring_peek_front_multiple(const ring_t *rb, void *data, uint32_t count);

/**
 * @brief Destroys a ring buffer and frees dynamically allocated memory if applicable.
 *
 * This function cleans up a ring buffer structure. If the buffer was dynamically allocated
 * using ring_init_dynamic(), this function will free the allocated memory.
 *
 * @param rb Pointer to the ring buffer structure to destroy.
 *
 * @note Only call this function for ring buffers that were initialized with ring_init_dynamic().
 * @note For ring buffers initialized with ring_init() using external buffers, this function
 *       will only clear the structure without freeing the buffer.
 */
void ring_destroy(ring_t *rb);

/**
 * @brief Checks if the ring buffer owns its buffer (dynamically allocated).
 *
 * This function returns whether the ring buffer was initialized with dynamic allocation
 * and is responsible for freeing its buffer memory.
 *
 * @param rb Pointer to the ring buffer structure.
 * 
 * @return true if the buffer was dynamically allocated by the ring buffer.
 * @return false if the buffer was provided externally or ring buffer is NULL.
 */
bool ring_is_owns_buffer(const ring_t *rb);

/**
 * @brief Dumps (copies) all elements from source ring buffer to destination ring buffer.
 *
 * This function reads all available elements from the source ring buffer and writes them
 * to the destination ring buffer. The source ring buffer is not modified (elements remain).
 * The destination ring buffer must have compatible element size and sufficient space.
 *
 * @param src_rb Pointer to the source ring buffer to read from.
 * @param dst_rb Pointer to the destination ring buffer to write to.
 * @param preserve_source If true, source data is preserved (peeked). If false, source data is consumed (read).
 * 
 * @return The number of elements successfully copied from source to destination.
 * @return 0 if either ring buffer is NULL, incompatible element sizes, or no data to copy.
 *
 * @note Both ring buffers must have the same element_size for compatibility.
 * @note If destination buffer becomes full, copying stops and returns the count of elements copied.
 * @note Use preserve_source=true to keep source data intact, false to consume it during copy.
 */
uint32_t ring_dump(ring_t *src_rb, ring_t *dst_rb, bool preserve_source);

/**
 * @brief Dumps (copies) a specific number of elements from source to destination ring buffer.
 *
 * This function reads up to 'max_count' elements from the source ring buffer and writes them
 * to the destination ring buffer. Provides more control over how many elements to copy.
 *
 * @param src_rb Pointer to the source ring buffer to read from.
 * @param dst_rb Pointer to the destination ring buffer to write to.
 * @param max_count Maximum number of elements to copy.
 * @param preserve_source If true, source data is preserved (peeked). If false, source data is consumed (read).
 * 
 * @return The number of elements successfully copied from source to destination.
 * @return 0 if either ring buffer is NULL, incompatible element sizes, or no data to copy.
 *
 * @note Both ring buffers must have the same element_size for compatibility.
 * @note Actual copied count may be less than max_count due to source availability or destination capacity.
 */
uint32_t ring_dump_count(ring_t *src_rb, ring_t *dst_rb, uint32_t max_count, bool preserve_source);

#endif