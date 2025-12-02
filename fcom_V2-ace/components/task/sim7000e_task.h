/*
 * sim7000e_task.h
 *
 *  Created on: Nov 24, 2020
 *      Author: Yolo
 */

#ifndef MAIN_TASK_SIM7000E_TASK_H_
#define MAIN_TASK_SIM7000E_TASK_H_
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "../../common_interface.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "driver/gpio.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
#define TXD_PIN (GPIO_NUM_5)
#define RXD_PIN (GPIO_NUM_4)

#define BUFF_SIZE         		(1024)
#define UART_BAUD_4800 			(4800)
#define UART_BAUD_115200 		(115200)
#define UART_BAUD_9600          (9600)

#define GPIO_SIM_PWRKEY         33
#define GPIO_SIM_RESET          32
#define GPIO_OUTPUT_PIN_SIM_SEL ((1ULL<<GPIO_SIM_PWRKEY) | (1ULL<<GPIO_SIM_RESET))
/****************************************************************************/
/***         Exported global functions                                     ***/
/****************************************************************************/
void sim_init(void);
void sim_start(void);
void sim_gpio_init(void);
void sim_hard_reset(void);
#endif /* MAIN_TASK_UART_TASK_H_ */
