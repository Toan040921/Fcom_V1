/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef MAIN_APP_H_
#define MAIN_APP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/**
 * @brief ESP Modem DTE Default Configuration
 *
 */

void main_sim_init(void);

#define ESP_MODEM_DTE_DEFAULT_SIM_CONFIG()          \
    {                                           \
        .port_num = UART_NUM_2,                 \
        .data_bits = UART_DATA_8_BITS,          \
        .stop_bits = UART_STOP_BITS_1,          \
        .parity = UART_PARITY_DISABLE,          \
        .baud_rate = 115200,                    \
        .flow_control = MODEM_FLOW_CONTROL_NONE,\
        .tx_io_num = 5,                        \
        .rx_io_num = 4,                        \
        .rts_io_num = 0,                       \
        .cts_io_num = 0,                       \
        .rx_buffer_size = 1024,                 \
        .tx_buffer_size = 512,                  \
        .pattern_queue_size = 20,               \
        .event_queue_size = 30,                 \
        .event_task_stack_size = 2048,          \
        .event_task_priority = 5,               \
        .line_buffer_size = 512                 \
   }

// SPI LAN W5500
#define PIN_NUM_MISO                19
#define PIN_NUM_MOSI                23
#define PIN_NUM_CLK                 18
#define PIN_NUM_CS                  21
#define SPI_CLOCK_MHZ               20
#define CONFIG_ETH_SPI_HOST         2
#define CONFIG_ETH_SPI_INT_GPIO     35



#endif /* MAIN_APP_H_ */