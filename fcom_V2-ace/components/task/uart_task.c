/*
 * uart_task.c
 *
 *  Created on: Nov 24, 2020
 *      Author: Yolo
 */
/***********************************************************************************************************************
 * Pragma directive
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes <System Includes>
 ***********************************************************************************************************************/
#include "uart_task.h"
#include "mqtt_task.h"
#include "alarm_task.h"

/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/
#define TAG "SSNET"
/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
#define BUF_SIZE (1024) // test
#define RD_BUF_SIZE (BUF_SIZE) // test
#define EX_UART_NUM UART_NUM_1
/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/
bool rf_config_mode;
static void uart_tx_task(void *arg);
static void state_check_connect_reset(void);
static uint8_t isSensorFeedback = 0;
bool isRegisterSensor = false;
static void get_addr_config(void);
static void RF_config_default(void);
static void SET_RF__SYNC_config(unsigned char MK1, unsigned char MK2, unsigned char MK3); // ham cai dat RF
/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/
static char buf_data_send[25];
static int sync_device_byte1, sync_device_byte2, sync_device_byte3;
static uint8_t count_check_active = 1;
static char ADD_H;
static char ADD_L;
static int ADD = 0;
sensor_data_t sensor_data;
gateway_data_t gateway_data;
xQueueHandle data_process;
SemaphoreHandle_t xMutex;
uint32_t prev_send_cmd = 0;

uint8_t prev_detector = 0;
sensor_state_t prev_state = ERROR_ST;

static QueueHandle_t uart0_queue; // test
static void rf_handle_ext_task(void *pvParameters); // test

static bool isRegister2Server(int num);
static void sensor_data_handler(void);

void uart_fire_detector_sensor_init(void)
{
	ESP_LOGI(TAG, "%s", __func__);
	/* Configure parameters of an UART driver,
	* communication pins and install the driver */
	uart_config_t uart_config = {
		.baud_rate = UART_BAUD_9600,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_APB,
	};
    //Set UART parameters
	ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    //Set UART pins
	ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, TXD_RF_PIN, RXD_RF_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    //Install UART driver, and get the queue.
	// ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, BUFF_SIZE * 2, 0, 0, NULL, 0));
	ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, BUFF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0));
}
/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/

static void uart_tx_task(void *arg)
{

}

void uart_fire_detector_sensor_start(void)
{
	ESP_LOGI(TAG, "Fire Detector Init");
	xMutex = xSemaphoreCreateMutex();
	// xTaskCreatePinnedToCore(uart_tx_task, "uart_tx_task", 1024 * 2, NULL, 1 | portPRIVILEGE_BIT, NULL, 0);
	ssStateMachine = INIT_CMD;
	xTaskCreatePinnedToCore(rf_handle_ext_task, "rf_handle_ext_task", 1024 * 3, NULL, 2 | portPRIVILEGE_BIT, NULL, 0);
}

void uart_fire_detector_send(unsigned char *buf, unsigned char size)
{
	// UserUart_WriteData(buf, size);
}

/***********************************************************************************************************************
 * Function Name:
 * Description  :
 * Arguments    : none
 * Return Value : none
 ***********************************************************************************************************************/

