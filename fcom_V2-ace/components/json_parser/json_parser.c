/*
 * json_parser.c
 *
 *  Created on: Nov 16, 2020
 *      Author: Yolo
 */

/***********************************************************************************************************************
 * Pragma directive
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes <System Includes>
 ***********************************************************************************************************************/
#include "json_parser.h"
#include "cJson_lib/cJSON.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../user_driver/flash/user_flash.h"
#include "../uuid/uuid.h"

// #include "../Interface/Logger_File/logger_file.h"
/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/
#define TAG "JSON"

/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
sensor_data_t sensor_data;
gateway_data_t gateway_data;
wifi_author_t wifi_author;
/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/
#define CMD_GW_RECEIVE_ADD_SUB "add_sub_devices"
#define CMD_GW_RECEIVE_STOP_ADD_SUB "stop_add_sub_devices"
#define CMD_GW_RECEIVE_RM_SUB "remove_sub_devices"
#define CMD_GW_RECEIVE_UPDATE_FW "update_fw"
#define CMD_GW_RECEIVE_REPLACE_GW "replace_gw"
#define CMD_GW_REPORT_STATUS "gateway_status"
#define CMD_GW_REPORT_BAT_STATE "battery_state"
#define CMD_GW_REPORT_CONNECT_INFO "connection_info"
#define CMD_GW_REPORT_FIRMWARE_INFO "firmware_info"

#define CMD_NODE_REPORT_SS_STATE "fire_sensor_state"
#define CMD_NODE_REPORT_BAT_STATE "battery_state"
#define CMD_NODE_REPORT_CONNECT_STATE "node_online"
#define CMD_NODE_IP_NODE "ip_node"
#define CMD_NODE_PRODUCT_CODE "product_code"

/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/
xQueueHandle data_process;
master_rf_event_t ssStateMachine;
/***********************************************************************************************************************
 * Function Name:
 * Description  :
    name	:	null
    pass	:	null
    pair	:	AWA8U8UTEKOLT5BJDVRN7SZ5AI4YX6BX
 * Arguments    : none
 * Return Value : none
 ***********************************************************************************************************************/
bool json_parser_message_ble_data(const char *message, uint16_t length)
{
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
    bool status = true;
    cJSON *root = cJSON_ParseWithLength(message, length);
    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
    cJSON *pass = cJSON_GetObjectItemCaseSensitive(root, "pass");
    cJSON *pair = cJSON_GetObjectItemCaseSensitive(root, "pair");

    if (cJSON_IsString(name) && (name->valuestring != NULL))
    {
        ESP_LOGI(TAG, "name = %s", name->valuestring);
        sprintf(wifi_author.mSsid, "%.*s", strlen(name->valuestring),
                (char *)name->valuestring);
    }
    if (cJSON_IsString(pass) && (pass->valuestring != NULL))
    {
        ESP_LOGI(TAG, "pass = %s", pass->valuestring);
        sprintf(wifi_author.mPassword, "%.*s", strlen(pass->valuestring),
                (char *)pass->valuestring);
    }
    if (cJSON_IsString(pair) && (pair->valuestring != NULL))
    {
        ESP_LOGI(TAG, "pair = %s", pair->valuestring);
        sprintf(wifi_author.mPairToken, "%.*s", strlen(pair->valuestring),
                (char *)pair->valuestring);

    }
    // ESP_LOGI(TAG, "-------\n%s\n%s\n%s\n----",wifi_author.mSsid,wifi_author.mPassword,wifi_author.mPairToken);
    cJSON_Delete(root);
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
    return status;
}
/***********************************************************************************************************************
 * Function Name:
 * Description  :
 * Arguments    : none
 * Return Value : none
 ***********************************************************************************************************************/
