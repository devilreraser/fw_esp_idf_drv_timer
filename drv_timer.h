/* *****************************************************************************
 * File:   drv_timer.h
 * Author: Dimitar Lilov
 *
 * Created on 2022 06 18
 * 
 * Description: ...
 * 
 **************************************************************************** */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/* *****************************************************************************
 * Header Includes
 **************************************************************************** */
#include "freertos/FreeRTOS.h"

#include <stdint.h>
    
/* *****************************************************************************
 * Configuration Definitions
 **************************************************************************** */

/* *****************************************************************************
 * Constants and Macros Definitions
 **************************************************************************** */

/* *****************************************************************************
 * Enumeration Definitions
 **************************************************************************** */

/* *****************************************************************************
 * Type Definitions
 **************************************************************************** */
typedef void (*drv_timer_periodic_function_t)(TickType_t ticks);

/* *****************************************************************************
 * Function-Like Macro
 **************************************************************************** */

/* *****************************************************************************
 * Variables External Usage
 **************************************************************************** */ 

/* *****************************************************************************
 * Function Prototypes
 **************************************************************************** */
void drv_timer_periodic_function_deny(drv_timer_periodic_function_t function);
void drv_timer_periodic_function_allow(drv_timer_periodic_function_t function);
void drv_timer_periodic_function_register(drv_timer_periodic_function_t function);
void drv_timer_periodic_function_deregister(drv_timer_periodic_function_t function);
void drv_timer_periodic_task_init(uint32_t delay_ms);

void drv_timer_periodic_init(uint32_t delay_ms);
void drv_timer_periodic_stop(void);
void drv_timer_one_shot_init(uint32_t delay_ms);


#ifdef __cplusplus
}
#endif /* __cplusplus */


