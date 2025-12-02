file(GLOB_RECURSE SOURCES *.c)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS .
                       REQUIRES esp_adc_cal esp_http_client spiffs nvs_flash peripheral)