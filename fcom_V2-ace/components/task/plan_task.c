/*
 * plan_task.c
 *
 *  Created on: Jan 7, 2021
 *      Author: ductu
 */
/***********************************************************************************************************************
* Pragma directive
***********************************************************************************************************************/

/***********************************************************************************************************************
* Includes <System Includes>
***********************************************************************************************************************/
#include "mqtt_task.h"
#include "plan_task.h"
#include "ota_task.h"
#include "../../main/main_app.h"
/***********************************************************************************************************************
* Macro definitions
***********************************************************************************************************************/
static const char *TAG = "PLAN";
/***********************************************************************************************************************
* Typedef definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
* Private global variables and functions
***********************************************************************************************************************/
enum {
	VERIFY_MQTT_CERT = 0,
	ACTIVATE_MQTT,
	WAIT_MANUAL_OTA
};

static void PlantControl_Task(void *pvParameters);

/***********************************************************************************************************************
* Exported global variables and functions (to be accessed by other files)
***********************************************************************************************************************/

/***********************************************************************************************************************
* Imported global variables and functions (from other files)
***********************************************************************************************************************/

/***********************************************************************************************************************
* Function Name:
* Description  :
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/
void plan_task(void)
{
	xTaskCreatePinnedToCore(PlantControl_Task, "plant_task", 8 * 1024, NULL, 7 | portPRIVILEGE_BIT, NULL, 1);
	// mqtt_task_start();
}
int plan_state = VERIFY_MQTT_CERT;
/***********************************************************************************************************************
* Static Functions
***********************************************************************************************************************/
static void PlantControl_Task(void *pvParameters)
{
	bool has_cert = false;
	//main_sim_init();

	while (1)
	{
		switch (plan_state)
		{
			case VERIFY_MQTT_CERT:
			{
				if(gateway_data.wifi_status == true || gateway_data.modem_status == true)
				{
					ESP_LOGI(TAG, "Verifying MQTT certification");
					if (mqtt_config.get_active_start == false)
					{
						has_cert = mqtt_get_cer();
					}
					else if (mqtt_config.get_active_start == true)
					{
						has_cert = true;
					}

					if(has_cert)
					{
						plan_state = ACTIVATE_MQTT;
						ESP_LOGI(TAG, "MQTT certificate correct");
					}
				}
				vTaskDelay(20 / portTICK_PERIOD_MS);
			}
			break;
			case ACTIVATE_MQTT:
			{
				if(gateway_data.wifi_status == true || gateway_data.modem_status == true)
				{
					if(is_start_ota == OTA_PENDING)
					{
						mqtt_task_start();
					}
					else if ((mqtt_config.get_active_start == false) && (is_start_ota == OTA_START))
					{
						ESP_LOGI(TAG, "\n\n==================Active OTA==================\n");
						ota_process();
					}
					plan_state = WAIT_MANUAL_OTA;
				}
				vTaskDelay(20 / portTICK_PERIOD_MS);
			}
			break;
			case WAIT_MANUAL_OTA:
			{
				if(gateway_data.wifi_status == true || gateway_data.modem_status == true)
				{
					if ((mqtt_config.get_active_start == true) && (is_start_ota == OTA_START))
					{
						ESP_LOGI(TAG, "\n\n==================Manual OTA==================\n");
						ota_process();
					}
				}
				vTaskDelay(20 / portTICK_PERIOD_MS);
			}
			break;
			default:
			{
				vTaskDelay(20 / portTICK_PERIOD_MS);
				ESP_LOGE(TAG, "Plan State Machine Error!");
			}
			break;
		}
	}
}

/***********************************************************************************************************************
* End of file
***********************************************************************************************************************/
