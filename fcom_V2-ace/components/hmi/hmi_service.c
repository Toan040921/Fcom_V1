#include "hmi_service.h"
#include "../task/alarm_task.h"

#define TAG "HMI"

#define KEY_SCAN_TIME 20*1000
#define WIFI_DISCONNECT_BLINK_TIME   100*1000
#define BLE_CONNECT_BLINK_TIME       500*1000
#define WIFI_CONNECT_BLINK_TIME      1000*1000
#define MQTT_CONNECT_BLINK_TIME      2000*1000
#define GW_BUSY_BLINK_TIME           100*1000
#define GW_AVAILABLE_BLINK_TIME      3000*1000
#define GW_OTA_BLINK_TIME            200*1000
#define USER_RESET_TIME              3000*1000
#define SEND_MESSAGE_NORMAL_TIME     5000*1000
#define SEND_MESSAGE_ALARM_TIME      10000*1000

static void led_on(gpio_num_t gpio_num);
static void led_off(gpio_num_t gpio_num);
static void hmi_process(void *pvParameters);
static void button_handler();
static void led_init(void);
static void relay_init(void);
// first led
uint8_t state = 0;
uint64_t prev_blink_time = 0;
// second led
uint8_t gw_mode = 0;
uint64_t gw_busy_time = 0;
// key scan
uint64_t prev_key_scan= 0;
uint8_t button_state = 0;
uint64_t perivos_time_button = 0;
uint32_t prev_time_message_normal = 0;
uint32_t prev_time_message_alarm = 0;

uint8_t is_start_add_device = 0;

static void led_init(void)
{
    gpio_config_t io_conf;
    esp_err_t error;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.5
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // interrupt of rising edge
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    // configure GPIO with th
    error = gpio_config(&io_conf);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "error configuring outputs");
    }
    //interrupt of rising edge
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //bit mask of the pins, use GPIO0 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    error = gpio_config(&io_conf); //configure GPIO with the given settings

    if (error != ESP_OK)
    {
       ESP_LOGE(TAG, "error configuring inputs\n");
    }
    led_off(GPIO_DK_LED_1);
    led_off(GPIO_DK_LED_2);
}

