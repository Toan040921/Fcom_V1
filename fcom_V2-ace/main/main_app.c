#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include <string.h>
#include "esp_wifi.h"
#include "main_app.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "user_timer.h"
#include "hmi_service.h"
#include "ble_service.h"
#include "../common_interface.h"
#include "../components/user_driver/flash/user_flash.h"

#include "../components/task/plan_task.h"
#include "../components/task/ota_task.h"

#include "../task/uart_task.h"
#include "../task/sim7000e_task.h"
#include "../user_driver/fire_detector/fire_detector.h"
#include "uuid.h"

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/spi_master.h"
#include "json_parser.h"

#include "esp_netif_ppp.h"
#include "mqtt_client.h"
#include "esp_modem.h"
#include "esp_modem_netif.h"
#include "sim800.h"
#include "bg96.h"
#include "sim7600.h"

#include "../task/mqtt_task.h"
#include <rom/rtc.h>
#include "../components/esp_netif/private_include/esp_netif_private.h"

#include "../task/alarm_task.h"
static const char *TAG = "MAIN";

#define EXAMPLE_ESP_MAXIMUM_RETRY 30
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
SemaphoreHandle_t xMainMutex;
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define PAIR_KEY_BIT BIT2

// GPIO SIM
#define CONFIG_EXAMPLE_GPIO_MODEM_PWRKEY 33
#define CONFIG_EXAMPLE_GPIO_MODEM_RESET 32
#define CONFIG_EXAMPLE_GPIO_MODEM_STATUS 34

// For PPPOS
static EventGroupHandle_t event_group = NULL;
static const int CONNECT_BIT = BIT0;
static const int STOP_BIT = BIT1;
static const int DISCONNECT_BIT = BIT2;

// SIM
modem_dte_t *dte = NULL;
modem_dce_t *dce = NULL;
void *modem_netif_adapter = NULL;
esp_netif_t *esp_netif = NULL;
static bool modem_is_disconnected = false;

// WiFi
static bool wifi_ssid_exist = false;
static int s_retry_num = 0;
// Pair key
static bool has_pair_key = false;

esp_event_handler_instance_t instance_any_id;
esp_event_handler_instance_t instance_got_ip;

// Network instance
static esp_netif_t *ifwifi = 0;
static esp_netif_t *ifmodem = 0;

xQueueHandle data_process;

void sim_denit(void);

static void update_network_type()
{
    ESP_LOGI(TAG, "Update newtwork type");
    data_event_t cmd1 =  GW_FEEDBACK_FLAG;
    if(xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS)!= pdPASS )
    {
        ESP_LOGE(TAG, "Failed to send %s", __func__);
    }
}

static void get_network_instane()
{
    esp_netif_t *ifscan = esp_netif_next (NULL);
    char ifdesc[7];
    ifdesc[6] = 0;  // Ensure null terminated string

    // Get wifi/modem interface instance
    while (ifscan != NULL)
    {
        esp_netif_get_netif_impl_name (ifscan, ifdesc);
        if (ifdesc[0] == 's' && ifdesc[1] == 't')
        {
            ifwifi = ifscan;
            ESP_LOGI(TAG, "Got Wifi interface instance");
        }
        else if (ifdesc[0] == 'p' && ifdesc[1] == 'p')
        {
            ifmodem = ifscan;
            ESP_LOGI(TAG, "Got Modem interface instance");
        }

        ESP_LOGI(TAG, "IF NAME: %s, is_up: %d, pri_num %d", ifdesc, esp_netif_is_netif_up(ifscan), esp_netif_get_route_prio(ifscan));
        ifscan = esp_netif_next (ifscan);
    }
}