static void rf_handle_ext_task(void *pvParameters)
{
	int switch_listen_mode = 0;
	xSemaphoreTake(xMutex, portMAX_DELAY);
	get_addr_config();
	xSemaphoreGive(xMutex);
    for(;;)
	{
		switch (ssStateMachine)
		{
		case INIT_CMD:
		{
			prev_send_cmd = usertimer_gettick();
			ssStateMachine = FINDING_SUB;
			ESP_LOGI(TAG, "Sensor state machine next step %d", ssStateMachine);
		}
		break;
		case FINDING_SUB:
		{
			if(usertimer_gettick() - prev_send_cmd > 500)
			{
				sprintf(buf_data_send, "{T00}");
				ESP_LOGI(TAG, "[ News ]%s", buf_data_send);
				const int len = strlen(buf_data_send);
				uart_write_bytes(UART_NUM_1, buf_data_send, len);
				ssStateMachine = INIT_CMD;
				switch_listen_mode++;
				if(switch_listen_mode >= 3)
				{
					ssStateMachine = WAIT_ADD_SUB;
				}
				ESP_LOGI(TAG, "Sensor state machine next step %d", ssStateMachine);
			}
		}
		break;
		case WAIT_ADD_SUB:
		{
			if (rf_config_mode)
			{
				ESP_LOGI(TAG, "Add sub devices %d %d %d ", sensor_data.sync_code[0], sensor_data.sync_code[1], sensor_data.sync_code[2]);
				if (gateway_data.status != ADD_SUB_DEVICE)
					gateway_data.status = ADD_SUB_DEVICE;
				ssStateMachine = CONFIG_ADD_MODE;
				ESP_LOGI(TAG, "Sensor state machine next step %d", ssStateMachine);
			}
			else
			{
				ESP_LOGI(TAG, "%s Listen to local network", __func__);
				ssStateMachine = CONFIG_LISTEN_MODE;
				ESP_LOGI(TAG, "Sensor state machine next step %d", ssStateMachine);
			}

		}
		break;
		case CONFIG_ADD_MODE:
		{
			xSemaphoreTake(xMutex, portMAX_DELAY);
			RF_config_default();
			get_addr_config();
			xSemaphoreGive(xMutex);
			data_event_t cmd1 = GW_FEEDBACK_FLAG;
			if (xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS) != pdPASS)
			{
				ESP_LOGE(TAG, "Failed to send %s", __func__);
			}

			if (rf_config_mode)
			{
				ssStateMachine = REGISTER_SUB;
			}
			else
			{
				ssStateMachine = CONFIG_LISTEN_MODE;
			}

			ESP_LOGI(TAG, "Sensor state machine next step %d", ssStateMachine);
		}
		break;
		case CONFIG_LISTEN_MODE:
		{
			xSemaphoreTake(xMutex, portMAX_DELAY);
			ESP_LOGI(TAG, "Sync code %d %d %d ", sensor_data.sync_code[0], sensor_data.sync_code[1], sensor_data.sync_code[2]);
			SET_RF__SYNC_config(sensor_data.sync_code[0], sensor_data.sync_code[1], sensor_data.sync_code[2]);
			get_addr_config();
			xSemaphoreGive(xMutex);

			ssStateMachine = LISTEN_SUB_EVENT;
			ESP_LOGI(TAG, "Sensor state machine next step %d done", ssStateMachine);
		}
		break;
		case REGISTER_SUB:
		case LISTEN_SUB_EVENT:
		{
			sensor_data_handler();
		}
		break;
		case FINISHED_ADD_SUB:
		default:
			ESP_LOGE(TAG, "Sensor state machine failed");
			break;
		}

    }
    vTaskDelete(NULL);
}
/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/