static void hmi_process(void *pvParameters)
{
    uint8_t count_check_active = 1;
    struct dtor *head_local = NULL;
    led_init();
    relay_init();
    while (1)
    {
        if (is_start_ota != OTA_PENDING)
        {
            switch (gw_mode)
            {
            case 0:
                gw_busy_time = esp_timer_get_time();
                gw_mode = 1;
                led_on(GPIO_DK_LED_2);
                led_on(GPIO_DK_LED_1);
                break;
            case 1:
                if (esp_timer_get_time() - gw_busy_time > GW_OTA_BLINK_TIME)
                {
                    gw_busy_time = esp_timer_get_time();
                    led_off(GPIO_DK_LED_2);
                    led_off(GPIO_DK_LED_1);
                    gw_mode = 2;
                }
                break;
            case 2:
                if (esp_timer_get_time() - gw_busy_time > GW_OTA_BLINK_TIME)
                {
                    gw_mode = 0;
                }
                break;
            default:
                gw_mode = 0;
                break;
            }
        }

        if ((gateway_data.wifi_status == 0 && gateway_data.modem_status == 0) && (is_start_ota == OTA_PENDING))
        {
            switch (state)
            {
            case 0:
                prev_blink_time = esp_timer_get_time();
                state = 1;
                led_on(GPIO_DK_LED_1);
                break;
            case 1:
                if (esp_timer_get_time() - prev_blink_time > WIFI_DISCONNECT_BLINK_TIME)
                {
                    prev_blink_time = esp_timer_get_time();
                    led_off(GPIO_DK_LED_1);
                    state = 2;
                }
                break;
            case 2:
                if (esp_timer_get_time() - prev_blink_time > WIFI_DISCONNECT_BLINK_TIME)
                {
                    state = 0;
                }
                break;
            default:
                 state = 0;
                break;
            }
        }
        else if ((gateway_data.wifi_status == 1 || gateway_data.modem_status == 1) && (is_start_ota == OTA_PENDING))
        {
            switch (state)
            {
            case 0:
                prev_blink_time = esp_timer_get_time();
                state = 1;
                led_on(GPIO_DK_LED_1);
                break;
            case 1:
                if (esp_timer_get_time() - prev_blink_time > WIFI_CONNECT_BLINK_TIME)
                {
                    led_off(GPIO_DK_LED_1);
                    state = 2;
                    prev_blink_time = esp_timer_get_time();
                }
                break;
            case 2:
                if (esp_timer_get_time() - prev_blink_time > WIFI_CONNECT_BLINK_TIME)
                {
                    state = 0;
                }
                break;
            default:
                state = 0;
                break;
            }
        }
        if ((gateway_data.ble_status == 1) && (is_start_ota == OTA_PENDING))
        {
            switch (state)
            {
            case 0:
                prev_blink_time = esp_timer_get_time();
                state = 1;
                led_on(GPIO_DK_LED_1);
                break;
            case 1:
                if (esp_timer_get_time() - prev_blink_time > BLE_CONNECT_BLINK_TIME)
                {
                    prev_blink_time = esp_timer_get_time();
                    led_off(GPIO_DK_LED_1);
                    state = 2;
                }
                break;
            case 2:
                if (esp_timer_get_time() - prev_blink_time > BLE_CONNECT_BLINK_TIME)
                {
                    state = 0;
                }
                break;
            default:
                    state = 0;
                break;
            }
        }
        if ((gateway_data.mqtt_status == 1) && (is_start_ota == OTA_PENDING))
        {
            switch (state)
            {
            case 0:
                prev_blink_time = esp_timer_get_time();
                state = 1;
                led_on(GPIO_DK_LED_1);
                break;
            case 1:
                if (esp_timer_get_time() - prev_blink_time > MQTT_CONNECT_BLINK_TIME)
                {
                    prev_blink_time = esp_timer_get_time();
                    led_off(GPIO_DK_LED_1);
                    state = 2;
                }
                break;
            case 2:
                if (esp_timer_get_time() - prev_blink_time > MQTT_CONNECT_BLINK_TIME)
                {
                    state = 0;
                }
                break;
            default:
                state = 0;
                break;
            }
        }
        if ((gateway_data.status == ADD_SUB_DEVICE) && (is_start_ota == OTA_PENDING))
        {
            switch (gw_mode)
            {
            case 0:
                gw_busy_time = esp_timer_get_time();
                gw_mode = 1;
                led_on(GPIO_DK_LED_2);
                break;
            case 1:
                if (esp_timer_get_time() - gw_busy_time > GW_BUSY_BLINK_TIME)
                {
                    gw_busy_time = esp_timer_get_time();
                    led_off(GPIO_DK_LED_2);
                    gw_mode = 2;
                }
                break;
            case 2:
                if (esp_timer_get_time() - gw_busy_time > GW_BUSY_BLINK_TIME)
                {
                    gw_mode = 0;
                }
                break;
            default:
                gw_mode = 0;
                break;
            }
        }
        else if ((gateway_data.status == NORMAL) && (is_start_ota == OTA_PENDING))
        {
            switch (gw_mode)
            {
            case 0:
                gw_busy_time = esp_timer_get_time();
                gw_mode = 1;
                led_on(GPIO_DK_LED_2);
                break;
            case 1:
                if (esp_timer_get_time() - gw_busy_time > GW_AVAILABLE_BLINK_TIME)
                {
                    gw_busy_time = esp_timer_get_time();
                    led_off(GPIO_DK_LED_2);
                    gw_mode = 2;
                }
                break;
            case 2:
                if (esp_timer_get_time() - gw_busy_time > GW_AVAILABLE_BLINK_TIME)
                {
                    gw_mode = 0;
                }
                break;
            default:
                gw_mode = 0;
                break;
            }
        }

        if(esp_timer_get_time() - prev_key_scan > KEY_SCAN_TIME)
        {
            prev_key_scan = esp_timer_get_time();
            button_handler();
        }
		vTaskDelay(10 / portTICK_PERIOD_MS);


        switch (ssStateMachine)
        {
            case REGISTER_SUB:
            {
                if ((gpio_get_level(INPUT_FIRE_PIN) == 0) && (gpio_get_level(INPUT_FAULT_PIN) == 0))
                {
                    if (is_start_add_device == 0)
                    {
                        is_start_add_device = 1;
                        for (count_check_active = 1 ; count_check_active < (MAX_DECTECTOR + 1); count_check_active++)
                        {
                            if (sensor_data.device_activated[count_check_active] == 0)
                            {
                                sensor_data.inputChannel = count_check_active;
                                if(sensor_data.maxDetector < count_check_active)
                                    sensor_data.maxDetector = count_check_active;
                                sensor_data.dtor_sensor_state[count_check_active] = NORMAL_ST;
                                sensor_data.dtor_bat_state[count_check_active] = HIGH;
                                sensor_data.device_activated[count_check_active] = 1;
                                sensor_data.curDetector = count_check_active;
                                vTaskDelay(350 / portTICK_PERIOD_MS);
                                ssStateMachine = CONFIG_LISTEN_MODE;
                                ESP_LOGI(TAG, "Sensor state machine next step %d", ssStateMachine);
                                data_event_t cmd1 = SS_REGISTER_SERVER;
                                if (xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS) != pdPASS)
                                {
                                    ESP_LOGE(TAG, "Failed to send %s", __func__);
                                }
                                vTaskDelay(350 / portTICK_PERIOD_MS);
                                cmd1 = SS_FEEDBACK_FLAG;
                                if (xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS) != pdPASS)
                                {
                                    ESP_LOGE(TAG, "Failed to send %s", __func__);
                                }
                                head_local = get_head();
                                add_state(&head_local, sensor_data.curDetector, esp_timer_get_time());
                                break;
                            }
                        }
                    }
                }
            }
            break;
            case LISTEN_SUB_EVENT:
            {
                if (1 == is_start_add_device)
                {
                    if ((gpio_get_level(INPUT_FIRE_PIN) == 0) && (gpio_get_level(INPUT_FAULT_PIN) == 0))
                    {
                        // do nothing
                    }
                    else
                    {
                        is_start_add_device = 0;
                    }
                    break;
                }
                
                if ((gpio_get_level(INPUT_FIRE_PIN) == 1) && (gpio_get_level(INPUT_FAULT_PIN) == 0) && (sensor_data.inputChannel != 0))
                {
                    if (esp_timer_get_time() - prev_time_message_normal > SEND_MESSAGE_NORMAL_TIME)
                    {
                        prev_time_message_normal = esp_timer_get_time();
                        ESP_LOGI(TAG, "Device_Fire %d T", sensor_data.inputChannel);
                        sensor_data.dtor_sensor_state[sensor_data.inputChannel] = NORMAL_ST;
                        sensor_data.dtor_bat_state[sensor_data.inputChannel]=HIGH;
                        sensor_data.curDetector = sensor_data.inputChannel;
                        update_relay_alarm_state(sensor_data.curDetector);

                        bool alarm = false;
                        //for (int i = 1; i <= sensor_data.maxDetector; i++)
                        for (int i = 1; i <= 50; i++)
                        {
                            if (sensor_data.dtor_sensor_state[i] == ALARM_ST) alarm = true;
                        }
                        gateway_data.alarm_status = alarm;
                        data_event_t cmd1 = SS_FEEDBACK_FLAG;
                        if (xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS) != pdPASS)
                        {
                            ESP_LOGE(TAG, "Failed to send %s", __func__);
                        }
                        head_local = get_head();
                        add_state(&head_local, sensor_data.curDetector, esp_timer_get_time());
                    }
                }
                else if ((gpio_get_level(INPUT_FIRE_PIN) == 0) && (sensor_data.inputChannel != 0))
                {
                    if (esp_timer_get_time() - prev_time_message_alarm > SEND_MESSAGE_ALARM_TIME)
                    {
                        prev_time_message_alarm = esp_timer_get_time();
                        ESP_LOGI(TAG, "Device_Fire %d F", sensor_data.inputChannel);
                        sensor_data.dtor_sensor_state[sensor_data.inputChannel] = ALARM_ST;
                        sensor_data.curDetector = sensor_data.inputChannel;
                        update_relay_alarm_state(sensor_data.curDetector);
                        gateway_data.alarm_status = true;
                        data_event_t cmd1 = SS_FEEDBACK_FLAG;
                        if (xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS) != pdPASS)
                        {
                            ESP_LOGE(TAG, "Failed to send %s", __func__);
                        }
                        head_local = get_head();
                        add_state(&head_local, sensor_data.curDetector, esp_timer_get_time());
                    }
                }
                else if ((gpio_get_level(INPUT_FAULT_PIN) == 1) && (sensor_data.inputChannel != 0))
                {
                    if (esp_timer_get_time() - prev_time_message_alarm > SEND_MESSAGE_ALARM_TIME)
                    {
                        prev_time_message_alarm = esp_timer_get_time();
                        ESP_LOGI(TAG, "Device_Fire %d E", sensor_data.inputChannel);
                        sensor_data.dtor_sensor_state[sensor_data.inputChannel] = ERROR_ST;
                        sensor_data.curDetector = sensor_data.inputChannel;

                        bool alarm = false;
                        //for (int i = 1; i <= sensor_data.maxDetector; i++)
                        for (int i = 1; i <= 50; i++)
                        {
                            if (sensor_data.dtor_sensor_state[i] == ALARM_ST) alarm = true;
                        }
                        gateway_data.alarm_status = alarm;
                        data_event_t cmd1 = SS_FEEDBACK_FLAG;
                        if (xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS) != pdPASS)
                        {
                            ESP_LOGE(TAG, "Failed to send %s", __func__);
                        }
                        head_local = get_head();
                        add_state(&head_local, sensor_data.curDetector, esp_timer_get_time());
                    }
                }
                else
                {
                }
            }
            break;
            default:
                break;
        }
    }
}

