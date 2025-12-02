#pragma once
#ifndef _HMI_SERVICE_H
#define _HMI_SERVICE_H

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "user_timer.h"
#include "flash/user_flash.h"
#include "../../common_interface.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
#define GPIO_DK_LED_1       2
#define GPIO_DK_LED_2       18
#define GPIO_OUTPUT_PIN_SEL ((1ULL<<GPIO_DK_LED_1) | (1ULL<<GPIO_DK_LED_2))

#define GPIO_DK_RELAY       15
#define GPIO_OUT_24V        13
#define GPIO_OUTPUT_PIN_RELAY_SEL ((1ULL<<GPIO_DK_RELAY) | (1ULL<<GPIO_OUT_24V))

#define GPIO_USER_BUTTON    21
#define GPIO_INPUT_1        22
#define GPIO_INPUT_2        23
#define GPIO_INPUT_PIN_SEL  ((1ULL << GPIO_USER_BUTTON) | (1ULL << GPIO_INPUT_1) | (1ULL << GPIO_INPUT_2))

#define INPUT_FIRE_PIN      GPIO_INPUT_1
#define INPUT_FAULT_PIN     GPIO_INPUT_2

/****************************************************************************/
/***         Exported global functions                                     ***/
/****************************************************************************/
void hmi_task(void);
void update_relay_alarm_state(uint8_t dev_alert);
#endif //_HMI_SERVICE_H