static void sensor_data_handler(void)
{
	static int i = 0;
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
	struct dtor *head_local = NULL;
	// 		vTaskDelay(10000 / portTICK_PERIOD_MS);
	// while (1)
	// {
	// 	data_event_t cmd1 = SS_FEEDBACK_FLAG;
	// 	for(int i = 1 ;i< 51 ; i++)
	// 	{
	// 		sensor_data.maxDetector = i;
	// 		sensor_data.device_activated[i] = 1;
	// 		sensor_data.dtor_sensor_state[i] = NORMAL_ST;
	// 		sensor_data.dtor_bat_state[i] = HIGH;
	// 		sensor_data.curDetector = i;
	// 		if (xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS) != pdPASS)
	// 		{
	// 			ESP_LOGE(TAG, "Failed to send %s", __func__);
	// 		}
	// 		vTaskDelay(100 / portTICK_PERIOD_MS);
	// 	}
	// }
	// ESP_LOGI(TAG, "%s ++", __func__);
	//Waiting for UART event.
	// if(xQueueReceive(uart0_queue, (void * )&event, (portTickType)portMAX_DELAY)) {
	if(xQueueReceive(uart0_queue, (void * )&event, (portTickType)  1000 / portTICK_RATE_MS))
	{
		bzero(dtmp, RD_BUF_SIZE);
		// ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
		switch(event.type) {
			//Event of UART receving data
			/*We'd better handler data event fast, there would be much more data events than
			other types of events. If we take too much time on data event, the queue might
			be full.*/
			case UART_DATA:
				ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
				const int rxBytes = uart_read_bytes(EX_UART_NUM, dtmp, event.size, 1000 / portTICK_RATE_MS);
				if (rxBytes > 0)
				{
					dtmp[rxBytes] = 0;
					ESP_LOGI(TAG, "Read %d bytes: '%s'", rxBytes, dtmp);
					ESP_LOG_BUFFER_HEXDUMP(TAG, dtmp, rxBytes, ESP_LOG_INFO);
					if (dtmp[0] == '{')
					{
						for (i = 0; i < rxBytes; i++)
						{
							xSemaphoreTake(xMutex, portMAX_DELAY );
							if ((dtmp[i] == 'N') && (dtmp[i + 1] == '0') && (dtmp[i + 2] == '0') && (dtmp[i + 3] == '}'))
							{
								for (count_check_active = 1 ; count_check_active < (MAX_DECTECTOR + 1); count_check_active++)
								{
									if (sensor_data.device_activated[count_check_active] == 0)
									{
										isRegisterSensor = true;
										sync_device_byte1 = (int)sensor_data.sync_code[0];
										sync_device_byte2 = (int)sensor_data.sync_code[1];
										sync_device_byte3 = (int)sensor_data.sync_code[2];
										ADD_H = count_check_active / 10;
										ADD_L = count_check_active % 10;
										sprintf(buf_data_send, "{S%d%d%d%d%d%d%d%d%d%d%d}", ADD_H, ADD_L,
												(sync_device_byte1 / 100), (sync_device_byte1 % 100 / 10), (sync_device_byte1 % 10),
												(sync_device_byte2 / 100), (sync_device_byte2 % 100 / 10), (sync_device_byte2 % 10),
												(sync_device_byte3 / 100), (sync_device_byte3 % 100 / 10), (sync_device_byte3 % 10));
										ESP_LOGI(TAG, "Device_Fire send %s", buf_data_send);
										const int len = strlen(buf_data_send);
										uart_write_bytes(UART_NUM_1, buf_data_send, len);
										if(sensor_data.maxDetector < count_check_active)
										sensor_data.maxDetector = count_check_active;
										sensor_data.dtor_sensor_state[count_check_active] = NORMAL_ST;
										sensor_data.dtor_bat_state[count_check_active] = HIGH;
										vTaskDelay(350 / portTICK_PERIOD_MS);
										ssStateMachine = CONFIG_LISTEN_MODE;
										ESP_LOGI(TAG, "Sensor state machine next step %d", ssStateMachine);
										break;
									}
								}
							}
							if ((dtmp[i] == 'F') && (dtmp[i + 3] == '}'))
							{
								ADD = ((dtmp[i + 1] - 0x30) * 10) + (dtmp[i + 2] - 0x30);
								if((ADD>0)&&(ADD<51))
								{
									//if(ADD <= sensor_data.maxDetector)
									if(ADD <= 50)
									{
										ESP_LOGI(TAG, "Device_Fire %d F", ADD);
										sensor_data.dtor_sensor_state[ADD] = ALARM_ST;
										//sensor_data.dtor_connect_state[ADD] = true;
										sensor_data.curDetector = ADD;
										update_relay_alarm_state(sensor_data.curDetector);
										isSensorFeedback = 1;
										prev_state = 0;
										prev_detector = 0;
										gateway_data.alarm_status = true;
										// ESP_LOGI(TAG, "Device_Fire isRegisterSensor =  %d", isRegisterSensor);
									}
								}
							}
							if ((dtmp[i] == 'L') && (dtmp[i + 3] == '}'))
							{
								ADD = ((dtmp[i + 1] - 0x30) * 10) + (dtmp[i + 2] - 0x30);
								if((ADD>0)&&(ADD<51))
								{
									//if(ADD <= sensor_data.maxDetector)
									if(ADD <= 50)
									{
										ESP_LOGI(TAG, "Device_Fire %d L", ADD);
										sensor_data.dtor_sensor_state[ADD] = NORMAL_ST;
										sensor_data.dtor_bat_state[ADD]=LOW;
										//sensor_data.dtor_connect_state[ADD] = True_ST;
										sensor_data.curDetector = ADD;
										isSensorFeedback = 1;
										prev_state = 0;
										prev_detector = 0;

										bool alarm = false;
										//for (int i = 1; i <= sensor_data.maxDetector; i++)
										for (int i = 1; i <= 50; i++)
										{
											if (sensor_data.dtor_sensor_state[i] == ALARM_ST) alarm = true;
										}
										gateway_data.alarm_status = alarm;
										// ESP_LOGI(TAG, "Device_Fire isRegisterSensor =  %d", isRegisterSensor);
									}
								}
							}
							if ((dtmp[i] == 'E') && (dtmp[i + 3] == '}'))
							{
								ADD = ((dtmp[i + 1] - 0x30) * 10) + (dtmp[i + 2] - 0x30);
								if((ADD>0)&&(ADD<51))
								{
									//if(ADD <= sensor_data.maxDetector)
									if(ADD <= 50)
									{
										ESP_LOGI(TAG, "Device_Fire %d E", ADD);
										sensor_data.dtor_sensor_state[ADD] = ERROR_ST;
										//sensor_data.dtor_connect_state[ADD] = True_ST;
										sensor_data.curDetector = ADD;
										isSensorFeedback = 1;
										prev_state = 0;
										prev_detector = 0;

										bool alarm = false;
										//for (int i = 1; i <= sensor_data.maxDetector; i++)
										for (int i = 1; i <= 50; i++)
										{
											if (sensor_data.dtor_sensor_state[i] == ALARM_ST) alarm = true;
										}
										gateway_data.alarm_status = alarm;
										// ESP_LOGI(TAG, "Device_Fire isRegisterSensor =  %d", isRegisterSensor);
									}
								}
							}
							if ((dtmp[i] == 'T') && (dtmp[i + 3] == '}'))
							{
								ADD = ((dtmp[i + 1] - 0x30) * 10) + (dtmp[i + 2] - 0x30);
								if((ADD>0)&&(ADD<51))
								{
									//if(ADD  <= sensor_data.maxDetector)
									if(ADD  <= 50)
									{
										ESP_LOGI(TAG, "Device_Fire %d T", ADD);
										sensor_data.dtor_sensor_state[ADD] = NORMAL_ST;
										sensor_data.dtor_bat_state[ADD]=HIGH;
										//sensor_data.dtor_connect_state[ADD] = True_ST;
										sensor_data.curDetector = ADD;
										update_relay_alarm_state(sensor_data.curDetector);
										isSensorFeedback = 1;
										prev_state = 0;
										prev_detector = 0;

										bool alarm = false;
										//for (int i = 1; i <= sensor_data.maxDetector; i++)
										for (int i = 1; i <= 50; i++)
										{
											if (sensor_data.dtor_sensor_state[i] == ALARM_ST) alarm = true;
										}
										gateway_data.alarm_status = alarm;
										// ESP_LOGI(TAG, "Device_Fire isRegisterSensor =  %d", isRegisterSensor);
									}
								}
							}
							xSemaphoreGive(xMutex);
						}
					}
					if ((isSensorFeedback == 1) && isRegister2Server(sensor_data.curDetector))
					{
						data_event_t cmd1 = SS_REGISTER_SERVER;
						if (xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS) != pdPASS)
						{
							ESP_LOGE(TAG, "Failed to send %s", __func__);
						}
					}
					//if ((isSensorFeedback == 1) && (sensor_data.device_activated[sensor_data.curDetector] == 1))
					if (isSensorFeedback == 1)
					{
						isSensorFeedback = 0;
						#if 0
						if ( (sensor_data.dtor_sensor_state[sensor_data.curDetector] != prev_state) || \
							 (sensor_data.curDetector != prev_detector))
						{
							prev_state = sensor_data.dtor_sensor_state[sensor_data.curDetector];
							prev_detector = sensor_data.curDetector;
						#endif
							data_event_t cmd1 = SS_FEEDBACK_FLAG;
							if (xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS) != pdPASS)
							{
								ESP_LOGE(TAG, "Failed to send %s", __func__);
							}
							head_local = get_head();
							add_state(&head_local, sensor_data.curDetector, esp_timer_get_time());
						// }
					}
				}
				break;
			//Event of HW FIFO overflow detected
			case UART_FIFO_OVF:
				ESP_LOGI(TAG, "hw fifo overflow");
				// If fifo overflow happened, you should consider adding flow control for your application.
				// The ISR has already reset the rx FIFO,
				// As an example, we directly flush the rx buffer here in order to read more data.
				uart_flush_input(EX_UART_NUM);
				xQueueReset(uart0_queue);
				break;
			//Event of UART ring buffer full
			case UART_BUFFER_FULL:
				ESP_LOGI(TAG, "ring buffer full");
				// If buffer full happened, you should consider encreasing your buffer size
				// As an example, we directly flush the rx buffer here in order to read more data.
				uart_flush_input(EX_UART_NUM);
				xQueueReset(uart0_queue);
				break;
			//Event of UART RX break detected
			case UART_BREAK:
				ESP_LOGI(TAG, "uart rx break");
				break;
			//Event of UART parity check error
			case UART_PARITY_ERR:
				ESP_LOGI(TAG, "uart parity error");
				break;
			//Event of UART frame error
			case UART_FRAME_ERR:
				ESP_LOGI(TAG, "uart frame error");
				break;
			//UART_PATTERN_DET
			case UART_PATTERN_DET:
				uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
				int pos = uart_pattern_pop_pos(EX_UART_NUM);
				ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
				// if (pos == -1) {
				//     // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
				//     // record the position. We should set a larger queue size.
				//     // As an example, we directly flush the rx buffer here.
				//     uart_flush_input(EX_UART_NUM);
				// } else {
				//     uart_read_bytes(EX_UART_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
				//     uint8_t pat[PATTERN_CHR_NUM + 1];
				//     memset(pat, 0, sizeof(pat));
				//     uart_read_bytes(EX_UART_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
				//     ESP_LOGI(TAG, "read data: %s", dtmp);
				//     ESP_LOGI(TAG, "read pat : %s", pat);
				// }
				break;
			//Others
			default:
				ESP_LOGI(TAG, "uart event type: %d", event.type);
				break;
		}
	}
    free(dtmp);
    dtmp = NULL;
	// ESP_LOGI(TAG, "%s --", __func__);
}