void hmi_task(void)
{
    xTaskCreatePinnedToCore(hmi_process, "hmi_process", 2 * 1024, NULL, 5 | portPRIVILEGE_BIT, NULL, 1);
}


static void button_handler()
{
    if (gpio_get_level(GPIO_USER_BUTTON) == 0)
    {
        switch (button_state)
        {
        case 0:
            button_state = 1;
            perivos_time_button = esp_timer_get_time();
            break;
        case 1:
            if (esp_timer_get_time() - perivos_time_button > USER_RESET_TIME)
            {
                ESP_LOGI(TAG, "button pressed");
                flash_erase_all_partions();
                esp_restart();
            }
            break;
        default:
            break;
        }
    }
    else
        button_state = 0;
}

static void led_on(gpio_num_t gpio_num)
{
    // ESP_LOGI(TAG, "%s %d", __func__, gpio_num);
    gpio_set_level(gpio_num, 0);
}

static void led_off(gpio_num_t gpio_num)
{
    // ESP_LOGI(TAG, "%s %d", __func__, gpio_num);
    gpio_set_level(gpio_num, 1);
}

static void relay_init(void)
{
    gpio_config_t io_conf;
    esp_err_t error;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.5
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_RELAY_SEL;
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // interrupt of rising edge
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    // configure GPIO with th
    error = gpio_config(&io_conf);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "error configuring outputs");
    }
    gpio_set_level(GPIO_DK_RELAY, 0);
    gpio_set_level(GPIO_OUT_24V, 0);
}
void update_relay_alarm_state(uint8_t dev_alert)
{
    if(sensor_data.dtor_sensor_state[dev_alert] == ALARM_ST)
    {
        gpio_set_level(GPIO_DK_RELAY, 1);
        gpio_set_level(GPIO_OUT_24V, 1);
        ESP_LOGI(TAG, "Sensor %d %s on", dev_alert, __func__);
    }
    else
    {
        gpio_set_level(GPIO_DK_RELAY, 0);
        gpio_set_level(GPIO_OUT_24V, 0);
        ESP_LOGI(TAG, "Sensor %d %s off", dev_alert, __func__);
    }
}