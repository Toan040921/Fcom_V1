/* esp_timer (high resolution timer) example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdbool.h>
#include "user_timer.h"

//int Counter_msec = 0;	// conter millisec

static void periodic_timer_callback(void *arg);

static uint32_t tick_time = 0;
void UserTimer_Init()
{
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback,
        /* name is optional, but may help identify the timer when debugging */
        .name = "periodic"};

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    /* The timer has been created but is not running yet */

    /* Start the timers */
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, TIMER_INTERVAL));
    //    /* Clean up and finish the example */
    //    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
    //    ESP_ERROR_CHECK(esp_timer_delete(periodic_timer));
}

uint32_t usertimer_gettick(void)
{
    return tick_time;
}

static void periodic_timer_callback(void *arg)
{
    if(tick_time >= UINT32_MAX)
    {
        tick_time = 0;
    }
    tick_time++;
}
