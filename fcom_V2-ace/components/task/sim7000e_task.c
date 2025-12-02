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
#include "sim7000e_task.h"
/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/
#define TAG "SIM7000E"
/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
#define PATTERN_CHR_NUM (3)
#define RD_BUF_SIZE (BUFF_SIZE)
/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/
static QueueHandle_t uart2_queue;
typedef enum
{
    INIT = 0,
    CONNECT,
    PAUSE,
    AT_OK
} sim_state_machine_t;
static sim_state_machine_t SimStateMachine = INIT;
static void sim_rx_task(void *pvParameters);
static void sim_tx_task(void *pvParameters);
// static void sim_gpio_init(void);
// static void sim_hard_reset(void);
/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/
uint32_t recheck = 0;
/***********************************************************************************************************************
 * Function Name:
 * Description  :
 * Arguments    : none
 * Return Value : none
 ***********************************************************************************************************************/
void sim_init(void)
{
    ESP_LOGI(TAG, "%s", __func__);

    sim_gpio_init();
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // Set UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
    // Set UART pins
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    // Install UART driver, and get the queue.
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, BUFF_SIZE * 2, 0, 20, &uart2_queue, 0));

    // Set uart pattern detect function.
    // uart_enable_pattern_det_baud_intr(UART_NUM_2, '+', PATTERN_CHR_NUM, 9, 0, 0);
    // Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(UART_NUM_2, 20);
    sim_start();
}

