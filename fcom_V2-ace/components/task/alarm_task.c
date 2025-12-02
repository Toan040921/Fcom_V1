/*
 * plan_task.c
 *
 *  Created on: Jan 7, 2021
 *      Author: delvin
 */
/***********************************************************************************************************************
* Pragma directive
***********************************************************************************************************************/

/***********************************************************************************************************************
* Includes <System Includes>
***********************************************************************************************************************/
#include "mqtt_task.h"
#include "ota_task.h"
#include "../../main/main_app.h"
#include "alarm_task.h"
/***********************************************************************************************************************
* Macro definitions
***********************************************************************************************************************/
static const char *TAG = "ALARM";
/***********************************************************************************************************************
* Typedef definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
* Private global variables and functions
***********************************************************************************************************************/
static void Heartbeat_Task(void *pvParameters);
SemaphoreHandle_t alarm_mutex;
/***********************************************************************************************************************
* Exported global variables and functions (to be accessed by other files)
***********************************************************************************************************************/
struct dtor *head = NULL;
/***********************************************************************************************************************
* Imported global variables and functions (from other files)
***********************************************************************************************************************/

/***********************************************************************************************************************
* Function Name:
* Description  :
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/
void alarm_task(void)
{
	alarm_mutex = xSemaphoreCreateMutex();
	xTaskCreatePinnedToCore(Heartbeat_Task, "heartbeat_task", 2 * 1024, NULL, 2 | portPRIVILEGE_BIT, NULL, 1);
}
/***********************************************************************************************************************
* Static Functions
***********************************************************************************************************************/
static void Heartbeat_Task(void *pvParameters)
{
	ESP_LOGI(TAG, " %s ++", __func__);
    struct dtor *checker = NULL;
    struct dtor *head_local = NULL;
    int64_t cur_time = 0;
    // print_state(&head_local);
    while(1)
    {
        head_local = get_head();
        while(head_local != NULL) {
            checker = get_head();
            while(checker != NULL)
            {
	            xSemaphoreTake(alarm_mutex, portMAX_DELAY);
                cur_time = esp_timer_get_time();
                if((cur_time - checker->timestamp >= HEART_BEAT_TIME) && (checker != NULL))
                {
                    sensor_data.curDetector = checker->id;
                    data_event_t cmd1 = SS_ERROR_FLAG;
                    if (xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS) != pdPASS)
                    {
                        ESP_LOGE(TAG, "Failed to send %s", __func__);
                    }
                    ESP_LOGE(TAG, "%lld Detector id %d dead!!!", cur_time, checker->id);
                    head_local= get_head();
                    pop_state(&head_local, checker->id);
                }
                else
                {
                    // ESP_LOGE(TAG, "Do nothing %s", __func__);
                }
                if(checker != NULL)
                    checker = checker->next;
                xSemaphoreGive(alarm_mutex);
                vTaskDelay(20 / portTICK_PERIOD_MS);
            }
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}
struct dtor *get_head(void)
{
    return head;
}

void set_head(struct dtor **list)
{
    head = *list;
}

void push_state(struct dtor **list, int id, int64_t timestamp)
{
    struct dtor *temp = NULL;
    if(*list == NULL)
    {
        temp = (struct dtor*)malloc(sizeof(struct dtor));
        if (temp == NULL)
            return;

        temp->id = id;
        temp->timestamp = timestamp;
        *list = temp;
        temp->next = NULL;
    } else {
        temp = (struct dtor*)malloc(sizeof(struct dtor));
        if (temp == NULL)
            return;

        temp->id = id;
        temp->timestamp = timestamp;
        temp->next = *list ;
        *list = temp;
    }
}

void print_state(struct dtor **list)
{
    struct dtor *temp = *list;
    int list_size = 0;
    while(temp != NULL)
    {
        ESP_LOGE(TAG, "[%d %lld]",temp->id, temp->timestamp);
        temp = temp->next;
        list_size++;
    }
    ESP_LOGE(TAG, " size %d",list_size);
}

void pop_state(struct dtor **list, int id)
{
    struct dtor *prev = *list;
    struct dtor *temp = *list;
    // print_state(&(*list));
    while (temp != NULL)
    {
        if (temp->id == id)
        {
            // if current node is the list head
            if (temp == *list)
            {
                *list = temp->next;
                set_head(&(*list));
            } else {
                prev->next = temp->next;
                free(temp);
                temp = NULL;
            }
            print_state(&(*list));
            return;
        }
        prev = temp;
        temp = temp->next;
    }
}

void add_state(struct dtor **list, int id, int64_t timestamp)
{
    struct dtor * temp = *list;
    // print_state(&(*list));
    while(temp != NULL)
    {
        if(temp->id == id)
        {
            temp->timestamp = timestamp;
            ESP_LOGI(TAG, "Update %d %lld",id, timestamp);
            set_head(&(*list));
            return;
        }
        temp = temp->next;
    }
    push_state(&(*list),id, timestamp);
    set_head(&(*list));
    ESP_LOGI(TAG, "Push %d %lld",id, timestamp);
}

/***********************************************************************************************************************
* End of file
***********************************************************************************************************************/