static void state_check_connect_reset(void)
{
	// empty
}

static bool isRegister2Server(int num)
{
	int idev = 0;
	xSemaphoreTake(xMutex, portMAX_DELAY );
	if(num > (MAX_DECTECTOR + 1)|| num < 1)
	{
		xSemaphoreGive(xMutex);
		return false;
	}

	for( idev = 1 ; idev <= num ; idev++)
	{
		// ESP_LOGI(TAG, "Device i = %d  num = %d %s isRegisterSensor = %s", idev, num, __func__, isRegisterSensor ? "true" : "false");
		if((sensor_data.device_activated[idev] == 0) && (isRegisterSensor != false))
		{
			ESP_LOGI(TAG, "Device %d %s", idev , __func__);
			isRegisterSensor = false;
			sensor_data.device_activated[idev] = 1;
			xSemaphoreGive(xMutex);
			return true;
		}
	}
	xSemaphoreGive(xMutex);
	return false;
}

static void get_addr_config(void)
{
	unsigned char data1[] = {0xAA};
    ESP_LOGI(TAG, "%s ++ ", __func__);

    data1[0]=0xFD;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x00;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xFE;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(600 / portTICK_PERIOD_MS);

    data1[0]=0xFD;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x05;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x01;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x08;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xFE;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(600 / portTICK_PERIOD_MS);

    data1[0]=0xFD;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x01;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xFE;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(600 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "%s -- ", __func__);
}
static void RF_config_default(void)
{
	unsigned char data1[] = {0xAA};
    ESP_LOGI(TAG, "%s ++ ", __func__);
	// rf_config_write(start_config, 3);
	// rf_config_write(get_mac_info, 5);
	// rf_config_write(stop_config, 3);
    //----Start config------------//
    data1[0]=0xFD;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x00;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xFE;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(600 / portTICK_PERIOD_MS);

       //SET GFSK_DATA_RATE_4.8kbs
      //while(SYNC_RF==0);// cho Busy// cho Busy
     data1[0]=0xFD;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x44;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x01;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x00;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x03;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xFE;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(600 / portTICK_PERIOD_MS);
      //SET GFSK_DATA_RATE_chanel 3 - 434Mhz
     // while(SYNC_RF==0);// cho Busy// cho Busy
    data1[0]=0xFD;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x44;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x01;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x02;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x01;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x03;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xFE;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(600 / portTICK_PERIOD_MS);
    /*
      // SET ADD
      while(SYNC_RF==0);// cho Busy
      send_byte(0xFD);__delay_ms(1);
      send_byte(0x44);__delay_ms(1);
      send_byte(0x02);__delay_ms(1);
      send_byte(0x00);__delay_ms(1);
      send_byte(add_L);__delay_ms(1);
      send_byte(add_H);__delay_ms(1);
      send_byte(0xFE);__delay_ms(600);
      */
      // while(SYNC_RF==0);// cho Busy
     data1[0]=0xFD;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
     data1[0]=0x44;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x04;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x06;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xA6;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xFE;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(600 / portTICK_PERIOD_MS);
            // SET SYN WORD
     // while(SYNC_RF==0);// cho Busy
     data1[0]=0xFD;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x44;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x01;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
     data1[0]=0x08;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xDE;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x51;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x0B;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
     data1[0]=0x93;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xFE;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(600 / portTICK_PERIOD_MS);

       //END CONFIG
     // while(SYNC_RF==0);// cho Busy
     data1[0]=0xFD;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x01;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xFE;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(600 / portTICK_PERIOD_MS);

	// rf_config_write(stop_config);
	// while(SYNC_RF==0x00);// cho Busy
    ESP_LOGI(TAG, "%s --", __func__);
}

static void SET_RF__SYNC_config(unsigned char MK1, unsigned char MK2, unsigned char MK3) // ham cai dat RF
{
	unsigned char data1[] = {0xAA};
    ESP_LOGI(TAG, "%s ++", __func__);
      //START CONFIG
     // while(SYNC_RF==0);// cho Busy
    data1[0]=0xFD;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x00;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xFE;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(600 / portTICK_PERIOD_MS);


            // SET SYN WORD
      //while(SYNC_RF==0);// cho Busy
     data1[0]=0xFD;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
     data1[0]=0x44;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x01;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x08;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
     data1[0]=MK3;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=MK2;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=MK1;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xFA;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xFE;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(600 / portTICK_PERIOD_MS);
       //END CONFIG
     // while(SYNC_RF==0);// cho Busy
    data1[0]=0xFD;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0x01;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(1 / portTICK_PERIOD_MS);
    data1[0]=0xFE;
    uart_write_bytes(UART_NUM_1, data1, 1);vTaskDelay(600 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "%s --", __func__);
}
/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/