void sim_start(void)
{
    ESP_LOGI(TAG, "Sim Init");
    // xMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(sim_rx_task, "sim_rx_task", 1024 * 4, NULL, 2 | portPRIVILEGE_BIT, NULL, 0);
    xTaskCreatePinnedToCore(sim_tx_task, "sim_tx_task", 1024 * 4, NULL, 1 | portPRIVILEGE_BIT, NULL, 0);
}
/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/
int stm_sim = 0;
static void sim_tx_task(void *pvParameters)
{
    char buf_data_send[30];
    ESP_LOGI(TAG, "%s %d", __func__, __LINE__);
    while (1)
    {
        switch (SimStateMachine)
        {
            case INIT:
            {
                recheck = usertimer_gettick();
                SimStateMachine = CONNECT;
            }
            break;
            case CONNECT:
            {
                sprintf(buf_data_send, "AT\r");
                const int len = strlen(buf_data_send);
                if (usertimer_gettick() - recheck > 2000)
                {
                    recheck = usertimer_gettick();
                    ESP_LOGI(TAG, "[ News ] %s %s", __func__, buf_data_send);
                    uart_write_bytes(UART_NUM_2, buf_data_send, len);
                    SimStateMachine = CONNECT;
                }
                vTaskDelay(20);
            }
            break;
            case AT_OK:
            {
                ESP_LOGI(TAG, "-------------Success connect SIM--------------");
                stm_sim++;
                if(stm_sim == 1)
                {
                    sprintf(buf_data_send, "AT+CPIN?\r");
                    const int len = strlen(buf_data_send);
                    ESP_LOGI(TAG, "News [%s]", buf_data_send);
                    uart_write_bytes(UART_NUM_2, buf_data_send, len);
                }
                else if (stm_sim == 2)
                {
                    sprintf(buf_data_send, "AT+CSTT=\"v-internet\"\r");
                    const int len = strlen(buf_data_send);
                    ESP_LOGI(TAG, "News [%s]", buf_data_send);
                    uart_write_bytes(UART_NUM_2, buf_data_send, len);
                }
                else if (stm_sim == 3)
                {
                    sprintf(buf_data_send, "AT+CIICR\r");
                    const int len = strlen(buf_data_send);
                    ESP_LOGI(TAG, "News [%s]", buf_data_send);
                    uart_write_bytes(UART_NUM_2, buf_data_send, len);
                }
                else if (stm_sim == 4)
                {
                    sprintf(buf_data_send, "AT+CIFSR\r");
                    const int len = strlen(buf_data_send);
                    ESP_LOGI(TAG, "News [%s]", buf_data_send);
                    uart_write_bytes(UART_NUM_2, buf_data_send, len);
                }
                else if (stm_sim == 5)
                {
                    sprintf(buf_data_send, "AT+CIPPING=\"www.google.com\"\r");
                    const int len = strlen(buf_data_send);
                    ESP_LOGI(TAG, "News [%s]", buf_data_send);
                    uart_write_bytes(UART_NUM_2, buf_data_send, len);
                }

                SimStateMachine = PAUSE;
            }
            break;
            case PAUSE:
            {
                vTaskDelay(5000/portTICK_PERIOD_MS);
                if(stm_sim == 6)
                {
                    while(1)
                    {
                        vTaskDelay(5000/portTICK_PERIOD_MS);
                        ESP_LOGI(TAG, "Tx do nothing ");
                    }
                }
            }
            break;
            default:
            {
                ESP_LOGI(TAG, "State Machine Error");
            }
            break;
        break;
        }
    }
}
static void sim_rx_task(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    uint8_t *dtmp = (uint8_t *)malloc(RD_BUF_SIZE);
    ESP_LOGI(TAG, "%s %d", __func__, __LINE__);
    for (;;)
    {
        // Waiting for UART event.
        if (xQueueReceive(uart2_queue, (void *)&event, (portTickType)portMAX_DELAY))
        {
            bzero(dtmp, RD_BUF_SIZE);
            ESP_LOGI(TAG, "uart[%d] event:", UART_NUM_2);
            switch (event.type)
            {
            // Event of UART receving data
            /*We'd better handler data event fast, there would be much more data events than
            other types of events. If we take too much time on data event, the queue might
            be full.*/
            case UART_DATA:
                {
                ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                uart_read_bytes(UART_NUM_2, dtmp, event.size, portMAX_DELAY);
                ESP_LOG_BUFFER_HEXDUMP(TAG, dtmp, event.size, ESP_LOG_INFO);
                    if(strstr((char *)dtmp, "OK"))
                    {
                        ESP_LOGI(TAG, "Pattern found");
                        SimStateMachine = AT_OK;
                    }
                    if(stm_sim >= 1 )
                    {
                        SimStateMachine = AT_OK;
                    }
                }
                break;
            // Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "hw fifo overflow");
                // If fifo overflow happened, you should consider adding flow control for your application.
                // The ISR has already reset the rx FIFO,
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(UART_NUM_2);
                xQueueReset(uart2_queue);
                break;
            // Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "ring buffer full");
                // If buffer full happened, you should consider encreasing your buffer size
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(UART_NUM_2);
                xQueueReset(uart2_queue);
                break;
            // Event of UART RX break detected
            case UART_BREAK:
                ESP_LOGI(TAG, "uart rx break");
                break;
            // Event of UART parity check error
            case UART_PARITY_ERR:
                ESP_LOGI(TAG, "uart parity error");
                break;
            // Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGI(TAG, "uart frame error");
                break;
            // UART_PATTERN_DET
            case UART_PATTERN_DET:
                uart_get_buffered_data_len(UART_NUM_2, &buffered_size);
                int pos = uart_pattern_pop_pos(UART_NUM_2);
                ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                if (pos == -1)
                {
                    // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                    // record the position. We should set a larger queue size.
                    // As an example, we directly flush the rx buffer here.
                    uart_flush_input(UART_NUM_2);
                }
                else
                {
                    uart_read_bytes(UART_NUM_2, dtmp, pos, 100 / portTICK_PERIOD_MS);
                    // uint8_t pat[PATTERN_CHR_NUM + 1];
                    // memset(pat, 0, sizeof(pat));
                    // uart_read_bytes(UART_NUM_2, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                    ESP_LOGI(TAG, "read data: %s", dtmp);
                    // ESP_LOGI(TAG, "read pat : %s", pat);
                }
                break;
            // Others
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void sim_gpio_init(void)
{
    gpio_config_t io_conf;
    esp_err_t error;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.5
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SIM_SEL;
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

    // gpio_set_level(GPIO_SIM_PWRKEY, 0);
    // gpio_set_level(GPIO_SIM_RESET, 1);
    // vTaskDelay(5000 / portTICK_PERIOD_MS);

    gpio_set_level(GPIO_SIM_PWRKEY, 1);
    gpio_set_level(GPIO_SIM_RESET, 1);
    vTaskDelay(1500/portTICK_PERIOD_MS);
    gpio_set_level(GPIO_SIM_PWRKEY, 0);
    vTaskDelay(15000/portTICK_PERIOD_MS);
}

void sim_hard_reset(void)
{
    ESP_LOGI(TAG, "%s", __func__);
    gpio_set_level(GPIO_SIM_RESET, 0);
    vTaskDelay(500 / portTICK_PERIOD_MS); // temp
    gpio_set_level(GPIO_SIM_RESET, 1);
}

/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/
