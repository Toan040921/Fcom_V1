/*
 * fire_detector.c
 *
 *  Created on: Jan 9, 2021
 *      Author: ductu
 */
/***********************************************************************************************************************
 * Pragma directive
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes <System Includes>
 ***********************************************************************************************************************/
#include "fire_detector.h"

#define TAG "FIRE_DTOR"
#if 0
static void tx_task(void *pvParameters);
static unsigned char data1[] = {0xAA};

void fire_detector_tx_process_task_creat(void)
{
	// xTaskCreatePinnedToCore(tx_task, "tx_task", 1 * 1024, NULL, 2 | portPRIVILEGE_BIT, NULL, 1);
}



uint32_t prev_send_time = 0;
int tmp = 0;
// static DRAM_ATTR SemaphoreHandle_t xSemaphoreRf = NULL;
void rf_config_write(const send_pkg_t rfcfg[],int len)
{
	int c = 0;
	int curCount = 0;
	// xSemaphoreRf = xSemaphoreCreateMutex();
	// xSemaphoreTake(xSemaphoreRf, portMAX_DELAY);

	for(c = 0; c < len;)
	{
		curCount = c;
		while( curCount == c)
		{
			switch(tmp)
			{
				case 0:
				{
					tmp = 1;
					prev_send_time = usertimer_gettick();
					// ESP_LOGI(TAG, "case 0 count  %d ", prev_send_time);
				}
				break;
				case 1:
				{
					if(usertimer_gettick() - prev_send_time > rfcfg[c].delay)
					{
						// uart_write_bytes(UART_NUM_1,(unsigned char*) &rfcfg[c].data, 1);
						if(c == 0)
						{
							data1[0] = 0xFD;
						}
						else if(c == 1)
						{
							data1[0] = 0x00;
						}
						else
						{
							data1[0] = 0xFE;
						}

						ESP_LOGI(TAG, "Delay %d pkg %d size %d", rfcfg[c].delay, rfcfg[c].data, sizeof(rfcfg[c].data));
						c++;
						prev_send_time = usertimer_gettick();
						tmp = 0;
					}
					// vTaskDelay(1);
				}
				break;
				default:
				break;
			}

		}
	}
	// xSemaphoreGive(xSemaphoreRf);
}
#endif