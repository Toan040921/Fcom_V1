#include "ble_service.h"

#define OTA_UPDATE_TIME 500
TaskHandle_t TaskHandle_ota;

static const char *TAG = "BLE";
static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst heart_rate_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/* Service */
static const uint16_t GATTS_SERVICE_UUID_TEST = 0x00FF;
static const uint16_t GATTS_CHAR_UUID_TEST_A = 0xFF01;

static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t heart_measurement_ccc[2] = {0x00, 0x00};
static const uint8_t char_value[4] = {0x12, 0x34, 0x56, 0x98};

static bool is_ota_ntf = false;
static void ble_ota_noti_task(void * arg);
uint32_t ble_noti_next = 0;

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_db[HRS_IDX_NB] =
    {
        // Service Declaration
        [IDX_SVC] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID_TEST), (uint8_t *)&GATTS_SERVICE_UUID_TEST}},

        /* Characteristic Declaration */
        [IDX_CHAR_A] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

        /* Characteristic Value */
        [IDX_CHAR_VAL_A] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_TEST_A, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(char_value), (uint8_t *)char_value}},

        /* Client Characteristic Configuration Descriptor */
        [IDX_CHAR_CFG_A] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(heart_measurement_ccc), (uint8_t *)heart_measurement_ccc}},

};

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{

    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~ADV_CONFIG_FLAG);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        /* advertising start complete event to indicate advertising start successfully or failed */
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "advertising start failed");
        }
        else
        {
            ESP_LOGI(TAG, "advertising start successfully");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "Advertising stop failed");
        }
        else
        {
            ESP_LOGI(TAG, "Stop adv successfully\n");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                 param->update_conn_params.status,
                 param->update_conn_params.min_int,
                 param->update_conn_params.max_int,
                 param->update_conn_params.conn_int,
                 param->update_conn_params.latency,
                 param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

// void example_prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
// {
//     ESP_LOGI(TAG, "prepare write, handle = %d, value len = %d", param->write.handle, param->write.len);
//     esp_gatt_status_t status = ESP_GATT_OK;
//     if (prepare_write_env->prepare_buf == NULL)
//     {
//         prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
//         prepare_write_env->prepare_len = 0;
//         if (prepare_write_env->prepare_buf == NULL)
//         {
//             ESP_LOGE(TAG, "%s, Gatt_server prep no mem", __func__);
//             status = ESP_GATT_NO_RESOURCES;
//         }
//     }
//     else
//     {
//         if (param->write.offset > PREPARE_BUF_MAX_SIZE)
//         {
//             status = ESP_GATT_INVALID_OFFSET;
//         }
//         else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE)
//         {
//             status = ESP_GATT_INVALID_ATTR_LEN;
//         }
//     }
//     /*send response when param->write.need_rsp is true */
//     if (param->write.need_rsp)
//     {
//         esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
//         if (gatt_rsp != NULL)
//         {
//             gatt_rsp->attr_value.len = param->write.len;
//             gatt_rsp->attr_value.handle = param->write.handle;
//             gatt_rsp->attr_value.offset = param->write.offset;
//             gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
//             memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
//             esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
//             if (response_err != ESP_OK)
//             {
//                 ESP_LOGE(TAG, "Send response error");
//             }
//             free(gatt_rsp);
//         }
//         else
//         {
//             ESP_LOGE(TAG, "%s, malloc failed", __func__);
//         }
//     }
//     if (status != ESP_GATT_OK)
//     {
//         return;
//     }
//     memcpy(prepare_write_env->prepare_buf + param->write.offset,
//            param->write.value,
//            param->write.len);
//     prepare_write_env->prepare_len += param->write.len;
// }

// void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
// {
//     ESP_LOGI(TAG, "%s %d handle = %d, value len = %d", __func__, __LINE__, param->write.handle, param->write.len);
//     if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC && prepare_write_env->prepare_buf)
//     {
//         esp_log_buffer_hex(TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
//     }
//     else
//     {
//         ESP_LOGI(TAG, "ESP_GATT_PREP_WRITE_CANCEL");
//     }
//     if (prepare_write_env->prepare_buf)
//     {
//         free(prepare_write_env->prepare_buf);
//         prepare_write_env->prepare_buf = NULL;
//     }
//     prepare_write_env->prepare_len = 0;
// }

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    esp_ble_gatts_cb_param_t *p_data = (esp_ble_gatts_cb_param_t *)param;

    switch (event)
    {
    case ESP_GATTS_REG_EVT:
    {
        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
        if (set_dev_name_ret)
        {
            ESP_LOGE(TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }
        // config adv data
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret)
        {
            ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
        }
        adv_config_done |= ADV_CONFIG_FLAG;
        // config scan response data
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret)
        {
            ESP_LOGE(TAG, "config scan response data failed, error code = %x", ret);
        }
        adv_config_done |= SCAN_RSP_CONFIG_FLAG;
        esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, HRS_IDX_NB, SVC_INST_ID);
        if (create_attr_ret)
        {
            ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
        }
    }
    break;
    case ESP_GATTS_READ_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_READ_EVT");
        break;
    case ESP_GATTS_WRITE_EVT:
        // the data length of gattc write  must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
        ESP_LOGI(TAG, "GATT_WRITE_EVT, handle = %d, value len = %d", param->write.handle, param->write.len);
        if (p_data->write.is_prep == false)
        {
            if (ble_sm_handle_table[IDX_CHAR_CFG_A] == param->write.handle && param->write.len == 2){
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0001){
                    ESP_LOGI(TAG, "notify enable");
                    is_ota_ntf = true;
                } else if (descr_value == 0x0000){
                    ESP_LOGI(TAG, "notify disable");
                    is_ota_ntf = false;
                }
                else
                {
                        ESP_LOGE(TAG, "unknown descr value");
                        esp_log_buffer_hex(TAG, param->write.value, param->write.len);
                }
            }
            else
            {
                uint8_t *spp_cmd_buff = NULL;
                spp_cmd_buff = (uint8_t *)malloc(250 * sizeof(uint8_t));
                if (spp_cmd_buff == NULL)
                {
                    ESP_LOGE(TAG, "%s malloc failed\n", __func__);
                    break;
                }
                ESP_LOG_BUFFER_HEXDUMP(TAG, p_data->write.value, p_data->write.len, ESP_LOG_INFO);
                memset(spp_cmd_buff, 0x0, 250);
                memcpy(spp_cmd_buff, p_data->write.value, p_data->write.len);
                xQueueSend(ble_receive_queue, &spp_cmd_buff, 50 / portTICK_PERIOD_MS);
            }
        }
        break;
    case ESP_GATTS_EXEC_WRITE_EVT:
        // the length of gattc prepare write data must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
        ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_CONNECT_EVT:
        heart_rate_profile_tab[PROFILE_APP_IDX].conn_id = p_data->connect.conn_id;
        gateway_data.ble_status = true;
        ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d ble_status = %d", param->connect.conn_id, gateway_data.ble_status);
        esp_log_buffer_hex(TAG, param->connect.remote_bda, 6);
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20; // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10; // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;  // timeout = 400*10ms = 4000ms
        // start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        gateway_data.ble_status = false;
        is_ota_ntf = false;
        break;
    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
    {
        if (param->add_attr_tab.status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
        }
        else if (param->add_attr_tab.num_handle != HRS_IDX_NB)
        {
            ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)",
                     param->add_attr_tab.num_handle, HRS_IDX_NB);
        }
        else
        {
            ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d\n", param->add_attr_tab.num_handle);
            memcpy(ble_sm_handle_table, param->add_attr_tab.handles, sizeof(ble_sm_handle_table));
            esp_ble_gatts_start_service(ble_sm_handle_table[IDX_SVC]);
        }
        break;
    }
    case ESP_GATTS_STOP_EVT:
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    case ESP_GATTS_UNREG_EVT:
    case ESP_GATTS_DELETE_EVT:
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    // ESP_LOGI(TAG, "%s %d handle = %d, value len = %d ++ ", __func__, __LINE__, param->write.handle, param->write.len);
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
            heart_rate_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        }
        else
        {
            ESP_LOGE(TAG, "reg app failed, app_id %04x, status %d",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }
    }
    do
    {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++)
        {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == heart_rate_profile_tab[idx].gatts_if)
            {
                if (heart_rate_profile_tab[idx].gatts_cb)
                {
                    heart_rate_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
    // ESP_LOGI(TAG, "%s %d handle = %d, value len = %d -- ", __func__, __LINE__, param->write.handle, param->write.len);
}

void ble_init(void)
{
    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        // return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret)
    {
        ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        // return;
    }

    ESP_LOGI(TAG, "%s init bluetooth", __func__);
    ret = esp_bluedroid_init();
    if (ret)
    {
        ESP_LOGE(TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        // return;
    }

    ret = esp_bluedroid_enable();
    if (ret)
    {
        ESP_LOGE(TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        // return;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret)
    {
        ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
        // return;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret)
    {
        ESP_LOGE(TAG, "gap register error, error code = %x", ret);
        // return;
    }

    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    if (ret)
    {
        ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
        // return;
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret)
    {
        ESP_LOGE(TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }

    xTaskCreate(ble_ota_noti_task, "ble_ota_noti_task", 2048, NULL, 5 | portPRIVILEGE_BIT , &TaskHandle_ota);
}

void ble_denit(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "BLE denit");

    vTaskDelete(TaskHandle_ota); 

    ret = esp_bluedroid_disable();
    if (ret) {
        ESP_LOGE(TAG, "%s esp_bluedroid_disable failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_deinit();
    if (ret) {
        ESP_LOGE(TAG, "%s esp_bluedroid_deinit failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bt_controller_disable();
    if (ret) {
        ESP_LOGE(TAG, "%s esp_bt_controller_disable failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bt_controller_deinit();
    if (ret) {
        ESP_LOGE(TAG, "%s esp_bt_controller_deinit failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
}

void ble_ota_noti_task(void * arg)
{
    ESP_LOGI(TAG, "%s", __func__);

    while(1){
        // if(gateway_data.ble_status && is_ota_ntf)
        switch (is_start_ota)
        {
            case OTA_PENDING:
            {
                vTaskDelay(20 / portTICK_PERIOD_MS);
                ble_noti_next = usertimer_gettick();
            }
            break;
            case OTA_START:
            {
                if((gateway_data.ble_status == true) && (usertimer_gettick() - ble_noti_next >= OTA_UPDATE_TIME))
                {
                    char *ble_res_data;
                    ble_res_data = (char *)malloc(100 + 1);
                    sprintf(ble_res_data, "{\"id\": \"%s\", \"action\": \"ota\", \"status\": \"start\" }",
                                            (const char *)wifi_author.mPairToken);
                    //the size of notify_data[] need less than MTU size
                    ESP_LOGI(TAG, "News [%s]", ble_res_data);
                    esp_ble_gatts_send_indicate(heart_rate_profile_tab[PROFILE_APP_IDX].gatts_if, heart_rate_profile_tab[PROFILE_APP_IDX].conn_id, ble_sm_handle_table[IDX_CHAR_VAL_A],strlen(ble_res_data), (uint8_t *)ble_res_data, false);
                    free(ble_res_data);
                    ble_noti_next = usertimer_gettick();
                }
                vTaskDelay(20 / portTICK_PERIOD_MS);
            }
            break;
            case OTA_DONE:
            {
                if((gateway_data.ble_status == true) && (usertimer_gettick() - ble_noti_next >= OTA_UPDATE_TIME))
                {
                    char *ble_res_data;
                    ble_res_data = (char *)malloc(100 + 1);
                    sprintf(ble_res_data, "{\"id\": \"%s\", \"action\": \"ota\", \"status\": \"done\" }",
                                            (const char *)wifi_author.mPairToken);
                    //the size of notify_data[] need less than MTU size
                    ESP_LOGI(TAG, "News [%s]", ble_res_data);
                    esp_ble_gatts_send_indicate(heart_rate_profile_tab[PROFILE_APP_IDX].gatts_if, heart_rate_profile_tab[PROFILE_APP_IDX].conn_id, ble_sm_handle_table[IDX_CHAR_VAL_A],strlen(ble_res_data), (uint8_t *)ble_res_data, false);
                    free(ble_res_data);
                    ble_noti_next = usertimer_gettick();
                }
                vTaskDelay(20 / portTICK_PERIOD_MS);
            }

            break;
            default:
            // Do nothing
			vTaskDelay(20 / portTICK_PERIOD_MS);
            break;
        }
    }
    // vTaskDelete(NULL);
}