bool json_parser_gw_job(const char *message, uint16_t length)
{
    ESP_LOGD(TAG, "Serialize.....");

		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
    bool status = true;
    cJSON *root2 = cJSON_ParseWithLength(message, length);
    cJSON *jobId = cJSON_GetObjectItem(root2, "request_id");
    cJSON *commands = NULL;
    cJSON *command = NULL;
    cJSON *sync_code_add = NULL;
    cJSON *sync_code_stop_add = NULL;
    cJSON *sync_code_rm = NULL;
    cJSON *update_fw = NULL;


    if (jobId)
    {
        commands = cJSON_GetObjectItem(root2, "commands");
        if (commands)
        {
            cJSON_ArrayForEach(command, commands)
            {

                sync_code_add = cJSON_GetObjectItemCaseSensitive(command, CMD_GW_RECEIVE_ADD_SUB);
                if (cJSON_IsString(sync_code_add) && (sync_code_add->valuestring != NULL))
                {
                    ESP_LOGI(TAG, "sync_code_add = %s", sync_code_add->valuestring);
                    char * token = strtok(sync_code_add->valuestring, ".");
                    int num = 0 ;
                    while( token != NULL ) {
                        sensor_data.sync_code[num] = (unsigned char) atoi(token);
                        num++;
                        token = strtok(NULL, ".");
                    }
                    data_event_t cmd1 = JOB_REQUESTED_FEEDBACK_FLAG;
                    if(xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS)!= pdPASS )
                    {
                        ESP_LOGE(TAG, "Failed to send %s", __func__);
                    }
                    rf_config_mode = true;
                    ssStateMachine = WAIT_ADD_SUB;
			        ESP_LOGI(TAG, "Sensor state machine next step %d", ssStateMachine);
                    // xEventGroupSetBits(sub_device_event_group, GW_ADD_SUB_DEVICE_BIT);
                    goto end;
                }

                sync_code_stop_add = cJSON_GetObjectItemCaseSensitive(command, CMD_GW_RECEIVE_STOP_ADD_SUB);
                if (cJSON_IsBool(sync_code_stop_add))
                {
                    ESP_LOGI(TAG, "sync_code_stop_add = True");
                    if(gateway_data.status != NORMAL)
                    gateway_data.status = NORMAL;
                    rf_config_mode = false;
                    ssStateMachine = CONFIG_LISTEN_MODE;
			        ESP_LOGI(TAG, "Sensor state machine next step %d", ssStateMachine);
                    data_event_t cmd1 = GW_FEEDBACK_FLAG;
                    if(xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS)!= pdPASS )
                    {
                        ESP_LOGE(TAG, "Failed to send %s", __func__);
                    }
                    goto end;
                }

                sync_code_rm = cJSON_GetObjectItemCaseSensitive(command, CMD_GW_RECEIVE_RM_SUB);
                if (cJSON_IsString(sync_code_rm) && (sync_code_rm->valuestring != NULL))
                {
                    ESP_LOGI(TAG, "sync_code_rm = %s", sync_code_rm->valuestring);
                    char * token = strtok(sync_code_rm->valuestring, ".");
                    int node = 0;
                    while( token != NULL ) {
                        node = (int)atoi(token);
                        token = strtok(NULL, ".");
                    }
                    flash_erase_sensor_data(node);
                    ESP_LOGI(TAG, "Delete device node %d infor in mem", node);
                    goto end;
                }
            }

            update_fw = cJSON_GetObjectItem(commands, CMD_GW_RECEIVE_UPDATE_FW);
            if (update_fw)
            {
                char tmp_content[50] = {0};
                sprintf(tmp_content, "%s", cJSON_GetObjectItem(update_fw, "version_number")->valuestring);
                ESP_LOGI(TAG, "version_number = %s", cJSON_GetObjectItem(update_fw, "version_number")->valuestring);
                sprintf(tmp_content, "%s", cJSON_GetObjectItem(update_fw, "version_name")->valuestring);
                ESP_LOGI(TAG, "version_name = %s", tmp_content);
                sprintf(ota_message_link, "%.*s", strlen(cJSON_GetObjectItem(update_fw, "url_firmware")->valuestring),
                        (char *)cJSON_GetObjectItem(update_fw, "url_firmware")->valuestring);
                sprintf(tmp_content, "%.*s", strlen(cJSON_GetObjectItem(update_fw, "hash_firmware")->valuestring),
                        (char *)cJSON_GetObjectItem(update_fw, "hash_firmware")->valuestring);
                ESP_LOGI(TAG, "url_firmware = %s", ota_message_link);
                ESP_LOGI(TAG, "hash_firmware = %s", tmp_content);
                is_start_ota = OTA_START;
                ESP_LOGI(TAG, "Found new OTA FIRMWARE !!!");
                data_event_t cmd1 = JOB_REQUESTED_OTA;
                if(xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS)!= pdPASS )
                {
                    ESP_LOGE(TAG, "Failed to send %s", __func__);
                }
                goto end;
            }
        }
    }
    else
    {
        ESP_LOGD(TAG, "thing_token have'nt jobID");
        status = false;
    }
