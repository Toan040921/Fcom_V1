/*
 * common.h
 *
 *  Created on: Apr 24, 2021
 *      Author: ductu
 */
#pragma once
#ifndef MAIN_COMMON_INTERFACE_H_
#define MAIN_COMMON_INTERFACE_H_
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***         Exported global functions                                     ***/
/****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#define MAX_DECTECTOR 50

extern bool rf_config_mode;
typedef enum {
	INIT_CMD = 0,
	FINDING_SUB,
	WAIT_ADD_SUB,
	CONFIG_ADD_MODE,
	REGISTER_SUB,
	CONFIG_LISTEN_MODE,
	LISTEN_SUB_EVENT,
	FINISHED_ADD_SUB,
} master_rf_event_t;
extern master_rf_event_t ssStateMachine;

typedef enum
{
	GW_ADD_SUB_DEVICE_FLAG = 0,
	GW_FEEDBACK_FLAG,
	SS_FEEDBACK_FLAG,
	JOB_REQUESTED_FEEDBACK_FLAG,
	SS_REGISTER_SERVER,
	SS_UPLOAD_DB,
	JOB_REQUESTED_OTA,
	GW_RESET_SS_STATE,
	SS_ERROR_FLAG,
} data_event_t;

extern xQueueHandle data_process;

typedef enum
{
    NORMAL = 0,
	ADD_SUB_DEVICE,
	ERROR
} gw_status_t;

typedef enum
{
    LOW = 0,
    MIDDLE,
	HIGH
} bat_state_t;

typedef enum
{
    ETHERNET = 0,
    WIFI,
	SIM3G,
} gw_connect_info_t;

typedef enum
{
	NORMAL_ST = 0,
	ALARM_ST,
	ERROR_ST,
	MAX_ST
} sensor_state_t;

typedef enum
{
	False_ST = 0,
	True_ST
} connect_state_t;

typedef struct
{
	sensor_state_t dtor_sensor_state[MAX_DECTECTOR+1];
	bat_state_t    dtor_bat_state[MAX_DECTECTOR+1];
	//connect_state_t    dtor_connect_state[MAX_DECTECTOR+1];
	unsigned char sync_code[3];
	uint8_t curDetector;
	uint8_t maxDetector;
	int device_activated[MAX_DECTECTOR+1];
	uint8_t inputChannel;
} sensor_data_t;

typedef struct
{
	bool ble_status;
	bool wifi_status;
	bool modem_status;
	bool mqtt_status;
	bool alarm_status;
	bool user_reset;
	gw_status_t status;
	bat_state_t bat_state;
	gw_connect_info_t connect_info;
	char fw_info[20];
} gateway_data_t;


typedef struct
{
	bool get_active_start;
	bool status_get_cer;
	char jobId[50];
	char mqtt_topic_pub[100];
	char mqtt_topic_pub_err[100];
	char mqtt_topic_sub_job[100];
	char mqtt_topic_pub_job_accept[100];
	char mqtt_topic_gw_add_sub[100];
	char mqtt_topic_gw_pub_dt[100];
	char mqtt_topic_gw_pub_err[100];
	char mqtt_topic_gw_sub_job[100];
	char mqtt_topic_gw_pub_accept[100];
	char client_id[33];
	char certificate_pem[3000];
	char certificate_pem_link[600];
	char private_pem[3000];
	char private_pem_link[600];
} mqtt_config_t;

char ota_message_link[500];

typedef enum {
	OTA_PENDING = 0,
	OTA_START,
	OTA_DONE,
} ota_state_t;
ota_state_t is_start_ota;

extern mqtt_config_t mqtt_config;
extern gateway_data_t gateway_data;
extern sensor_data_t sensor_data;

typedef struct {
	char mSsid[32];      /**< SSID of target AP. */
	char mPassword[64] ;  /**< Password of target AP. */
	char mPairToken[33];      /**< Pair token of target AP. */
} wifi_author_t;
extern wifi_author_t wifi_author;

extern xQueueHandle ble_receive_queue;
uint32_t usertimer_gettick(void);

extern bool isRegisterSensor;
#endif /* MAIN_COMMON_INTERFACE_H_ */
