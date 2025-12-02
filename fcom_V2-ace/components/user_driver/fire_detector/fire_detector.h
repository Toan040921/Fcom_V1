/*
 * stm32_com.h
 *
 *  Created on: Jan 9, 2021
 *      Author: ductu
 */

#ifndef MAIN_USER_DRIVER_FIRE_DETECTOR_H_
#define MAIN_USER_DRIVER_FIRE_DETECTOR_H_

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "../../common_interface.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "string.h"
#include "stdio.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct {
    unsigned char data;
    unsigned int delay;
}send_pkg_t;

static const send_pkg_t start_config[] = {{0xFD, 1}, {0x00, 1}, {0xFE, 600}};
static const send_pkg_t stop_config[]  = {{0xFD, 1}, {0x01, 1}, {0xFE, 600}};
// static const unsigned char get_mac_addr[]  = { 10, 0xFD, 1, 0x05, 1, 0x02, 1, 0x04, 1, 0xFE, 255};
static const send_pkg_t get_uart_baudrate[]  = {{0xFD, 1}, {0x05, 1}, {0x00, 1}, {0x00, 1}, {0xFE, 600}};
static const send_pkg_t get_mac_info[]  = {{0xFD, 1}, {0x05, 1}, {0x02, 1}, {0x04, 1}, {0xFE, 600}};
static const send_pkg_t get_all_config[]  = {{0xFD, 1}, {0x05, 1}, {0xFF, 1}, {0xFE, 600}};

/****************************************************************************/
/***         Exported global functions                                     ***/
/****************************************************************************/
void fire_detector_tx_process_task_creat(void);

void RF_config_default(void);
void SET_RF__SYNC_config(unsigned char MK1, unsigned char MK2, unsigned char MK3); // ham cai dat RF

void rf_config_write(const send_pkg_t rfcfg[],int len);

#endif /* MAIN_USER_DRIVER_FIRE_DETECTOR_H_ */
