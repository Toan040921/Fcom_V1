/*
 * user_flash.h
 *
 *  Created on: Jan 9, 2021
 *      Author: ductu
 */

#ifndef MAIN_USER_DRIVER_USER_FLASH_SENSOR_H_
#define MAIN_USER_DRIVER_USER_FLASH_SENSOR_H_

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "../../common_interface.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***         Exported global functions                                     ***/
/****************************************************************************/
void flash_file_init(void);

void flash_client_cert_pem_save(void);
void flash_client_private_pem_save(void);
void flash_client_id_save(void);

bool flash_client_cert_pem_read(void);
bool flash_client_private_pem_read(void);
bool flash_client_id_read(void);

void flash_save_pair_key(const wifi_author_t input);
bool flash_read_pair_key(void);

bool logger_list_file(char *location);
uint8_t check_map_size(void);

void flash_save_sensor_data(void);
bool flash_read_sensor_data(void);
bool flash_erase_sensor_data(int index);

void flash_erase_all_partions(void);
#endif /* MAIN_USER_DRIVER_USER_FLASH_SENSOR_H_ */
