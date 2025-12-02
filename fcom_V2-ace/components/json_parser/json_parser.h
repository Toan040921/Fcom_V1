/*
 * json_parser.h
 *
 *  Created on: Nov 16, 2020
 *      Author: Yolo
 */

#ifndef MAIN_JSON_PARSER_JSON_PARSER_H_
#define MAIN_JSON_PARSER_JSON_PARSER_H_

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
typedef enum
{
	ENUM_NONE,
	ENUM_MESSAGE_SMART_CONFIG,
	ENUM_MESSAGE_CONTROL_IR,
	ENUM_MESSAGE_CONTROL_RAW,
	ENUM_MESSAGE_MODE_LEARN,
	ENUM_MESSAGE_MODE_TEST_DEVICE,
	ENUM_MESSAGE_NEW_IR_LEARN,
	ENUM_MESSAGE_RESPONSE
} enum_type_message;

/****************************************************************************/
/***         Exported global functions                                     ***/
/****************************************************************************/
bool json_parser_message_ble_data(const char *message, uint16_t length);

bool json_parser_gw_job(const char *message, uint16_t length);

bool json_parser_ss_job(const char *message, uint16_t length);

void json_packet_message_sensor_data(char *message_packet, uint8_t sub_id);

void json_packet_message_gateway_data(char *message_packet);

void json_packet_message_error(char *message_packet, uint8_t sensor_error);

void json_packet_message_sensor_error(char *message_packet, uint8_t sensor_error);

void json_packet_message_fb_ss_process(char *message_packet, uint8_t sub_id);

bool json_parser_certificate(const char *message, uint16_t length);

bool json_parser_ota_link(const char *message, uint16_t length);

void json_packet_message_job_requested_feedback(char *message_packet);
#endif /* MAIN_JSON_PARSER_JSON_PARSER_H_ */