static void sim_power_reset()
{
    // GPIO Init
    gpio_pad_select_gpio(CONFIG_EXAMPLE_GPIO_MODEM_PWRKEY);
    gpio_set_direction(CONFIG_EXAMPLE_GPIO_MODEM_PWRKEY, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_EXAMPLE_GPIO_MODEM_PWRKEY, 0);

    gpio_pad_select_gpio(CONFIG_EXAMPLE_GPIO_MODEM_RESET);
    gpio_set_direction(CONFIG_EXAMPLE_GPIO_MODEM_RESET, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_EXAMPLE_GPIO_MODEM_RESET, 0);

    gpio_pad_select_gpio(CONFIG_EXAMPLE_GPIO_MODEM_STATUS);
    gpio_set_direction(CONFIG_EXAMPLE_GPIO_MODEM_STATUS, GPIO_MODE_INPUT);
    /////

    bool status = false;
    int inc = 0;

    // Reset on module (...ms on NRESET pin); 500ms sim7600; 2.5sec simA7670C
    ESP_LOGI(TAG, "Module SIM will be reset");
    gpio_set_level(CONFIG_EXAMPLE_GPIO_MODEM_RESET, 1);
    vTaskDelay(2500 / portTICK_PERIOD_MS);
    gpio_set_level(CONFIG_EXAMPLE_GPIO_MODEM_RESET, 0);
    ESP_LOGI(TAG, "Pulse on RESET is done");

    vTaskDelay(1100 / portTICK_PERIOD_MS);
    gpio_set_level(CONFIG_EXAMPLE_GPIO_MODEM_PWRKEY, 0);

    // Wait time of reboot (6sec) sim7600; (12sec) simA7670C
    for (int i = 0; i < 12; i++)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        //ESP_LOGI(TAG, ".");
    }

    do
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        inc += 1;
        status = gpio_get_level(CONFIG_EXAMPLE_GPIO_MODEM_STATUS) > 0;

        // Unbounce input
        if(status) {
            vTaskDelay(30 / portTICK_PERIOD_MS);
            status = gpio_get_level(CONFIG_EXAMPLE_GPIO_MODEM_STATUS) > 0;
        }
        ESP_LOGI(TAG, "STATUS is %d", status);
    } while (!(status==true || inc>20));

    if (!status) {
        ESP_LOGI(TAG, "Failed to opening module SIM (STATUS pin not enable)");
        return;
    }
    ESP_LOGI(TAG, "STATUS of module SIM is OK");

}