end:
    // coppy to JobId
    if (status == true)
    {
        sprintf(mqtt_config.jobId, "%s", cJSON_GetObjectItem(root2, "request_id")->valuestring);
        ESP_LOGI(TAG, "mqtt_config.jobId = %s", mqtt_config.jobId);
    }
    ESP_LOGI(TAG, "end process 1");
    // cJSON_Delete(value);
    // free(operation);
    // cJSON_Delete(commands);
    // cJSON_Delete(jobId);
    cJSON_Delete(root2);
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
    ESP_LOGI(TAG, "end process 2");
    return status;
}

/***********************************************************************************************************************
* Function Name: GT001
* Description  :
{
  "request_id": "d6d7bef778564f848a548885c701bcb2",
  "sub_id": 2,
  "states":{
    "fire_sensor_state": 1,
    "battery_state": 0
    }
}
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/

void json_packet_message_sensor_data(char *message_packet, uint8_t sub_id)
{
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
    char *str1 = NULL;
    cJSON *root = NULL;
    cJSON *device_info = NULL;
    uuid_t request_id;
    char uu_str[UUID_STR_LEN];
    char *ss_state[] = {"normal", "alarm", "error","max"};
    char *ss_bat[] = {"low", "middle", "high"};
    bool *ss_connect[] = {false, true};
    char text[20];

    uuid_generate(request_id);
    // ESP_LOG_BUFFER_HEXDUMP(TAG, request_id, sizeof(uuid_t), ESP_LOG_INFO);
    uuid_unparse(request_id, uu_str);

    //sensor_data.dtor_bat_state[sub_id] = 2;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "request_id", cJSON_CreateString(uu_str));
    sprintf(text, "%03d.%03d.%03d.%03d", sensor_data.sync_code[0], sensor_data.sync_code[1], sensor_data.sync_code[2], sub_id);
    cJSON_AddItemToObject(root, "sub_id", cJSON_CreateString(text));
    cJSON_AddItemToObject(root, "states", device_info = cJSON_CreateObject());
    cJSON_AddStringToObject(device_info, CMD_NODE_REPORT_SS_STATE, ss_state[sensor_data.dtor_sensor_state[sub_id]]);
    cJSON_AddStringToObject(device_info, CMD_NODE_REPORT_BAT_STATE, ss_bat[sensor_data.dtor_bat_state[sub_id]]);
   // cJSON_AddBoolToObject(device_info,CMD_NODE_REPORT_CONNECT_STATE,ss_connect[sensor_data.dtor_connect_state[sub_id]]);
     cJSON_AddBoolToObject(device_info,CMD_NODE_REPORT_CONNECT_STATE,ss_connect[1]);
    str1 = cJSON_PrintUnformatted(root);
    strcpy(message_packet, str1);
    free(str1);
    cJSON_Delete(root);
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
}

/***********************************************************************************************************************
* Function Name: T001 GW -> Server
* Description  :
"{
  ""request_id"": <request_id>,
  ""states"":
       {
          gateway_status: 1,
          battery_state: 1,
          connection_info: 1,
       }
}"
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/
void json_packet_message_gateway_data(char *message_packet)
{
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
    char *str1 = NULL;
    cJSON *root = NULL;
    cJSON *device_info = NULL;
    uuid_t request_id;
    char uu_str[UUID_STR_LEN];
    char *gw_status[] = {"normal", "adding_sub_devices", "error"};
    char *gw_bat[] = {"low", "middle", "high"};
    char *gw_connect_info[] = {"ethernet", "wifi", "3g"};

    uuid_generate(request_id);
    // ESP_LOG_BUFFER_HEXDUMP(TAG, request_id, sizeof(uuid_t), ESP_LOG_INFO);
    uuid_unparse(request_id, uu_str);

    gateway_data.bat_state = HIGH;
    // gateway_data.connect_info = WIFI;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "request_id", cJSON_CreateString(uu_str));
    cJSON_AddItemToObject(root, "states", device_info = cJSON_CreateObject());
    cJSON_AddStringToObject(device_info, CMD_GW_REPORT_STATUS, gw_status[gateway_data.status]);
    cJSON_AddStringToObject(device_info, CMD_GW_REPORT_BAT_STATE, gw_bat[gateway_data.bat_state]);
    cJSON_AddStringToObject(device_info, CMD_GW_REPORT_CONNECT_INFO, gw_connect_info[gateway_data.connect_info]);
    cJSON_AddStringToObject(device_info, CMD_GW_REPORT_FIRMWARE_INFO, gateway_data.fw_info);

    str1 = cJSON_PrintUnformatted(root);
    strcpy(message_packet, str1);
    free(str1);
    cJSON_Delete(root);
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
}

/***********************************************************************************************************************
* Function Name: T002 GW -> Server
* Description  :
"{
  ""request_id"": <request_id>,
  ""error_code"":<error_code>,
  ""message"": <string>
}"
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/
void json_packet_message_error(char *message_packet, uint8_t sensor_error)
{
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);

    char *str1 = NULL;
    cJSON *root = NULL;
    uuid_t request_id;
    char uu_str[UUID_STR_LEN];
    char *error_code = "E0001"; //dump value
    char *message;
    message = (char *)malloc(30*sizeof(char));
    sprintf(message, "Sensor %d lost connection", sensor_error);

    uuid_generate(request_id);
    // ESP_LOG_BUFFER_HEXDUMP(TAG, request_id, sizeof(uuid_t), ESP_LOG_INFO);
    uuid_unparse(request_id, uu_str);

    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "request_id", cJSON_CreateString(uu_str));
    cJSON_AddItemToObject(root, "error_code", cJSON_CreateString(error_code));
    cJSON_AddItemToObject(root, "message", cJSON_CreateString(message));

    str1 = cJSON_PrintUnformatted(root);
    strcpy(message_packet, str1);
    free(str1);
    free(message);
    cJSON_Delete(root);
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
  
}

/***********************************************************************************************************************
* Function Name: GT002 SS -> Server
* Description  :
"{
  ""request_id"": <request_id>,
  ""sub_id"": <request_id>,
  ""error_code"":<error_code>,
  ""message"": <string>
}"
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/
void json_packet_message_sensor_error(char *message_packet, uint8_t sub_id)
{
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
        /*
    char *str1 = NULL;
    cJSON *root = NULL;
    uuid_t request_id;
    char uu_str[UUID_STR_LEN];
    char *error_code = "E0001"; //dump value
    char *message;
    char text[20];

    message = (char *)malloc(30*sizeof(char));
    sprintf(message, "Sensor %d lost connection", sensor_error);

    uuid_generate(request_id);
    // ESP_LOG_BUFFER_HEXDUMP(TAG, request_id, sizeof(uuid_t), ESP_LOG_INFO);
    uuid_unparse(request_id, uu_str);

    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "request_id", cJSON_CreateString(uu_str));
    sprintf(text, "%03d.%03d.%03d.%03d", sensor_data.sync_code[0], sensor_data.sync_code[1], sensor_data.sync_code[2], sensor_error);
    cJSON_AddItemToObject(root, "sub_id", cJSON_CreateString(text));
    cJSON_AddItemToObject(root, "error_code", cJSON_CreateString(error_code));
    cJSON_AddItemToObject(root, "message", cJSON_CreateString(message));

    str1 = cJSON_PrintUnformatted(root);
    strcpy(message_packet, str1);
    free(str1);
    free(message);
    cJSON_Delete(root);
		// ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);*/

    char *str1 = NULL;
    cJSON *root = NULL;
    cJSON *device_info = NULL;
    uuid_t request_id;
    char uu_str[UUID_STR_LEN];
    char *ss_state[] = {"normal", "alarm", "error","max"};
    char *ss_bat[] = {"low", "middle", "high"};
    bool *ss_connect[] = {false, true};
    char text[20];
 
    uuid_generate(request_id);
    // ESP_LOG_BUFFER_HEXDUMP(TAG, request_id, sizeof(uuid_t), ESP_LOG_INFO);
    uuid_unparse(request_id, uu_str);

    //sensor_data.dtor_bat_state[sub_id] = 2;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "request_id", cJSON_CreateString(uu_str));
    sprintf(text, "%03d.%03d.%03d.%03d", sensor_data.sync_code[0], sensor_data.sync_code[1], sensor_data.sync_code[2], sub_id);
    cJSON_AddItemToObject(root, "sub_id", cJSON_CreateString(text));
    cJSON_AddItemToObject(root, "states", device_info = cJSON_CreateObject());
    cJSON_AddStringToObject(device_info, CMD_NODE_REPORT_SS_STATE, ss_state[0]);
    cJSON_AddStringToObject(device_info, CMD_NODE_REPORT_BAT_STATE, ss_bat[2]);
    cJSON_AddBoolToObject(device_info,CMD_NODE_REPORT_CONNECT_STATE,ss_connect[0]);
   //cJSON_AddBoolToObject(device_info,CMD_NODE_REPORT_CONNECT_STATE,false);
    str1 = cJSON_PrintUnformatted(root);
    strcpy(message_packet, str1);
    free(str1);
    cJSON_Delete(root);
}

