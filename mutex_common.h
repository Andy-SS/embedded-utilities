/***********************************************************
* @file	mutex_common.h
* @author	Andy Chen (clgm216@gmail.com)
* @version	0.01
* @date	2025-12-25
* @brief  
* **********************************************************
* @copyright Copyright (c) 2025 TTK. All rights reserved.
* 
************************************************************/
#ifndef UTILITIES_MUTEX_COMMON_H_
#define UTILITIES_MUTEX_COMMON_H_
#include <stdint.h>
#include <stdbool.h>

#define RING_USE_RTOS_MUTEX 1  /* Set to 1 to enable RTOS mutex support */
#define MUTEX_TIMEOUT_MS 500 /* Default mutex timeout in milliseconds */
/* Unified Mutex Result Codes */
typedef enum {
  MUTEX_OK = 0,
  MUTEX_TIMEOUT,
  MUTEX_ERROR,
  MUTEX_NOT_SUPPORTED
} mutex_result_t;

/* Unified Mutex Callback Function Prototypes */
typedef void* (*mutex_create_fn)(void);           /* Create mutex, return handle */
typedef mutex_result_t (*mutex_destroy_fn)(void *mutex);    /* Destroy mutex */
typedef mutex_result_t (*mutex_acquire_fn)(void *mutex, uint32_t timeout_ms);
typedef mutex_result_t (*mutex_release_fn)(void *mutex);

/* Unified Mutex Callbacks Structure */
typedef struct {
  mutex_create_fn create;
  mutex_destroy_fn destroy;
  mutex_acquire_fn acquire;
  mutex_release_fn release;
} mutex_callbacks_t;

void utilities_register_cs_cbs(const mutex_callbacks_t *callbacks);

bool utilities_is_RTOS_ready(void);

void utilities_set_RTOS_ready(bool status);

mutex_result_t utilities_mutex_create(void* mutex);

mutex_result_t utilities_mutex_take(void *mutex, uint32_t timeout_ms);

mutex_result_t utilities_mutex_give(void *mutex);

/**
 * @brief Delete a mutex
 * @return Thread operation result
 */
mutex_result_t utilities_mutex_delete(void *mutex);
#endif /* UTILITIES_MUTEX_COMMON_H_ */