void network_switch_task(void *arg)
{
    static int mqtt_count = 0;
    gateway_data.connect_info = WIFI;

    sim_power_reset();

    ESP_LOGI(TAG, "Waiting the Pair key.....");
    while(!has_pair_key)
    {
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }

    if (mqtt_config.get_active_start)
    {
        ESP_LOGI(TAG, "Gateway has ativated, BLE expires in 1 minutes");
        for (int time_cnt=0; time_cnt<60; time_cnt++)
        {
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
        ble_denit();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Time's up - Disable BLE, Enable SIM");

    }
    else
    {
        ble_denit();
        vTaskDelay(20000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Gateway has activated first time - Disable BLE, enable SIM");
    }

    main_sim_init();

    get_network_instane();

    while (1)
    {
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        // Check mqtt disconected, every 10 sencond will reconnect mqtt
        if (gateway_data.mqtt_status == false && is_start_ota == false)
        {
            mqtt_count++;
            ESP_LOGW(TAG, "MQTT connection failed %d", mqtt_count);
        }
        else
        {
            mqtt_count = 0;
            if (esp_netif_is_netif_up(ifwifi) == false && wifi_ssid_exist && s_retry_num == 0){
                esp_netif_up(ifwifi);
                esp_wifi_disconnect();
                vTaskDelay(500 / portTICK_PERIOD_MS);
                esp_wifi_connect();
                ESP_LOGI (TAG, "WiFi interface is up");
            }
        }

        if (modem_is_disconnected && (gateway_data.mqtt_status || mqtt_count > 6))
        {
            modem_is_disconnected = false;
            sim_denit();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            sim_power_reset();
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            main_sim_init();
        }

        if (mqtt_count >= 3 && gateway_data.modem_status && gateway_data.wifi_status) // 2 minitue
        {
            sim_denit();
            gateway_data.modem_status = false;
            gateway_data.connect_info = WIFI;
            modem_is_disconnected = false; // no re-init SIM
            update_network_type();
            
            ESP_LOGI (TAG, "SIM has no internet, turn off SIM");
        }

        // if (mqtt_count >= 6 && gateway_data.wifi_status && gateway_data.modem_status) // 1 minitue
        // {
        //     esp_netif_down(ifwifi);
        //     ESP_LOGI (TAG, "WiFi has no internet, WiFi interface is down");
        // }

        if (mqtt_count >= 60 || esp_get_free_heap_size() <= 8000) // 10 minitue
        {
            ESP_LOGW(TAG, "Force esp restart due to: mqtt_count %d, heapsize: %d", mqtt_count, esp_get_free_heap_size());
            esp_restart();
        }

        //ESP_LOGW(TAG, "Free memory: %d bytes  %d ++", esp_get_free_heap_size(), __LINE__);
    }
}

void ble_config_wifi_task(void *arg)
{
    char *cmd_id;

    for (;;)
    {
        // vTaskDelay(50 / portTICK_PERIOD_MS);
        if (xQueueReceive(ble_receive_queue, &cmd_id, portMAX_DELAY))
        {
            // ESP_LOGI(TAG, "%s",cmd_id);
            json_parser_message_ble_data((const char *)cmd_id, strlen(cmd_id));

            if (strlen((const char *)wifi_author.mPairToken))
            {
                flash_save_pair_key(wifi_author);
                flash_read_pair_key();
                has_pair_key = true;
                ESP_LOGI(TAG, "Received Pair_key:%s", (const char *)wifi_author.mPairToken);
                //goto ble_end;
            }

            esp_wifi_disconnect();
            esp_wifi_stop();

            wifi_config_t wifi_config = {
                .sta = {
                    /* Setting a password implies station will connect to all security modes including WEP/WPA.
                     * However these modes are deprecated and not advisable to be used. Incase your Access point
                     * doesn't support WPA2, these mode can be enabled by commenting below line */
                    .threshold.authmode = WIFI_AUTH_WPA2_PSK,

                    .pmf_cfg = {
                        .capable = true,
                        .required = false},
                },
            };
            memcpy(wifi_config.sta.ssid, wifi_author.mSsid, sizeof(wifi_author.mSsid));
            memcpy(wifi_config.sta.password, wifi_author.mPassword, sizeof(wifi_author.mPassword));

            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
            wifi_config_t wifi_cfg;
            if (esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg) == ESP_OK)
            {
                if (strlen((const char *)wifi_cfg.sta.ssid))
                {
                    wifi_ssid_exist = true;
                    ESP_LOGI(TAG, "Reconfig ssid %s", (const char *)wifi_cfg.sta.ssid);
                    ESP_LOGI(TAG, "Reconfig password %s", (const char *)wifi_cfg.sta.password);
                }
                else
                {
                    wifi_ssid_exist = false;
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                    ESP_LOGI(TAG, "Skip wifi setup");
                    goto ble_end;
                }
            }

            ESP_ERROR_CHECK(esp_wifi_start());

         ble_end:
            free(cmd_id);
        }
    }
    vTaskDelete(NULL);
}

void network_init(void)
{
    // Initialize TCP/IP network interface (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

}
typedef enum esp_netif_action {
    ESP_NETIF_UNDEF,
    ESP_NETIF_STARTED,
    ESP_NETIF_STOPPED,
} esp_netif_action_t;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Event base %s event_id %d", event_base, event_id);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGE(TAG, "Disconnect reason : %d", disconnected->reason);
        gateway_data.wifi_status = false;
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);

        ESP_LOGW(TAG, "retry %d to connect to server", s_retry_num);
        vTaskDelay(1500 / portTICK_PERIOD_MS);
        esp_wifi_connect();
        s_retry_num++;
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG, "WiFi Connected");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WIFI GOT IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        gateway_data.wifi_status = 1;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    ESP_LOGI(TAG, "Wifi STA Init");
    s_wifi_event_group = xEventGroupCreate();

    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());

    //esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //esp_event_handler_instance_t instance_any_id;
    //esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "DefaultSSID",
            .password = "DefaultPW",
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };

    wifi_config_t wifi_cfg;
    if (esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg) == ESP_OK)
    {
        if (strcmp((const char *)wifi_config.sta.ssid, (const char *)wifi_cfg.sta.ssid) != 0)
        {
            memcpy(wifi_config.sta.ssid, wifi_cfg.sta.ssid, sizeof(wifi_cfg.sta.ssid));
        }
        if (strcmp((const char *)wifi_config.sta.password, (const char *)wifi_cfg.sta.password) != 0)
        {
            memcpy(wifi_config.sta.password, wifi_cfg.sta.password, sizeof(wifi_cfg.sta.password));
        }
    }

    wifi_ssid_exist = true;
    if (mqtt_config.get_active_start == true && !strlen((const char *)wifi_config.sta.ssid))
    {
        ESP_LOGI(TAG, "Not found wifi ssid, skip wifi init");
        wifi_ssid_exist = false;
        goto skip;
    }

    ESP_LOGI(TAG, "Found ssid %s", (const char *)wifi_config.sta.ssid);
    ESP_LOGI(TAG, "Found password %s", (const char *)wifi_config.sta.password);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    if (strlen((const char *)wifi_config.sta.ssid)) ESP_ERROR_CHECK(esp_wifi_start());

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to wifi");
        gateway_data.wifi_status = true;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to  to wifi");
        gateway_data.wifi_status = false;
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        // esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    skip:
    ESP_LOGI(TAG, "End of wifi init");

    /* The event will not be processed after unregister */
    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    // vEventGroupDelete(s_wifi_event_group);
}