/***********************************************************************************************************************
* Function Name: GW001 SS regis done-> GW -> Server
* Description  :
"{
   ""request_id"": """",
   ""action"": <action>
   ""devices: [
        {
            ip_node:"1";
            product_code:"1";
        }
    ]
}"
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/
void json_packet_message_fb_ss_process(char *message_packet, uint8_t sub_id)
{
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
    char *str1 = NULL;
    cJSON *root = NULL;
    cJSON *device_info = NULL;
    cJSON *device_array = NULL;
    uuid_t request_id;
    char uu_str[UUID_STR_LEN];
    char text[20];
    int nums = 0;

    uuid_generate(request_id);
    // ESP_LOG_BUFFER_HEXDUMP(TAG, request_id, sizeof(uuid_t), ESP_LOG_INFO);
    uuid_unparse(request_id, uu_str);

    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "request_id", cJSON_CreateString(uu_str));
    cJSON_AddItemToObject(root, "action", cJSON_CreateString("add"));
    cJSON_AddItemToObject(root, "devices", device_array = cJSON_CreateArray());
    for( nums = 0; nums < 1; nums++ ) // add 1 device 1 array
    {
        device_info = cJSON_CreateObject();
        cJSON_AddItemToArray(device_array, device_info);
        sprintf(text, "%03d.%03d.%03d.%03d", sensor_data.sync_code[0], sensor_data.sync_code[1], sensor_data.sync_code[2], sensor_data.curDetector);
        cJSON_AddStringToObject(device_info, CMD_NODE_IP_NODE, text);
        cJSON_AddStringToObject(device_info, CMD_NODE_PRODUCT_CODE, "sensor001");
    }

    str1 = cJSON_PrintUnformatted(root);
    strcpy(message_packet, str1);
    free(str1);
    cJSON_Delete(root);
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
}
/***********************************************************************************************************************
* Function Name: C002 GW -> Server
* Description  :
"{
  ""request_id"": <request id>,
  ""identifier"": <command_identifier>
  ""status: <status>,
  ""error_message"": """"
}"
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/
// void json_packet_message_job_requested_feedback(char *message_packet)
// {
//     char *str1 = NULL;
//     cJSON *root = NULL;
//     uuid_t request_id;
//     char uu_str[UUID_STR_LEN];
//     int status = 1; //dump value
//     char *message = "success";//dump value

//     uuid_generate(request_id);
//     // ESP_LOG_BUFFER_HEXDUMP(TAG, request_id, sizeof(uuid_t), ESP_LOG_INFO);
//     uuid_unparse(request_id, uu_str);

//     root = cJSON_CreateObject();
//     cJSON_AddItemToObject(root, "request_id", cJSON_CreateString(uu_str));
//     cJSON_AddItemToObject(root, "identifier", cJSON_CreateString(&mqtt_config.jobId[0]));
//     cJSON_AddItemToObject(root, "status", cJSON_CreateNumber(status));
//     cJSON_AddItemToObject(root, "error_message", cJSON_CreateString(message));

//     str1 = cJSON_PrintUnformatted(root);
//     strcpy(message_packet, str1);
//     free(str1);
//     cJSON_Delete(root);
// }
/***********************************************************************************************************************
* Function Name: C001 GW -> Server
* Description  :
"{
  ""request_id"": <request id>,
  ""commands"":
      {
          <device command>
      }
}"
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/
void json_packet_message_job_requested_feedback(char *message_packet)
{
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
    char *str1 = NULL;
    cJSON *root = NULL;
    uuid_t request_id;
    char uu_str[UUID_STR_LEN];
    cJSON *device_command = NULL;

    uuid_generate(request_id);
    // ESP_LOG_BUFFER_HEXDUMP(TAG, request_id, sizeof(uuid_t), ESP_LOG_INFO);
    uuid_unparse(request_id, uu_str);

    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "request_id", cJSON_CreateString(uu_str));
    cJSON_AddItemToObject(root, "commands", device_command = cJSON_CreateObject());

    cJSON_AddBoolToObject(device_command, CMD_GW_RECEIVE_ADD_SUB, true);

    str1 = cJSON_PrintUnformatted(root);
    strcpy(message_packet, str1);
    free(str1);
    cJSON_Delete(root);
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
}
/***********************************************************************************************************************
* Function Name: GC001 SS <- GW <- cmd
* Description  :
"{
  ""request_id"": <request id>,
  ""sub_id"": <sub_id>,
  ""commands"":
      {
          <device instruction>
      }
}"
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/
bool json_parser_ss_job(const char *message, uint16_t length)
{
    ESP_LOGD(TAG, "Serialize.....");
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
    bool status = true;
    cJSON *root2 = cJSON_ParseWithLength(message, length);
    cJSON *jobId = cJSON_GetObjectItem(root2, "request_id");
    cJSON *commands;
    cJSON *sub_id;
    uint8_t ss_node_id = 0;


    if (jobId)
    {
        sub_id = cJSON_GetObjectItem(root2, "sub_id");
        if (cJSON_IsString(sub_id) && (sub_id->valuestring != NULL))
        {
            ESP_LOGI(TAG, "sub_id = %s", sub_id->valuestring);
            char * token = strtok(sub_id->valuestring, ".");
            int num = 0 ;
            while( token != NULL ) {
                if(num > 0 && num < 3)
                {
                    sensor_data.sync_code[num] = (unsigned char) atoi(token);
                }
                else
                {
                    ss_node_id = (unsigned char) atoi(token); // temp store
                    ESP_LOGI(TAG, "ss_node_id = %d", ss_node_id);
                }
                token = strtok(NULL, ".");
                num++;
            }
        }
        commands = cJSON_GetObjectItem(root2, "commands");
        if (commands)
        {
            // Empty device instruction
            status = true;
            goto end_ss;
        }
    }
    else
    {
        ESP_LOGD(TAG, "thing_token have'nt jobID");
        status = false;
    }

end_ss:
    // coppy to JobId
    if (status == true)
    {
        sprintf(mqtt_config.jobId, "%s", cJSON_GetObjectItem(root2, "request_id")->valuestring);
        ESP_LOGI(TAG, "mqtt_config.jobId = %s", mqtt_config.jobId);
    }
    ESP_LOGI(TAG, "end process 1");
    // cJSON_Delete(value);
    // free(operation);
    // cJSON_Delete(commands);
    // cJSON_Delete(jobId);
    cJSON_Delete(root2);
    ESP_LOGI(TAG, "end process 2");
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
    return status;
}

bool json_parser_certificate(const char *message, uint16_t length)
{
    bool status = false;
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
    cJSON *root2 = cJSON_ParseWithLength(message, length);
    cJSON *data;
    cJSON *success = cJSON_GetObjectItem(root2, "success");
	char sha256_buf[20];

    if (success)
    {
        int value_success = cJSON_GetObjectItem(root2, "success")->valueint;
        if (value_success == true)
        {
            data = cJSON_GetObjectItem(root2, "data");
            if (data)
            {
                sprintf(mqtt_config.client_id, "%.*s", strlen(cJSON_GetObjectItem(data, "client_id")->valuestring),
                        (char *)cJSON_GetObjectItem(data, "client_id")->valuestring);

                sprintf(mqtt_config.certificate_pem_link, "%.*s", strlen(cJSON_GetObjectItem(data, "certificate_pem")->valuestring),
                        (char *)cJSON_GetObjectItem(data, "certificate_pem")->valuestring);

                sprintf(mqtt_config.private_pem_link, "%.*s", strlen(cJSON_GetObjectItem(data, "private_pem")->valuestring),
                        (char *)cJSON_GetObjectItem(data, "private_pem")->valuestring);

                sprintf(mqtt_config.private_pem_link, "%.*s", strlen(cJSON_GetObjectItem(data, "private_pem")->valuestring),
                        (char *)cJSON_GetObjectItem(data, "private_pem")->valuestring);

                status = cJSON_GetObjectItem(data, "new_firmware")->valueint;
                if(status == true)
                {
                    is_start_ota = OTA_START;
                    ESP_LOGI(TAG, "Found new OTA FIRMWARE !!!");
                }
                else
                {
                    status = true;
                    ESP_LOGI(TAG, "OTA FIRMWARE IS LATEST OR NOT FOUND!!!");
                }

                sprintf(ota_message_link, "%.*s", strlen(cJSON_GetObjectItem(data, "url_firmware")->valuestring),
                        (char *)cJSON_GetObjectItem(data, "url_firmware")->valuestring);

                sprintf(sha256_buf, "%.*s", strlen(cJSON_GetObjectItem(data, "hash_firmware")->valuestring),
                        (char *)cJSON_GetObjectItem(data, "hash_firmware")->valuestring);

                ESP_LOGI(TAG, "client_id = %s", mqtt_config.client_id);
                ESP_LOGI(TAG, "certificate_pem_link = %s", mqtt_config.certificate_pem_link);
                ESP_LOGI(TAG, "private_pem_link = %s", mqtt_config.private_pem_link);
                ESP_LOGI(TAG, "url_firmware = %s", ota_message_link);
                ESP_LOGI(TAG, "hash_firmware = %s", sha256_buf);
            }
        }
        else
        {
            status = false;
        }
    }
    cJSON_Delete(root2);
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
    return status;
}

bool json_parser_ota_link(const char *message, uint16_t length)
{
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
    bool status = false;
    cJSON *data;
    int new_firmware = 0;
    cJSON *root2 = cJSON_ParseWithLength(message, length);
    cJSON *success = cJSON_GetObjectItem(root2, "success");
    char hash_firware[50];
    ESP_LOGI(TAG, "message = %.*s", length, message);
    if (success)
    {
        int value_success = cJSON_GetObjectItem(root2, "success")->valueint;
        if (value_success == true)
        {
            data = cJSON_GetObjectItem(root2, "data");
            new_firmware = cJSON_GetObjectItem(data, "new_firmware")->valueint;
            if (new_firmware == true)
            {
                if ((cJSON_GetObjectItem(data, "url_firmware")) != NULL)
                {
                    if (cJSON_IsString(cJSON_GetObjectItem(data, "url_firmware")))
                    {
                        sprintf(ota_message_link, "%.*s", strlen(cJSON_GetObjectItem(data, "url_firmware")->valuestring),
                                (char *)cJSON_GetObjectItem(data, "url_firmware")->valuestring);
                        ESP_LOGI(TAG, "ota_message_link = %s", ota_message_link);
                        status = true;
                    }
                }
                if ((cJSON_GetObjectItem(data, "hash_firmware")) != NULL)
                {
                    if (cJSON_IsString(cJSON_GetObjectItem(data, "hash_firmware")))
                    {
                        sprintf(hash_firware, "%.*s", strlen(cJSON_GetObjectItem(data, "hash_firmware")->valuestring),
                                (char *)cJSON_GetObjectItem(data, "hash_firmware")->valuestring);
                        ESP_LOGI(TAG, "hash_firware = %s", hash_firware);
                        status = true;
                    }
                }
            }
            else
            {
                ESP_LOGI(TAG, "new_firmware false");
                status = false;
            }
        }
        else
        {
            status = false;
            ESP_LOGI(TAG, "value_success false");
        }
    }
    else
        ESP_LOGI(TAG, "success false");
    cJSON_Delete(root2);
		//ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
    return status;
}
/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/
