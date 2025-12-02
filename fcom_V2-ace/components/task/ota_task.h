/*
 * ota_task.h
 *
 *  Created on: Jan 7, 2021
 *      Author: ductu
 */

#ifndef MAIN_TASK_OTA_TASK_H_
#define MAIN_TASK_OTA_TASK_H_
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "../../common_interface.h"
#include "../../components/json_parser/json_parser.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***         Exported global functions                                     ***/
/****************************************************************************/
bool ota_process(void);
char* app_get_version(void);
#endif /* MAIN_TASK_OTA_TASK_H_ */
