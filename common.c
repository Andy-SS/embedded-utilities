/***********************************************************
 * @file	common.c
* @author	Andy Chen (clgm216@gmail.com)
* @version	0.01
* @date	2025-12-25
* @brief  
* **********************************************************
* @copyright Copyright (c) 2025 TTK. All rights reserved.
* 
************************************************************/
#include "mutex_common.h"
#include <stdbool.h>
#include <stddef.h>

static mutex_callbacks_t *cs_callbacks = NULL;
static volatile bool RTOS_READY = false; // Flag to indicate if RTOS is ready

void utilities_set_RTOS_ready(bool status){
  RTOS_READY = status;
}

void utilities_register_cs_cbs(const mutex_callbacks_t *callbacks){
  cs_callbacks = (mutex_callbacks_t *)callbacks;
}

bool utilities_is_RTOS_ready(void){
  return RTOS_READY;
}

mutex_result_t utilities_mutex_create(void* mutex){
  if(utilities_is_RTOS_ready() && cs_callbacks != NULL){
    mutex = cs_callbacks->create();
  }
  // Return success if mutex handle is valid
  if (mutex != NULL){
    return MUTEX_OK;
  }
  // Return error if mutex creation failed
  return MUTEX_ERROR;
}

mutex_result_t utilities_mutex_take(void *mutex, uint32_t timeout_ms){
  if(utilities_is_RTOS_ready() && cs_callbacks != NULL && mutex != NULL){
    return cs_callbacks->acquire(mutex, timeout_ms);
  }
  return MUTEX_ERROR;
}

mutex_result_t utilities_mutex_give(void *mutex){
  if(utilities_is_RTOS_ready() && cs_callbacks != NULL && mutex != NULL){
    return cs_callbacks->release(mutex);
  }
  return MUTEX_ERROR;
}

mutex_result_t utilities_mutex_delete(void *mutex){
  if (utilities_is_RTOS_ready() && cs_callbacks != NULL && mutex != NULL)
  {
    return cs_callbacks->destroy(mutex);
  }
  else
  {
    return MUTEX_OK;
  }
}