// FOR PPPOS
static void modem_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case ESP_MODEM_EVENT_PPP_START:
        ESP_LOGI(TAG, "Modem PPP Started");
        break;
    case ESP_MODEM_EVENT_PPP_STOP:
        ESP_LOGI(TAG, "Modem PPP Stopped");
        xEventGroupSetBits(event_group, STOP_BIT);
        break;
    case ESP_MODEM_EVENT_UNKNOWN:
        ESP_LOGW(TAG, "Unknow line received: %s", (char *)event_data);
        // xEventGroupSetBits(event_group, CONNECT_BIT);
        break;
    default:
        break;
    }
}

static void on_ppp_changed(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "PPP state changed event %d", event_id);
    if (event_id == NETIF_PPP_ERRORUSER)
    {
        /* User interrupted event from esp-netif */
        esp_netif_t *netif = *(esp_netif_t **)event_data;
        ESP_LOGI(TAG, "User interrupted event from netif:%p", netif);
    }
    if (event_id == NETIF_PPP_PHASE_DISCONNECT)
    {
        xEventGroupSetBits(event_group, DISCONNECT_BIT);
    }
}

static void on_ip_event(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "IP event! %d", event_id);
    if (event_id == IP_EVENT_PPP_GOT_IP)
    {
        esp_netif_dns_info_t dns_info;

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        esp_netif_t *netif = event->esp_netif;

        ESP_LOGI(TAG, "Modem Connect to PPP Server");
        ESP_LOGI(TAG, "~~~~~~~~~~~~~~");
        ESP_LOGI(TAG, "IP          : " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Netmask     : " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "Gateway     : " IPSTR, IP2STR(&event->ip_info.gw));
        esp_netif_get_dns_info(netif, 0, &dns_info);
        ESP_LOGI(TAG, "Name Server1: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        esp_netif_get_dns_info(netif, 1, &dns_info);
        ESP_LOGI(TAG, "Name Server2: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        ESP_LOGI(TAG, "~~~~~~~~~~~~~~");
        xEventGroupSetBits(event_group, CONNECT_BIT);
        gateway_data.modem_status = true;
        modem_is_disconnected = false;
        ESP_LOGI(TAG, "GOT ip event!!!");

        if (!is_start_ota) {
            gateway_data.connect_info =  SIM3G;
            update_network_type();
            get_network_instane();

            esp_mqtt_client_stop(mqtt_handle);
            esp_netif_down(ifwifi);
            esp_mqtt_client_start(mqtt_handle);

            ESP_LOGI(TAG, "Change default network to SIM");
        }
    }
    else if (event_id == IP_EVENT_PPP_LOST_IP)
    {
        ESP_LOGI(TAG, "Modem Disconnect from PPP Server");
        xEventGroupSetBits(event_group, DISCONNECT_BIT);
        gateway_data.modem_status = false;
        gateway_data.connect_info = WIFI;
        modem_is_disconnected = true;
        update_network_type();
    }
    else if (event_id == IP_EVENT_GOT_IP6)
    {
        ESP_LOGI(TAG, "GOT IPv6 event!");

        ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
        ESP_LOGI(TAG, "Got IPv6 address " IPV6STR, IPV62STR(event->ip6_info.ip));
    }
}

void sim_denit(void){
    /* Exit PPP mode */
     ESP_ERROR_CHECK(esp_modem_stop_ppp(dte));
    
     xEventGroupWaitBits(event_group, STOP_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

    /* Power down module */
     //ESP_ERROR_CHECK(dce->power_down(dce));
     ESP_LOGI(TAG, "Power down");
     ESP_ERROR_CHECK(dce->deinit(dce));

     esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip_event);
     esp_event_handler_unregister(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, &on_ppp_changed);
     vEventGroupDelete(event_group);

    /* Destroy the netif adapter withe events, which internally frees also the esp-netif instance */
     esp_modem_netif_clear_default_handlers(modem_netif_adapter);
     esp_modem_netif_teardown(modem_netif_adapter);
     esp_netif_destroy(esp_netif);

     ESP_ERROR_CHECK(dte->deinit(dte));
}

void main_sim_init()
{

    ESP_LOGI(TAG, "Sim7000 Init");
    esp_log_level_set("*", ESP_LOG_VERBOSE);

#if CONFIG_LWIP_PPP_PAP_SUPPORT
    esp_netif_auth_type_t auth_type = NETIF_PPP_AUTHTYPE_PAP;
#elif CONFIG_LWIP_PPP_CHAP_SUPPORT
    esp_netif_auth_type_t auth_type = NETIF_PPP_AUTHTYPE_CHAP;
#else
#error "Unsupported AUTH Negotiation"
#endif
    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, &on_ppp_changed, NULL));

    event_group = xEventGroupCreate();

    // Init netif object
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_PPP();
    //esp_netif_t *
    esp_netif = esp_netif_new(&cfg);
    assert(esp_netif);

    /* create dte object */
    esp_modem_dte_config_t config = ESP_MODEM_DTE_DEFAULT_SIM_CONFIG();
    //modem_dte_t
    dte = esp_modem_dte_init(&config);
    /* Register event handler */
    ESP_ERROR_CHECK(esp_modem_set_event_handler(dte, modem_event_handler, ESP_EVENT_ANY_ID, NULL));
    /* create dce object */
#if CONFIG_EXAMPLE_MODEM_DEVICE_SIM800
    //modem_dce_t *
    dce = sim800_init(dte);
    //vTaskDelay(5000/portTICK_PERIOD_MS);
    //while(dce == NULL);
    // {
    //     ESP_ERROR_CHECK(dce->deinit(dce));
    //     vTaskDelay(3000/portTICK_PERIOD_MS);
    //     ESP_LOGI(TAG, "Device SIM800 is init()");
    // }

    //assert(dce != NULL);
    ESP_ERROR_CHECK(dce->power_up());
    //vTaskDelay(7000/portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Device SIM800 is power_up()");
    // sim_gpio_init();

    //ESP_ERROR_CHECK(dce->open(dce));
    if (dce->open(dce) == -1) {
        ESP_LOGI(TAG, "Sim not found, skip setup");
        goto skip_sim;
    }
    ESP_LOGI(TAG, "Device SIM800 is open()");
#elif CONFIG_EXAMPLE_MODEM_DEVICE_BG96
    modem_dce_t *dce = bg96_init(dte);
#else
#error "Unsupported DCE"
#endif
    ESP_ERROR_CHECK(dce->store_profile(dce));

    /* Print Module ID, Operator, IMEI, IMSI */
    ESP_LOGI(TAG, "Module: %s", dce->name);
    ESP_LOGI(TAG, "Operator: %s", dce->oper);
    ESP_LOGI(TAG, "IMEI: %s", dce->imei);
    ESP_LOGI(TAG, "IMSI: %s", dce->imsi);
    /* Get signal quality */
    uint32_t rssi = 0, ber = 0;
    ESP_ERROR_CHECK(dce->get_signal_quality(dce, &rssi, &ber));
    ESP_LOGI(TAG, "rssi: %d, ber: %d", rssi, ber);
    /* Get battery voltage */
    uint32_t voltage = 0, bcs = 0, bcl = 0;
    ESP_ERROR_CHECK(dce->get_battery_status(dce, &bcs, &bcl, &voltage));
    ESP_LOGI(TAG, "Battery voltage: %d mV", voltage);

    /* setup PPPoS network parameters */
    esp_netif_ppp_set_auth(esp_netif, auth_type, "", "");
    //void *
    modem_netif_adapter = esp_modem_netif_setup(dte);
    esp_modem_netif_set_default_handlers(modem_netif_adapter, esp_netif);
    /* attach the modem to the network interface */
    esp_netif_attach(esp_netif, modem_netif_adapter);
    /* Wait for IP address */
    xEventGroupWaitBits(event_group, CONNECT_BIT | DISCONNECT_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

#if CONFIG_EXAMPLE_SEND_MSG
    const char *message = "Welcome to ESP32!";
    ESP_ERROR_CHECK(example_send_message_text(dce, CONFIG_EXAMPLE_SEND_MSG_PEER_PHONE_NUMBER, message));
    ESP_LOGI(TAG, "Send send message [%s] ok", message);
#endif

    /* Power down module */
    // ESP_ERROR_CHECK(dce->power_down(dce));
    // ESP_LOGI(TAG, "Power down");
    // ESP_ERROR_CHECK(dce->deinit(dce));
    // ESP_ERROR_CHECK(dte->deinit(dte));
    skip_sim:
    ESP_LOGI(TAG, "%s %d", __func__, __LINE__);
}

// --- Software UART (TX only) on pins GPIO19 (TX) and GPIO25 (RX) ---
#define SOFT_UART_TX_PIN GPIO_NUM_19
#define SOFT_UART_RX_PIN GPIO_NUM_25
#define SOFT_UART_BAUD    9600

static inline void soft_uart_init(void)
{
    // Configure TX pin as output and idle HIGH
    gpio_pad_select_gpio(SOFT_UART_TX_PIN);
    gpio_set_direction(SOFT_UART_TX_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(SOFT_UART_TX_PIN, 1);

    // Configure RX pin as input (pull-up enabled for idle HIGH)
    gpio_pad_select_gpio(SOFT_UART_RX_PIN);
    gpio_set_direction(SOFT_UART_RX_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SOFT_UART_RX_PIN, GPIO_PULLUP_ONLY);
}

// Send one byte as UART (start bit, 8 data bits, 1 stop) - blocking
static void soft_uart_send_byte(uint8_t b)
{
    const uint32_t bit_time_us = (1000000UL + (SOFT_UART_BAUD / 2)) / SOFT_UART_BAUD; // rounded

    // Start bit (LOW)
    gpio_set_level(SOFT_UART_TX_PIN, 0);
    esp_rom_delay_us(bit_time_us);

    // Data bits (LSB first)
    for (int i = 0; i < 8; i++)
    {
        gpio_set_level(SOFT_UART_TX_PIN, (b >> i) & 1);
        esp_rom_delay_us(bit_time_us);
    }

    // Stop bit (HIGH)
    gpio_set_level(SOFT_UART_TX_PIN, 1);
    esp_rom_delay_us(bit_time_us);
}

static void soft_uart_send_buffer(const char *buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        soft_uart_send_byte((uint8_t)buf[i]);
    }
}

static void soft_uart_task(void *pvParameters)
{
    const char *msg = "Hello, debug\r\n";
    while (1)
    {
        soft_uart_send_buffer(msg, strlen(msg));
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}


void print_reset_reason(RESET_REASON reason)
{
  switch ( reason)
  {
    case 1 : ESP_LOGW(TAG, "POWERON_RESET");break;          /**<1, Vbat power on reset*/
    case 3 : ESP_LOGW(TAG, "SW_RESET");break;               /**<3, Software reset digital core*/
    case 4 : ESP_LOGW(TAG, "OWDT_RESET");break;             /**<4, Legacy watch dog reset digital core*/
    case 5 : ESP_LOGW(TAG, "DEEPSLEEP_RESET");break;        /**<5, Deep Sleep reset digital core*/
    case 6 : ESP_LOGW(TAG, "SDIO_RESET");break;             /**<6, Reset by SLC module, reset digital core*/
    case 7 : ESP_LOGW(TAG, "TG0WDT_SYS_RESET");break;       /**<7, Timer Group0 Watch dog reset digital core*/
    case 8 : ESP_LOGW(TAG, "TG1WDT_SYS_RESET");break;       /**<8, Timer Group1 Watch dog reset digital core*/
    case 9 : ESP_LOGW(TAG, "RTCWDT_SYS_RESET");break;       /**<9, RTC Watch dog Reset digital core*/
    case 10 : ESP_LOGW(TAG, "INTRUSION_RESET");break;       /**<10, Instrusion tested to reset CPU*/
    case 11 : ESP_LOGW(TAG, "TGWDT_CPU_RESET");break;       /**<11, Time Group reset CPU*/
    case 12 : ESP_LOGW(TAG, "SW_CPU_RESET");break;          /**<12, Software reset CPU*/
    case 13 : ESP_LOGW(TAG, "RTCWDT_CPU_RESET");break;      /**<13, RTC Watch dog Reset CPU*/
    case 14 : /**<14, for APP CPU, reseted by PRO CPU*/
    ESP_LOGW(TAG, "EXT_CPU_RESET");
    gateway_data.user_reset = true;
    break;
    case 15 : ESP_LOGW(TAG, "RTCWDT_BROWN_OUT_RESET");break;/**<15, Reset when the vdd voltage is not stable*/
    case 16 : ESP_LOGW(TAG, "RTCWDT_RTC_RESET");break;      /**<16, RTC Watch dog reset digital core and rtc module*/
    default : ESP_LOGW(TAG, "NO_MEAN");
  }
}

void app_main(void)
{
    esp_err_t ret;
	xMainMutex = xSemaphoreCreateMutex();
    struct dtor *head_local = NULL;
    int iter =0 ;
	// esp_ota_get_app_elf_sha256(gateway_data.fw_info, sizeof(gateway_data.fw_info));
    strcpy(gateway_data.fw_info,"a2f62970fc748f26"); // fake hash v17

    ESP_LOGI(TAG, "--- APP_MAIN: Fire Smart ......");
    ESP_LOGI(TAG, "--- APP_MAIN: IDF version: %s", esp_get_idf_version());
	ESP_LOGI(TAG, "--- APP_MAIN: App version %s", app_get_version());
	ESP_LOGI(TAG, "--- APP_MAIN: Firmware version %s", gateway_data.fw_info);
    ESP_LOGI(TAG, "--- APP_MAIN: Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGW(TAG, "CPU0 reset reason: ");
    print_reset_reason(rtc_get_reset_reason(0));

    ESP_LOGW(TAG, "CPU1 reset reason: ");
    print_reset_reason(rtc_get_reset_reason(1));
  
    // ESP_LOGW(TAG, "Running time %lld ", esp_timer_get_time());
    // int64_t current_time = esp_timer_get_time();
    // while (esp_timer_get_time() < 10000000 + current_time)
    // {
    //     vTaskDelay(20 / portTICK_PERIOD_MS);
    // };
    // ESP_LOGW(TAG, "Running time %lld delay 1min", esp_timer_get_time());

    /* Initialize NVS. */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
	ESP_LOGW(TAG, "Free memory: %d bytes  %d ++", esp_get_free_heap_size(), __LINE__);
    ESP_LOGI(TAG, "HMI init");
    UserTimer_Init();
    hmi_task();
    // Initialize soft UART and start TX task so external USB-TTL adapters connected to
    // GPIO19 (TX) and GPIO25 (RX) will see 'Hello, debug' periodically.
    soft_uart_init();
    xTaskCreatePinnedToCore(soft_uart_task, "soft_uart_task", 2048, NULL, 5 | portPRIVILEGE_BIT, NULL, 1);

	ESP_LOGW(TAG, "Free memory: %d bytes  %d ++", esp_get_free_heap_size(), __LINE__);
    ESP_LOGI(TAG, "Mount System file");
    flash_file_init();
    logger_list_file("/spiffs");
    check_map_size();
    bool status_1 = flash_client_private_pem_read();
    bool status_2 = flash_client_cert_pem_read();
    bool status_3 = flash_client_id_read();
    if ((status_1 == 1) && (status_2 == 1) && (status_3 == 1))
    {
        mqtt_config.get_active_start = true; //
    }
    // memset(mqtt_config.jobId, 0x00, sizeof(mqtt_config.jobId));
	is_start_ota = OTA_PENDING;

	ESP_LOGW(TAG, "Free memory: %d bytes  %d ++", esp_get_free_heap_size(), __LINE__);

    ESP_LOGI(TAG, "Sensor init");
    if (flash_read_sensor_data())
    {
        ESP_LOGI(TAG, "Sensor data restore");
    }
    else
    {
        ESP_LOGI(TAG, "Init sensor data");
        memset(&sensor_data.device_activated[0], 0x00, MAX_DECTECTOR + 1);
        memset(&sensor_data.dtor_sensor_state[0], 0x00, MAX_DECTECTOR + 1);
        memset(&sensor_data.dtor_bat_state[0], 0x00, MAX_DECTECTOR + 1);
        sensor_data.curDetector = 0;
        sensor_data.maxDetector = 0;
        sensor_data.inputChannel = 0;
        memset(&sensor_data.sync_code[0], 0x00, 3);
    }
    uart_fire_detector_sensor_init();
    network_init();

	ESP_LOGW(TAG, "Free memory: %d bytes  %d ++", esp_get_free_heap_size(), __LINE__);
    ESP_LOGI(TAG, "BLE Init");
    ble_init();
    ble_receive_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreatePinnedToCore(ble_config_wifi_task, "ble_config_wifi_task", 3 * 1024, NULL, 6 | portPRIVILEGE_BIT, NULL, 1);
    xTaskCreatePinnedToCore(network_switch_task, "network_switch_task", 8 * 1024, NULL, 8 | portPRIVILEGE_BIT, NULL, 1);

	ESP_LOGW(TAG, "Free memory: %d bytes  %d ++", esp_get_free_heap_size(), __LINE__);

    data_process = xQueueCreate(10, sizeof(uint32_t));

    wifi_init_sta();

	ESP_LOGW(TAG, "Free memory: %d bytes  %d ++", esp_get_free_heap_size(), __LINE__);

    if (flash_read_pair_key())
    {
        has_pair_key = true;
        ESP_LOGI(TAG, "Found the Pair key");
    }


	ESP_LOGW(TAG, "Free memory: %d bytes  %d ++", esp_get_free_heap_size(), __LINE__);
    uart_fire_detector_sensor_start();
	ESP_LOGW(TAG, "Free memory: %d bytes  %d ++", esp_get_free_heap_size(), __LINE__);
    ESP_LOGI(TAG, "MQTTS Init");
    plan_task();
	ESP_LOGW(TAG, "Free memory: %d bytes  %d ++", esp_get_free_heap_size(), __LINE__);
    ESP_LOGI(TAG, "Alarm Init");
    alarm_task();
	ESP_LOGW(TAG, "Free memory: %d bytes  %d ++", esp_get_free_heap_size(), __LINE__);
    head_local = get_head();
    for( iter = 1 ; iter <= sensor_data.maxDetector ; iter++ )
    {
        if(sensor_data.device_activated[iter] == 1)
        {
            add_state(&head_local, iter, esp_timer_get_time());
        }
    }
    ESP_LOGI(TAG, "Reset Alarm done");
    ESP_LOGI(TAG, "OTA Init");
    // ota_task();
	ESP_LOGW(TAG, "Free memory: %d bytes  %d ++", esp_get_free_heap_size(), __LINE__);

}
