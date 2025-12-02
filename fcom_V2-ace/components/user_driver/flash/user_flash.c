/*
 * user_flash.c
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
#include "user_flash.h"
#include "esp_spiffs.h"
#include <dirent.h>
#include "nvs_flash.h"
/***********************************************************************************************************************
* Macro definitions
***********************************************************************************************************************/
#define TAG "MEM"
/***********************************************************************************************************************
* Typedef definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
* Private global variables and functions
***********************************************************************************************************************/
esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 7,
    .format_if_mount_failed = true
};

static int find_curDetector(void);
/***********************************************************************************************************************
* Exported global variables and functions (to be accessed by other files)
***********************************************************************************************************************/
/***********************************************************************************************************************
* Imported global variables and functions (from other files)
***********************************************************************************************************************/
/***********************************************************************************************************************
* Function Name: flash_erase_all_partions
* Description  : format all
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/
void flash_erase_all_partions(void)
{
    nvs_flash_erase();
    ESP_LOGI(TAG, "erase all nvs partions");
    esp_spiffs_format(conf.partition_label);
    ESP_LOGI(TAG, "erase all spiffs partions");
    esp_vfs_spiffs_unregister(&conf);
}
/***********************************************************************************************************************
* Function Name:
* Description  :
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/
void flash_file_init(void)
{
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGI(TAG, "Failed to initialize SPIFFS (%d)", ret);
        }
    }
}
/***********************************************************************************************************************
* Function Name:
* Description  :
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/

/***********************************************************************************************************************
* Function Name:
* Description  :
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/

void flash_client_private_pem_save(void)
{
    FILE *file;
    char buffer[3010];
    memset(buffer, 0x00, sizeof(buffer));
    file = fopen("/spiffs/private_pem.txt", "w");
    ESP_LOGI(TAG, "mqtt_config.certificate_pem =\n %s", mqtt_config.private_pem);

    //ghi de gia tri vao
    sprintf(buffer, "%s", mqtt_config.private_pem);
    fputs(buffer, file);
    memset(buffer, 0x00, sizeof(buffer));
    fclose(file);
}

void flash_client_cert_pem_save(void)
{
    FILE *file;
    char buffer[3010];
    memset(buffer, 0x00, sizeof(buffer));
    file = fopen("/spiffs/certificate_pem.txt", "w");
    ESP_LOGI(TAG, "mqtt_config.certificate_pem =\n %s", mqtt_config.certificate_pem);
    //ghi de gia tri vao
    sprintf(buffer, "%s", mqtt_config.certificate_pem);

    fputs(buffer, file);
    memset(buffer, 0x00, sizeof(buffer));
    fclose(file);
}

void flash_client_id_save(void)
{
    FILE *file;
    char buffer[50];
    memset(buffer, 0x00, sizeof(buffer));
    file = fopen("/spiffs/client_id.txt", "w");
    ESP_LOGI(TAG, "mqtt_config.client_id = %s", mqtt_config.client_id);
    //ghi de gia tri vao
    sprintf(buffer, "%s", mqtt_config.client_id);

    fputs(buffer, file);
    memset(buffer, 0x00, sizeof(buffer));
    fclose(file);
}

bool flash_client_id_read(void)
{
    FILE *file;
    char *buffer;
    buffer = (char *)malloc(50 + 1);
    uint16_t length = 0;
    memset(buffer, 0x00, 50 + 1);
    file = fopen("/spiffs/client_id.txt", "r");
    if (file == NULL)
    {
        free(buffer);
        return 0;
    }
    length = fread(buffer, 1, 50, file);
    sprintf(mqtt_config.client_id, "%s",(char *)buffer);
    ESP_LOGI(TAG, "mqtt_config.client_id = %s", mqtt_config.client_id);
    free(buffer);
    fclose(file);
    return 1;
}
/***********************************************************************************************************************
* Function Name:
* Description  :
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/

bool flash_client_private_pem_read(void)
{
    FILE *file;
    char *buffer;
    buffer = (char *)malloc(2000 + 1);
    uint16_t length = 0;
    memset(buffer, 0x00, 2000 + 1);
    file = fopen("/spiffs/private_pem.txt", "r");
    if (file == NULL)
    {
        free(buffer);
        return 0;
    }
    length = fread(buffer, 1, 2000, file);
    sprintf(mqtt_config.private_pem, "%.*s\n", length,
            (char *)buffer);
    ESP_LOGI(TAG, "mqtt_config.private_pem = %s", mqtt_config.private_pem);
    free(buffer);
    fclose(file);
    return 1;
}

bool flash_client_cert_pem_read(void)
{
    FILE *file;
    char *buffer;
    buffer = (char *)malloc(2000 + 1);
    memset(buffer, 0x00, 2000 + 1);
    uint16_t length = 0;
    file = fopen("/spiffs/certificate_pem.txt", "r");
    if (file == NULL)
    {
        free(buffer);
        return 0;
    }
    length = fread(buffer, 1, 2000, file);
    sprintf(mqtt_config.certificate_pem, "%.*s\n", length,
            (char *)buffer);
    ESP_LOGI(TAG, "certificate_pem = %s", mqtt_config.certificate_pem);
    free(buffer);
    fclose(file);
    return 1;
}

/***********************************************************************************************************************
* Function Name: logger_list_file
* Description  :
* Arguments    : file_name name of file need check
                 dir dir of file name
* Return Value : true/false
***********************************************************************************************************************/
bool logger_list_file(char *location)
{
    DIR *d;
    struct dirent *dir;
    bool status = true;
    d = opendir(location);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            ESP_LOGI(TAG, "**** %s ****", dir->d_name);
        }
        closedir(d);
    }
    return status;
}

