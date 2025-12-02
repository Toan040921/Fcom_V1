/*
 * mqtt_task.h
 *
 *  Created on: Nov 24, 2020
 *      Author: Yolo
 */

#ifndef MAIN_TASK_MQTT_TASK_H_
#define MAIN_TASK_MQTT_TASK_H_
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "../../components/json_parser/json_parser.h"
#include "../../components/user_driver/flash/user_flash.h"
#include "../../common_interface.h"

#include "esp_wifi.h"
#include "esp_system.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

#include "esp_http_client.h"
#include "cJson_lib/cJSON.h"

#include "driver/gpio.h"
#include "ota_task.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_MQTT_GET_CER_LINK,
    E_MQTT_GET_CER_PEM_VALUE,
    E_MQTT_GET_PRIVATE_PEM_VALUE
}e_mqtt_get_state;

/****************************************************************************/
/***         Exported global functions                                     ***/
/****************************************************************************/
void mqtt_task_start(void);
bool mqtt_get_cer(void);
bool mqtt_get_cer_pem_file(void);
bool mqtt_get_private_pem_file(void);

esp_mqtt_client_handle_t mqtt_handle;
#endif /* MAIN_TASK_MQTT_TASK_H_ */
