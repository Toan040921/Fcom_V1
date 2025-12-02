/*
 * ota_task.c
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
#include "ota_task.h"
/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/
static const char *TAG = "OTA";
/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/
static void ota_start(void *pvParameters);
static bool ota_get_status(void);
// static bool ota_process(void);

uint8_t request_message_frist_time = 0;
// Define client certificate
const char *ota_root_ca_cert_pem_start = "-----BEGIN CERTIFICATE-----\n"
                                     "MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n"
                                     "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
                                     "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n"
                                     "WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n"
                                     "ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n"
                                     "MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n"
                                     "h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n"
                                     "0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n"
                                     "A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n"
                                     "T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n"
                                     "B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n"
                                     "B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n"
                                     "KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n"
                                     "OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n"
                                     "jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n"
                                     "qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n"
                                     "rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n"
                                     "HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n"
                                     "hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n"
                                     "ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n"
                                     "3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n"
                                     "NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n"
                                     "ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n"
                                     "TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n"
                                     "jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n"
                                     "oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n"
                                     "4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n"
                                     "mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n"
                                     "emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n"
                                     "-----END CERTIFICATE-----\n";
const char *ota_server_cert_pem_start = "-----BEGIN CERTIFICATE-----\n"
                                    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
                                    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
                                    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
                                    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
                                    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
                                    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
                                    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
                                    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
                                    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
                                    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
                                    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
                                    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
                                    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
                                    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
                                    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
                                    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
                                    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
                                    "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
                                    "-----END CERTIFICATE-----\n";
/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/
uint32_t previous_time_ota_start = 0;
uint8_t state_ota = 0;
char ota_infor_message[800] = {0};
uint16_t ota_infor_length = 0;
volatile bool ota_call_first_time = true;
bool ota_get = false;
/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/
esp_err_t _ota_http_event_handle(esp_http_client_event_t *evt)
{
	switch (evt->event_id)
	{
	case HTTP_EVENT_ERROR:
		ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
		break;
	case HTTP_EVENT_ON_CONNECTED:
		ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
		break;
	case HTTP_EVENT_HEADER_SENT:
		ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        // printf("%.*s", evt->data_len, (char *)evt->data);
		break;
	case HTTP_EVENT_ON_HEADER:
		ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
		// length = sprintf(cer_link + length, "%.*s", evt->data_len, (char *)evt->data);
		break;
	case HTTP_EVENT_ON_DATA:
		ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (esp_http_client_is_chunked_response(evt->client))
		{
			printf("Chunked data HTTP_EVENT_ON_DATA %d", evt->data_len);
			ota_infor_length += sprintf(ota_infor_message + ota_infor_length, "%.*s", evt->data_len, (char *)evt->data);
		}
		// if (!esp_http_client_is_chunked_response(evt->client))
		// {
		// 	if (request_message_frist_time >= 1)
		// 	{
		// 		request_message_frist_time = 2;
		// 	}
		// 	else
		// 		request_message_frist_time = 1;
		// 	printf("%.*s", evt->data_len, (char *)evt->data);
		// }
		break;
	case HTTP_EVENT_ON_FINISH:
		ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
		// printf("%.*s", evt->data_len, (char *)evt->data);
		break;
	case HTTP_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED ");
		ota_get = json_parser_ota_link(ota_infor_message, ota_infor_length);
		if (ota_get == false)
		{
			ESP_LOGI(TAG, "Failed clean data ");
			ota_infor_length = 0;
			memset(ota_message_link, 0x00, sizeof(ota_message_link));
			memset(ota_infor_message, 0x00, sizeof(ota_infor_message));
		}
		int mbedtls_err = 0;
		esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
		if (err != 0) {
			ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
			ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
		}
		break;
	}
	return ESP_OK;
}
/***********************************************************************************************************************
 * Function Name:
 * Description  :
 * Arguments    : none
 * Return Value : none
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * Static Functions
 ***********************************************************************************************************************/
// static bool ota_process(void)
bool ota_process(void)
{
	bool ota_process_status = false;
	ESP_LOGI(TAG, "Starting Advanced OTA");

	esp_err_t ota_finish_err = ESP_OK;
	esp_http_client_config_t config = {
		// .url = "https://iot-storages.s3.ap-southeast-1.amazonaws.com/firmware/fcom001/1aRacf8EBT_mk-XxUJLBP-XKfdQG0sC7.bin?X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIA2KOWOZWZCP5WEP7B%2F20220504%2Fap-southeast-1%2Fs3%2Faws4_request&X-Amz-Date=20220504T151803Z&X-Amz-SignedHeaders=host&X-Amz-Expires=3600&X-Amz-Signature=28399294367a57e7fb68a1e2220832c9c45ce4f0b1d03ad9d2d70ee512c902ba",
		.url = ota_message_link,
		.cert_pem = (char *)ota_server_cert_pem_start,
		.timeout_ms = 20000,
	};
	// esp_http_client_set_url(client_2, ota_message_link);

	esp_https_ota_config_t ota_config = {
		.http_config = &config,
	};
	esp_https_ota_handle_t https_ota_handle = NULL;

	esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
	if (err != ESP_OK)
	{
		ESP_LOGW(TAG, "ESP HTTPS OTA Begin failed");
		ota_process_status = false;
	}

	esp_app_desc_t app_desc;
	err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
	if (err != ESP_OK)
	{
		ESP_LOGW(TAG, "esp_https_ota_read_img_desc failed");
		ota_process_status = false;
		goto ota_end;
	}

	while (1)
	{
		err = esp_https_ota_perform(https_ota_handle);
		if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
		{
			break;
		}
		// esp_https_ota_perform returns after every read operation which gives user the ability to
		// monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
		// data read so far.
		ESP_LOGI(TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
	}

	if (esp_https_ota_is_complete_data_received(https_ota_handle) != true)
	{
		// the OTA image was not completely received and user can customise the response to this situation.
		ESP_LOGW(TAG, "Complete data was not received.");
		ota_process_status = false;
	} else {
		ota_finish_err = esp_https_ota_finish(https_ota_handle);
		if ((err == ESP_OK) && (ota_finish_err == ESP_OK))
		{
			is_start_ota = OTA_DONE;
			ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			esp_restart();
		}
		else
		{
			if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED)
			{
				ESP_LOGW(TAG, "Image validation failed, image is corrupted");
			}
			ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade failed %d", ota_finish_err);
			ota_process_status = false;
		}
	}
	// free
ota_end:
	is_start_ota = OTA_PENDING;
    esp_https_ota_abort(https_ota_handle);
    ESP_LOGW(TAG, "ESP_HTTPS_OTA upgrade failed");
	ota_infor_length = 0;
	memset(ota_message_link, 0x00, sizeof(ota_message_link));
	return ota_process_status;
}

char* app_get_version(void)
{
    return esp_ota_get_app_description()->version;
}
/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/
