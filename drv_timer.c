/* *****************************************************************************
 * File:   drv_timer.c
 * Author: Dimitar Lilov
 *
 * Created on 2022 06 18
 * 
 * Description: ...
 * 
 **************************************************************************** */

/* *****************************************************************************
 * Header Includes
 **************************************************************************** */
#include "drv_timer.h"

#include <sdkconfig.h>

#include <stdbool.h>
//#include <stdio.h>
//#include <string.h>
#include <unistd.h>     //time.h inside

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_log.h"
/* *****************************************************************************
 * Configuration Definitions
 **************************************************************************** */
#define TAG "drv_timer"

#define DRV_TIMER_FUNCTIONS_MAX 10

/* *****************************************************************************
 * Constants and Macros Definitions
 **************************************************************************** */


/* *****************************************************************************
 * Enumeration Definitions
 **************************************************************************** */

/* *****************************************************************************
 * Type Definitions
 **************************************************************************** */

/* *****************************************************************************
 * Function-Like Macros
 **************************************************************************** */

/* *****************************************************************************
 * Variables Definitions
 **************************************************************************** */
TaskHandle_t periodic_task = NULL;
drv_timer_periodic_function_t timer_periodic_function_list[DRV_TIMER_FUNCTIONS_MAX] = { NULL }; 
bool deny_function_list[DRV_TIMER_FUNCTIONS_MAX] = { false };

esp_timer_handle_t periodic_timer = NULL;

/* *****************************************************************************
 * Prototype of functions definitions
 **************************************************************************** */

/* *****************************************************************************
 * Functions
 **************************************************************************** */
int drv_timer_periodic_find_index(drv_timer_periodic_function_t function)
{
    int index;
    for (index = 0; index < sizeof(timer_periodic_function_list)/sizeof(drv_timer_periodic_function_t); index++)
    {
        if (function == timer_periodic_function_list[index])
        {
            return index;
        }
    }
    return index;
}

void drv_timer_periodic_function_deny(drv_timer_periodic_function_t function)
{
    int index = drv_timer_periodic_find_index(function);
    if (index < sizeof(timer_periodic_function_list)/sizeof(drv_timer_periodic_function_t))
    {
        deny_function_list[index] = true;
    }
}

void drv_timer_periodic_function_allow(drv_timer_periodic_function_t function)
{
    int index = drv_timer_periodic_find_index(function);
    if (index < sizeof(timer_periodic_function_list)/sizeof(drv_timer_periodic_function_t))
    {
        deny_function_list[index] = false;
    }
}

void drv_timer_periodic_function_register(drv_timer_periodic_function_t function)
{
    int index = drv_timer_periodic_find_index(NULL);    //find first empty
    if (index < sizeof(timer_periodic_function_list)/sizeof(drv_timer_periodic_function_t))
    {
        timer_periodic_function_list[index] = function;
    }
}

void drv_timer_periodic_function_deregister(drv_timer_periodic_function_t function)
{
    int index = drv_timer_periodic_find_index(function);    //find first empty
    if (index < sizeof(timer_periodic_function_list)/sizeof(drv_timer_periodic_function_t))
    {
        timer_periodic_function_list[index] = NULL;
    }
}

void drv_periodic_task(void* arg)
{
    TickType_t xLastWakeTime;
    uint32_t period_task_ms = (uint32_t)arg;
    TickType_t period_task_ticks = period_task_ms / (1000 / configTICK_RATE_HZ);

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();

    while(periodic_task != NULL)
    {
        // Wait for the next cycle.
        vTaskDelayUntil( &xLastWakeTime, period_task_ticks );

        // Perform action here.
        for (int index = 0; index < sizeof(timer_periodic_function_list)/sizeof(drv_timer_periodic_function_t); index++)
        {
            if (NULL != timer_periodic_function_list[index])
            {
                timer_periodic_function_list[index](period_task_ticks);
            }
        }
    }
    vTaskDelete(NULL);
}