/***********************************************************************************************************************
* Function Name: check_map_size
* Description  :
* Arguments    :
* Return Value : true/false
***********************************************************************************************************************/

uint8_t check_map_size(void)
{
    bool status = false;
    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        status = false;
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        status = true;
        ESP_LOGI(TAG, "Partition size: total: %d Mb, used: %d", total, used);
    }
    return status;
}

void flash_save_pair_key(const wifi_author_t input)
{
    FILE *file;
    char buffer[50];
    memset(buffer, 0x00, sizeof(buffer));
    file = fopen("/spiffs/pair.txt", "w");

    //ghi de gia tri vao
    memset(buffer, 0x00, sizeof(buffer));
    sprintf(buffer, "%s", input.mPairToken);
    fputs(buffer, file);
    ESP_LOGI(TAG, "Write PairToken = %s %d buffer = %s", input.mPairToken, strlen(input.mPairToken), buffer);
    memset(buffer, 0x00, sizeof(buffer));
    fclose(file);
}

bool flash_read_pair_key(void)
{
    FILE *file;
    char *buffer;
    buffer = (char *)malloc(50 + 1);
    uint16_t length = 0;
    memset(buffer, 0x00, 50 + 1);
    file = fopen("/spiffs/pair.txt", "r");
    if (file == NULL)
    {
        free(buffer);
        return 0;
    }
    length = fread(buffer, 1, 50, file);
    memcpy(wifi_author.mPairToken,(char *) buffer, length);
    ESP_LOGI(TAG, "Readback PairToken = %s", wifi_author.mPairToken);
    free(buffer);
    fclose(file);
    return 1;
}

void flash_save_sensor_data(void)
{
    FILE *file;
    sensor_data_t *object=malloc(sizeof(sensor_data_t));
    //ghi de gia tri vao
    memcpy(object, &sensor_data, sizeof(sensor_data_t));
    file = fopen("/spiffs/sensor_data.txt", "w");
    if (file != NULL) {
        fwrite(object, sizeof(sensor_data_t), 1, file);
    }
    free(object);
    fclose(file);
}

bool flash_read_sensor_data(void)
{
    FILE *file;
    int i = 0;
    char *ss_state[] = {"normal", "alarm", "error"};
    char *ss_bat[] = {"low", "middle", "high"};

    file = fopen("/spiffs/sensor_data.txt", "r");
    if (file != NULL) {
        fread(&sensor_data, sizeof(sensor_data_t), 1, file);
        fseek(file, 0, SEEK_SET);
        fclose(file);
    }
    else
    {
        fclose(file);
        ESP_LOGE(TAG, "Readback sensor_data failed");
        return false;
    }
    ESP_LOGI(TAG, "Readback sensor_data.curDetector = %d", sensor_data.curDetector);
    ESP_LOGI(TAG, "Readback sensor_data.maxDetector = %d", sensor_data.maxDetector);
    ESP_LOGI(TAG, "Readback sensor_data.device_activated =");
    printf("[");
    for( i = 1 ; i <= sensor_data.maxDetector ; i++ )
    {
        printf(" %d", sensor_data.device_activated[i]);
    }
    printf(" ]\n");
    ESP_LOGI(TAG, "Readback sensor_data.dtor_sensor_state =");
    printf("[");
    for( i = 1 ; i <= sensor_data.maxDetector ; i++ )
    {
        printf(" %s", ss_state[sensor_data.dtor_sensor_state[i]]);
    }
    printf(" ]\n");
    ESP_LOGI(TAG, "Readback sensor_data.dtor_bat_state =");
    printf("[");
    for( i = 1 ; i <= sensor_data.maxDetector ; i++ )
    {
        printf(" %s", ss_bat[sensor_data.dtor_bat_state[i]]);
    }
    printf(" ]\n");
    ESP_LOGI(TAG, "Readback sensor_data.sync_code = %03d.%03d.%03d", sensor_data.sync_code[0], sensor_data.sync_code[1], sensor_data.sync_code[2]);
    fclose(file);
    return true;
}

bool flash_erase_sensor_data(int idx)
{
    int i = 0;
    if (sensor_data.maxDetector == 0 )
    {
        ESP_LOGI(TAG, "None detector");
        return false;
    }
    for( i = 1 ; i <= sensor_data.maxDetector ; i++ )
    {
        if(i == idx)
        {
            if (sensor_data.inputChannel == idx)
            {
                sensor_data.inputChannel = 0;
            }
            sensor_data.device_activated[idx] = 0;
            sensor_data.dtor_sensor_state[idx] = NORMAL_ST;
            sensor_data.dtor_bat_state[idx] = LOW;
            sensor_data.curDetector = find_curDetector();
            flash_save_sensor_data();
            ESP_LOGI(TAG, "Erase data sensor[%d]",idx);
            return true;
        }
    }
    
    ESP_LOGI(TAG, "Can't find out sensor[%d]",idx);
    return false;
}

static int find_curDetector(void)
{
    int i = 0;
    if(sensor_data.maxDetector == 0)
    {
        return 0;
    }

    for( i = 1; i <= MAX_DECTECTOR; i++)
    {
        if(sensor_data.device_activated[i] == 1)
        {
        ESP_LOGI(TAG, "Rewrite curDectetor %d",i);
        return i;
        }
    }
    return 0;
}

/***********************************************************************************************************************
* Static Functions
***********************************************************************************************************************/

/***********************************************************************************************************************
* End of file
***********************************************************************************************************************/