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

void* utilities_mutex_create(void){
  if(utilities_is_RTOS_ready() && cs_callbacks != NULL){
    return cs_callbacks->create();
  }
  // Return NULL if RTOS not ready or callbacks not registered
  return NULL;
}

mutex_result_t utilities_mutex_take(void *mutex, uint32_t timeout_ms){
  // If mutex is NULL, cannot take (caller should fall back to interrupt disable)
  if(mutex == NULL){
    return MUTEX_ERROR;
  }
  
  // If RTOS not ready or callbacks not registered, also fail
  if(!utilities_is_RTOS_ready() || cs_callbacks == NULL){
    return MUTEX_ERROR;
  }
  
  return cs_callbacks->acquire(mutex, timeout_ms);
}

mutex_result_t utilities_mutex_give(void *mutex){
  // If mutex is NULL, cannot give (caller must have used interrupt disable)
  if(mutex == NULL){
    return MUTEX_ERROR;
  }
  
  // If RTOS not ready or callbacks not registered, also fail
  if(!utilities_is_RTOS_ready() || cs_callbacks == NULL){
    return MUTEX_ERROR;
  }
  
  return cs_callbacks->release(mutex);
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