void drv_timer_periodic_task_init(uint32_t delay_ms)
{
    periodic_task = NULL;
    vTaskDelay(pdMS_TO_TICKS(1000));
    xTaskCreate(drv_periodic_task, "periodic", 2048 + 1024, (void*)(pdMS_TO_TICKS(delay_ms)), configMAX_PRIORITIES - 1, &periodic_task);
}






static void periodic_timer_callback(void* arg)
{
    uint32_t period_ms = (uint32_t)arg;
    TickType_t period_ticks = period_ms / (1000 / configTICK_RATE_HZ);

    // Perform action here.
    for (int index = 0; index < sizeof(timer_periodic_function_list)/sizeof(drv_timer_periodic_function_t); index++)
    {
        if (NULL != timer_periodic_function_list[index])
        {
            timer_periodic_function_list[index](period_ticks);
        }
    }


    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGD(TAG, "Periodic timer called, time since boot: %lld us", time_since_boot);
}

static void oneshot_timer_callback(void* arg)
{
    uint32_t one_shot_ms = (uint32_t)arg;
    //TickType_t one_shot_ticks = one_shot_ms / (1000 / configTICK_RATE_HZ);

    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "One-shot timer called, time since boot: %lld us | one_shot:%u ms", time_since_boot, one_shot_ms);

    // esp_timer_handle_t periodic_timer_handle = (esp_timer_handle_t) arg;
    // /* To start the timer which is running, need to stop it first */
    // ESP_ERROR_CHECK(esp_timer_stop(periodic_timer_handle));
    // ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer_handle, 1000000));
    // time_since_boot = esp_timer_get_time();
    // ESP_LOGI(TAG, "Restarted periodic timer with 1s period, time since boot: %lld us", time_since_boot);
}








void drv_timer_periodic_init(uint32_t delay_ms)
{
    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            .arg = (void*) delay_ms,
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };

    
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    /* The timer has been created but is not running yet */

    //start
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, delay_ms*1000));
    ESP_LOGI(TAG, "Started periodic timer, time since boot: %lld us", esp_timer_get_time());

    /* Print debugging information about timers to console every 2 seconds */
    for (int i = 0; i < 5; ++i) {
        ESP_ERROR_CHECK(esp_timer_dump(stdout));
        usleep(2000000);
    }
}

void drv_timer_periodic_stop(void)
{
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
    ESP_ERROR_CHECK(esp_timer_delete(periodic_timer));
    ESP_LOGI(TAG, "Stopped and deleted periodic timer");
}


void drv_timer_one_shot_init(uint32_t delay_ms)
{
    const esp_timer_create_args_t oneshot_timer_args = {
            .callback = &oneshot_timer_callback,
            /* argument specified here will be passed to timer callback function */
            .arg = (void*) delay_ms,
            .name = "one-shot"
    };
    esp_timer_handle_t oneshot_timer;
    ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &oneshot_timer));

    //start
    ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, delay_ms*1000));
    ESP_LOGI(TAG, "Started one shot timer, time since boot: %lld us", esp_timer_get_time());

    /* Print debugging information about timers to console every 2 seconds */
    for (int i = 0; i < 5; ++i) {
        ESP_ERROR_CHECK(esp_timer_dump(stdout));
        usleep(2000000);
    }

    /* Timekeeping continues in light sleep, and timers are scheduled
     * correctly after light sleep.
     */
    int64_t t1 = esp_timer_get_time();
    ESP_LOGI(TAG, "Entering light sleep for 0.5s, time since boot: %lld us", t1);

    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(500000));
    esp_light_sleep_start();

    int64_t t2 = esp_timer_get_time();
    ESP_LOGI(TAG, "Woke up from light sleep, time since boot: %lld us", t2);

    assert(llabs((t2 - t1) - 500000) < 1000);

    /* Let the timer run for a little bit more */
    usleep(2000000);

    /* Clean up and finish the example */
    ESP_ERROR_CHECK(esp_timer_delete(oneshot_timer));
    ESP_LOGI(TAG, "Stopped and deleted one shot timer");

}