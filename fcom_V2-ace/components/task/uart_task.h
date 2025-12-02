/*
 * uart_task.h
 *
 *  Created on: Nov 24, 2020
 *      Author: Yolo
 */

#ifndef MAIN_TASK_UART_TASK_H_
#define MAIN_TASK_UART_TASK_H_
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "../../common_interface.h"
// #include "../user_driver/sw_serial/sw_serial.h"
#include "../user_driver/flash/user_flash.h"
#include "driver/uart.h"            // for the uart driver access
#include "user_timer.h"
// #include "../user_driver/fire_detector/fire_detector.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
#define TXD_RF_PIN (GPIO_NUM_12)
#define RXD_RF_PIN (GPIO_NUM_14)

#define BUFF_SIZE         		(1024)
#define UART_BAUD_4800 			(4800)
#define UART_BAUD_115200 		(115200)
#define UART_BAUD_9600          (9600)

/****************************************************************************/
/***         Exported global functions                                     ***/
/****************************************************************************/
void uart_fire_detector_sensor_start(void);

void uart_fire_detector_send(unsigned char* buf, unsigned char size);
void uart_fire_detector_sensor_init(void);

#endif /* MAIN_TASK_UART_TASK_H_ */
