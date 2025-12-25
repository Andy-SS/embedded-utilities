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

#endif /* UTILITIES_MUTEX_COMMON_H_ */