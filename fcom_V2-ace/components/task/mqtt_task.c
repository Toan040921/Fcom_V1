/*
 * mqtt_task.c
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
#include "../hmi/hmi_service.h"
// #include "esp_heap_trace.h"

// #define NUM_RECORDS 100
// static heap_trace_record_t trace_record[NUM_RECORDS]; // This buffer must be in internal RAM
/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/
static const char *TAG = "MQTTS";
/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048 * 4
#define QOS_PKG_NORMAL 0
#define QOS_PKG_IMPORTANT 1
#define MQTT_HEAP_SIZE 512

/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/
wifi_author_t wifi_author;
mqtt_config_t mqtt_config;
sensor_data_t sensor_data;
gateway_data_t gateway_data;
// static esp_mqtt_client_handle_t s_client = NULL;

static void mqtt_app_start(void);
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event);
static void mqtt_send_task(void *pvParameters);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);


//static void update_relay_alarm_state(uint8_t dev_alert);

e_mqtt_get_state mqtt_get_state = E_MQTT_GET_CER_LINK;
bool page_not_found = false;
bool is_mqtt_first_boot = false;

// const char *client_cert_pem_start = "-----BEGIN CERTIFICATE-----\n"
//                                     "MIIEgDCCAmgCCQCNHYxckGcHjzANBgkqhkiG9w0BAQsFADBuMQswCQYDVQQGEwJW\n"
//                                     "TjEPMA0GA1UECAwGSGEgTm9pMQ8wDQYDVQQHDAZIYSBOb2kxEDAOBgNVBAoMB0Vs\n"
//                                     "aWZldXAxFTATBgNVBAsMDElvVCBQbGF0Zm9ybTEUMBIGA1UEAwwLZWxpZmV1cC5j\n"
//                                     "b20wIBcNMjEwNTE2MTAyNjE4WhgPMjA3MTA1MDQxMDI2MThaMIGTMQswCQYDVQQG\n"
//                                     "EwJWTjEPMA0GA1UECAwGSGEgTm9pMQ8wDQYDVQQHDAZIYSBOb2kxEDAOBgNVBAoM\n"
//                                     "B0VsaWZldXAxDDAKBgNVBAsMA0lvVDEiMCAGA1UEAwwZU21hcnRTdG9wIElvVCBD\n"
//                                     "ZXJ0aWZpY2F0ZTEeMBwGCSqGSIb3DQEJARYPaHV5QGVsaWZldXAuY29tMIIBIjAN\n"
//                                     "BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAqq2uEgH2hhNm1ZmUbAgIYFWvAduj\n"
//                                     "UbIkOPz7fY2QynxMAyWclB6c/Nz2ZBudKCbkUYaKHwzrk0rHwbF8WQiom2JHzsZx\n"
//                                     "QA4yJDuJ7aV9Zw+9btCnmwYSlWQ80TdfUcZgCedXusXvoPo1ajVtnGOlhsceY/0w\n"
//                                     "0mT0OJ9YKuOi0yaPrJas6bvw5xlVwhFN5r1PgFZ/mEZZLVLI/ki96Z/7pqKSD5g7\n"
//                                     "bz/nOPOrq5/LqMUXt8qjZv7BeYm+F+7ml4mu6WCVIE9U2k6o8ci/pjU5KQaAaqcL\n"
//                                     "5ivvCayQzG+5R8lhU96ppyoYojEy9F+3gp9Qd0hoXLt+uTbnNpXtY2dbvQIDAQAB\n"
//                                     "MA0GCSqGSIb3DQEBCwUAA4ICAQBjZMpSk0MgdGSJD0IWc9bv7do8ZaoMv8tETCQK\n"
//                                     "MIKNM8XBGpuYDQyS48dUUImmPpd+yBNDuqoBQs6xp9phDrwBXpSPr/vTiAAVWG51\n"
//                                     "zNRTSVhGgNGr/dd6WXWQApF7h8hAv2J9CL4q55eMPkSHyD9G7DuDbhL2ilP6PjE5\n"
//                                     "BBiXpMmvbN6kq+2PDS/TZwAHeRc1IFpbyllgzuOEqkRXnllMVPRWDcH4kp7Nr3il\n"
//                                     "jWI6zGbezFn23H+FNQatpW9EpjnOmdyfsh5OENf1nZ4zAHkO4MHoI3nWht6zWTMR\n"
//                                     "E2nTM6zi0aUU+aZO9MwWkdQkx8dpJl6oZ2Bcjx4mizDHoDDJN3fAT20IdvkcBwAb\n"
//                                     "L6ypDECy34LPq7w0jqeNuPCVXPz/VDtT5Dj40G4ChNVzmgapLU4LJZJAetdCJqja\n"
//                                     "hUBZ3/KHp0FcrIKEJo39HxZO2uDht0pK4OXTnzvFzsaAfMhtwCMaK4qWYmJqLxVl\n"
//                                     "Br5iWxWGPW2n/nhLNn8cU3Kl3b1wlwwJD65uQrRTxXn5Ux1BGaXi/w/8tqX8pbJS\n"
//                                     "WqaU6SEtS8xq9PIFqi+0BPrJN062aQ+KqWxkOpFSvg8oRc0H31K4LKy+YkgGzjYs\n"
//                                     "Pk89gojkYIOritKubZeLDOzjJKvYAEChoUf/aNWKt5m/mJZaCkh78bo5F8kvE3Az\n"
//                                     "8vmrjA==\n"
//                                     "-----END CERTIFICATE-----\n";

// const char *client_key_pem_start = "-----BEGIN RSA PRIVATE KEY-----\n"
//                                    "MIIEpAIBAAKCAQEAqq2uEgH2hhNm1ZmUbAgIYFWvAdujUbIkOPz7fY2QynxMAyWc\n"
//                                    "lB6c/Nz2ZBudKCbkUYaKHwzrk0rHwbF8WQiom2JHzsZxQA4yJDuJ7aV9Zw+9btCn\n"
//                                    "mwYSlWQ80TdfUcZgCedXusXvoPo1ajVtnGOlhsceY/0w0mT0OJ9YKuOi0yaPrJas\n"
//                                    "6bvw5xlVwhFN5r1PgFZ/mEZZLVLI/ki96Z/7pqKSD5g7bz/nOPOrq5/LqMUXt8qj\n"
//                                    "Zv7BeYm+F+7ml4mu6WCVIE9U2k6o8ci/pjU5KQaAaqcL5ivvCayQzG+5R8lhU96p\n"
//                                    "pyoYojEy9F+3gp9Qd0hoXLt+uTbnNpXtY2dbvQIDAQABAoIBAC92pw6o7xZv9Mv3\n"
//                                    "rpewUCwCB+37V1qTsJEMgR90K8yzbiv93KIwNTX4eKh0KWsODbZCNMzXufc293/8\n"
//                                    "zHix+LllRlTRSJMon3cF+6BTwiDT9rkHW2S39pkGzAbeYCqMgQ6f//yXqMDac9o6\n"
//                                    "S2YPK+vkGaZytY38txG79jfPH+uZvFHnraVbulKWF3ib/nG8DnOFPeCbkNYwk2CT\n"
//                                    "RlcGjlrBGMXTPz60OPykUhHqAggvRa1EgOEJjHPW7qwhApe1L5nmBf/9LVG/7YHR\n"
//                                    "442ahMQnjazVmdM3BiQXzxeoqk8V0UCswP9ur+QNKPFkw9q/FLRMu9LfyGtw6IhU\n"
//                                    "HiU47sECgYEA1ybctYJfkfSdmdiI8YoGsINFrAasSFM1SWo1ONrtq+ZTgKnvd3qB\n"
//                                    "q0fD97YpYESgBNOSci7C3uNPniyBrATYlQDgMqEm4njxbcsUlmyreufDKU22R8J/\n"
//                                    "KN1uL0vaeZZnfmEvgRUjcr3JNnJe2p8OgxyEgzqBm5IkMAAGRjOKH+0CgYEAyxVB\n"
//                                    "SLSHHon4dA3HzwdYS5XlwPy3G6hFnyOcFRZhqhhG0K9yKklW5bnLzqwYi+umAoUC\n"
//                                    "Q+FLE6HaLc+XN9Z/f4jSMZ0+UCZSm7KBH9Wt80HrMzRTA12pWxRhDzXi6cJSIq9J\n"
//                                    "0QsOZbBOdrSwECbn+CKg8f3rzER7bwnFdYb+kRECgYEAqz1aEvkmGaPov+bw79Wc\n"
//                                    "h2aj0EwrWREo6zqmC49r9RJHybL3Tk/p3qoq1gBdJCradZzzBQAkx4OB/fGMb54X\n"
//                                    "x1hAGOvcaAo8ldc5lpP9U8Acu8YHW0v5K0w6A1jLFVTZIGQ3i/SIFy3odPZIepZ3\n"
//                                    "1XCgI1Ywi+Kf/Lg4Ri2FNO0CgYEAmMmC9korpgQzUkzT2KQz/5nk4w6+TCaLSrEl\n"
//                                    "yo+uJqRhErwMblgC8o6YEQNU7F/748Vh8OPc8gZA+VpG8JGGFtM/IGim6vIKEG15\n"
//                                    "zBOc7XjYlQt2sP+UXJu2chUehLPXy5SJOqbQzByay6AhHeXHe93BrI5XCrUzEFUP\n"
//                                    "o95OQ6ECgYBz9AIaPjG46EkbMi4tivA2WOFDIQ20EZ4Mx8qhAeF7haKK8JDFuVZo\n"
//                                    "pttkbbTedsAN+kBCZ88c9uvf4PqFNqPN0pdcB4a6lqlx8GK9xQl5zpT+71dKG5r1\n"
//                                    "Xor3UGRUO5cB1V3O+2JAjeS3AZHCdr720XoVRKoQYzFulzZbJQ2QzA==\n"
//                                    "-----END RSA PRIVATE KEY-----\n";
const char *client_cert_pem_start = "-----BEGIN CERTIFICATE-----\n"
                                    "MIIEuDCCAqCgAwIBAgIBADANBgkqhkiG9w0BAQsFADBuMQswCQYDVQQGEwJWTjEP\n"
                                    "MA0GA1UECAwGSGEgTm9pMQ8wDQYDVQQHDAZIYSBOb2kxEDAOBgNVBAoMB0VsaWZl\n"
                                    "dXAxFTATBgNVBAsMDElvVCBQbGF0Zm9ybTEUMBIGA1UEAwwLZWxpZmV1cC5jb20w\n"
                                    "IBcNMjIwMjI3MTUyMDMwWhgPMjA3MjAyMTUxNTIwMzBaMHoxCzAJBgNVBAYTAlZO\n"
                                    "MQ8wDQYDVQQIDAZIYSBOb2kxDzANBgNVBAcMBkhhIE5vaTEQMA4GA1UECgwHRWxp\n"
                                    "ZmV1cDEXMBUGA1UECwwOSW9UIC0gUGxhdGZvcm0xHjAcBgkqhkiG9w0BCQEWD2h1\n"
                                    "eUBlbGlmZXVwLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALU+\n"
                                    "mFydaVOjg/2u4AqOCcochhm1toDusp5q5Q9Mfx3W9ZRdniNnxG54rlmjfMeup28a\n"
                                    "iRuBhzK8YhDRobRgZlEA907QtEUmubMYV30nPFRUNF7lZPmg8jXjdhYK9nWE7/Lf\n"
                                    "q8yWQJra32m2s+Rh9VJKZe2FTDjxP4YSFDI6SRsunuLXsj4iU7qZtisi17Gy/d0F\n"
                                    "RDYkrK7u/YKa7jbY7dwqWvejJK8jkkLuMCxfSVTYnmZe6ht0dygcTe6mA1Wglvlj\n"
                                    "VkruVgTF/xWmyPdAmM8csAC8wQC+j65gcU2e4f5E4UqgfjX54OQcnXQcr1FOG/vS\n"
                                    "X6qPGhPEZv8p0oeQL30CAwEAAaNTMFEwHQYDVR0OBBYEFCAffZkwk6mMsGbE+rPE\n"
                                    "b+PMk/6kMB8GA1UdIwQYMBaAFHjyjMhAgOa0rdD6odqK8byXsTcOMA8GA1UdEwEB\n"
                                    "/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggIBADl9fTSvnLtzd5bKrFl2IOI4zS6f\n"
                                    "08LMKZGl9+p2HlN17Z4vsQ5CO5InQ7qgmL8V+AQo2j3/4hegjTqg4TelHYRMwm9g\n"
                                    "UNZE30hm529ciHRuSdFrZehlwqlMoah3/bVl6mXggu2yt9LIB28j419QDo97Hpc7\n"
                                    "dejpc2tT3TCkha4t1hEeyopK81BsiUOUAbWs5P+wC8mvi90Xu2pitfTRkns7mQPl\n"
                                    "xLsehrO7Uw0ppqsMXOfgVIX3uRqhp3JRfEIJSTVuu+Y0qOw8oCvURJeHlHDrr51/\n"
                                    "eAG7kh0eZYBcTnu164+PP8ISi08XVJMNSe95loQvjDIPm25l9ZwLFXSimVe88nHh\n"
                                    "v8x4X8DvNqM+ecxSpcH6ve6wLVbIz1Nk4hWcik6+iKlPuZ/504YshZ0ShIDjW34P\n"
                                    "+EIrW0RASst/pIcl2Y457LVRZYs6xiRjbz3B5uhiJ2UjNAXnkhJlcFhM9vhQyYZA\n"
                                    "O6WV3QaO+GYxWc3ZWWgiaiGDOIgD2F9yrTNz0b6TZ6YLlaNbd3xp8GdydR/fOLEA\n"
                                    "bVHXQE29H/qnoDTcMZUH6lkdfvEosb74maFCkGBiNOxh6LPWTWnca1uimwOsXiSb\n"
                                    "gqOXAeMWqflWbh4EYULviq0Zl3RmmRzFRYbbuHpYumnf3RiBrRuS/1dk8Xx1E5DT\n"
                                    "ydr5Ze8rnTGCNWHZ\n"
                                    "-----END CERTIFICATE-----\n";

const char *client_key_pem_start = "-----BEGIN PRIVATE KEY-----\n"
                                   "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC1PphcnWlTo4P9\n"
                                   "ruAKjgnKHIYZtbaA7rKeauUPTH8d1vWUXZ4jZ8RueK5Zo3zHrqdvGokbgYcyvGIQ\n"
                                   "0aG0YGZRAPdO0LRFJrmzGFd9JzxUVDRe5WT5oPI143YWCvZ1hO/y36vMlkCa2t9p\n"
                                   "trPkYfVSSmXthUw48T+GEhQyOkkbLp7i17I+IlO6mbYrItexsv3dBUQ2JKyu7v2C\n"
                                   "mu422O3cKlr3oySvI5JC7jAsX0lU2J5mXuobdHcoHE3upgNVoJb5Y1ZK7lYExf8V\n"
                                   "psj3QJjPHLAAvMEAvo+uYHFNnuH+ROFKoH41+eDkHJ10HK9RThv70l+qjxoTxGb/\n"
                                   "KdKHkC99AgMBAAECggEAfzHrqWuLJHhnxBv6/U1LT30PG3HsH7XkwsqP8FmmCCH4\n"
                                   "fOPqfDxxnXmyGhRjGJvYw18jA5u2bwPH6rfDvRu+EnEUHkrXiIQx5J9bnuhvLVte\n"
                                   "2Y4xfALYmLsF/1g4OKdP4enY3p4/vq2GBuGCg++/Q74UpEldAcVE6Gk9pTZEYf75\n"
                                   "I6dOW24RUm5m0Wnd4csNeNLWbDR9K4nT5ETan74cTwKGts7Fj5p+0J6//rRpUp9J\n"
                                   "pVj1Y2VdjNYW9Cyy6evyCToVf5SmsUk3ETWiIlCjXyqApwE6JrL3JZc9cYck9Hi7\n"
                                   "4lRrTnWxVM+DMEAY6Wq0uRAnf9K9cKMCAhLyy5HvYQKBgQDtJNqIpo7+DACCHnVF\n"
                                   "l7LHSHkC+IKVEPi/Ss6cJiHvdrNYzg/pJrFlVLXlVuIibcZM86RlSNZFvjNLSC6y\n"
                                   "Usb8cKZDGusQW0UKQnCXPEE03LAnn94HbtjL/IdZic7Y1Bi762cfg18D++6rdVg4\n"
                                   "OLXquD62phzzbdVpflHKe9r+GQKBgQDDp+OgFBBScgNsuWZ6srp65hmhqsGgov5d\n"
                                   "+jYGuxleOgLztJoBIE37jagbBkSBh3LgcqzVVuWW182SIXvNXVxOY7CvuU3zUtzm\n"
                                   "c3uNE09gYsJLsFOFthfN4KU0rpBOp1O/HhDphvHZvK/m2m46siLytVB/OM9i+3K1\n"
                                   "I3CjDsghBQKBgQCHAGz8UD2lOLXtXWOLm8GmG4bwfwLv07qYRAD8+eRly6BsFNsP\n"
                                   "8vwYYLIwfla8HNxAII/dLSr2h0HyQIR89gzb11F/cEqThDH4ljw8jIveiSgrVqJy\n"
                                   "ftMixX9UJNTBsXTnPir10Lb9sKV9rI7tarN9jSf+mPBwUH2m0cfESuAvuQKBgApP\n"
                                   "yOFg7VNLCv8p7kq9LLLZebRZiYbr0Dgnvb0xEy8yI4GwnQU+LJ37Y0a5V9bzyowl\n"
                                   "vxInWEZQ7VasSRgTuY+wFVnNjFwsm9PRdkuDHwXXbqIKkvxTEoIZOtvB730VuIY0\n"
                                   "EXQdVpXBKgwiqviQ9uKPx8RMvDFdSzhcu8z8NHP5AoGAcRdzowQLH3OC92vyNqWf\n"
                                   "2GrjfgafL5JG1GBuNuyUHlvC8U38FojX0reps2Wp/2ioyA5YstMkCZz6m7SSRP6e\n"
                                   "uE/KaCx805ng2alfF0dPBrceJu5mf2o11K/OenUzL+4ZC4j31HCBfajOfWsQ6YzF\n"
                                   "BvIVOGJ3v3qxAWUrWipE2R4=\n"
                                   "-----END PRIVATE KEY-----\n";

const char *server_cert_pem_start = "-----BEGIN CERTIFICATE-----\n"
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
// Define client certificate
const char *root_ca_cert_pem_start = "-----BEGIN CERTIFICATE-----\n"
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

/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/
static void error_message_send(void);
static char *wifi_get_mac(void);
/***********************************************************************************************************************
 * Function Name:
 * Description  :
 * Arguments    : none
 * Return Value : none
 ***********************************************************************************************************************/
void mqtt_task_start(void)
{
    ESP_LOGI(TAG, "----------------------INIT MQTT-------------------------------");
    mqtt_app_start(); // init mqtt connect to AWS
    //relay_init();
    // xTaskCreatePinnedToCore(mqtt_send_task, "mqtt_send_task", 6 * 1024, NULL, 6| portPRIVILEGE_BIT, NULL, 1);
}
/***********************************************************************************************************************
 * Static Functions
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * Function Name:
 * Description  :
 * Arguments    : none
 * Return Value : none
 ***********************************************************************************************************************/
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}
static void mqtt_app_start(void)
{
    // ESP_ERROR_CHECK( heap_trace_init_standalone(trace_record, NUM_RECORDS) );
    // ESP_ERROR_CHECK( heap_trace_start(HEAP_TRACE_LEAKS) );
    flash_client_id_read();
    // flash_client_cert_pem_read();
    // flash_client_private_pem_read();
    ESP_LOGI(TAG, "client_id length = %d", strlen(mqtt_config.client_id));
    ESP_LOGI(TAG, "certificate_pem length = %d", strlen(mqtt_config.certificate_pem));
    ESP_LOGI(TAG, "private_pem length = %d", strlen(mqtt_config.private_pem));

    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtts://a3qnicqxfi1gf7-ats.iot.ap-southeast-1.amazonaws.com:8883",
        .client_cert_pem = (const char *)mqtt_config.certificate_pem,
        // .client_cert_len = strlen(mqtt_config.certificate_pem),
        .client_key_pem = (const char *)mqtt_config.private_pem,
        // .client_key_len = strlen(mqtt_config.private_pem),
        .cert_pem = (const char *)server_cert_pem_start,
        .client_id = (const char *)mqtt_config.client_id,
        .disable_clean_session = true,
        // .use_global_ca_store = true,
         .keepalive = 60,
    };

    // memset(mqtt_cfg.client_id, 0x00, sizeof(MQTT_MAX_CLIENT_LEN));
    memset(mqtt_config.mqtt_topic_pub, 0x00, sizeof(mqtt_config.mqtt_topic_pub));
    memset(mqtt_config.mqtt_topic_pub_err, 0x00, sizeof(mqtt_config.mqtt_topic_pub_err));
    memset(mqtt_config.mqtt_topic_sub_job, 0x00, sizeof(mqtt_config.mqtt_topic_sub_job));
    memset(mqtt_config.mqtt_topic_pub_job_accept, 0x00, sizeof(mqtt_config.mqtt_topic_pub_job_accept));
    memset(mqtt_config.mqtt_topic_gw_add_sub, 0x00, sizeof(mqtt_config.mqtt_topic_gw_add_sub));
    memset(mqtt_config.mqtt_topic_gw_pub_dt, 0x00, sizeof(mqtt_config.mqtt_topic_gw_pub_dt));
    memset(mqtt_config.mqtt_topic_gw_pub_err, 0x00, sizeof(mqtt_config.mqtt_topic_gw_pub_err));
    memset(mqtt_config.mqtt_topic_gw_sub_job, 0x00, sizeof(mqtt_config.mqtt_topic_gw_sub_job));
    memset(mqtt_config.mqtt_topic_gw_pub_accept, 0x00, sizeof(mqtt_config.mqtt_topic_gw_pub_accept));

    //------------------ban PROD---------------------------------------------------------//

    sprintf(mqtt_config.mqtt_topic_pub, "stag/dt/6DVZ8TLF/%s", (char *)mqtt_config.client_id);
    sprintf(mqtt_config.mqtt_topic_pub_err, "stag/dt/6DVZ8TLF/%s/error", (char *)mqtt_config.client_id);
    sprintf(mqtt_config.mqtt_topic_sub_job, "stag/cmd/6DVZ8TLF/%s/accepted", (char *)mqtt_config.client_id);
    sprintf(mqtt_config.mqtt_topic_pub_job_accept, "stag/cmd/6DVZ8TLF/%s/status", (char *)mqtt_config.client_id);

    sprintf(mqtt_config.mqtt_topic_gw_add_sub, "stag/gw/6DVZ8TLF/%s/devices", (char *)mqtt_config.client_id);
    sprintf(mqtt_config.mqtt_topic_gw_pub_dt, "stag/gw/6DVZ8TLF/%s/dt", (char *)mqtt_config.client_id);
    sprintf(mqtt_config.mqtt_topic_gw_pub_err, "stag/gw/6DVZ8TLF/%s/dt/error", (char *)mqtt_config.client_id);
    sprintf(mqtt_config.mqtt_topic_gw_sub_job, "stag/gw/6DVZ8TLF/%s/cmd/accepted", (char *)mqtt_config.client_id);
    sprintf(mqtt_config.mqtt_topic_gw_pub_accept, "stag/gw/6DVZ8TLF/%s/cmd/status", (char *)mqtt_config.client_id);
    
//------------------------------------Ban DEV-----------------------------------------------------//
/*
    sprintf(mqtt_config.mqtt_topic_pub, "stag/dt/7RA38RA8/%s", (char *)mqtt_config.client_id);
    sprintf(mqtt_config.mqtt_topic_pub_err, "stag/dt/7RA38RA8/%s/error", (char *)mqtt_config.client_id);
    sprintf(mqtt_config.mqtt_topic_sub_job, "stag/cmd/7RA38RA8/%s/accepted", (char *)mqtt_config.client_id);
    sprintf(mqtt_config.mqtt_topic_pub_job_accept, "stag/cmd/7RA38RA8/%s/status", (char *)mqtt_config.client_id);

    sprintf(mqtt_config.mqtt_topic_gw_add_sub, "stag/gw/7RA38RA8/%s/devices", (char *)mqtt_config.client_id);
    sprintf(mqtt_config.mqtt_topic_gw_pub_dt, "stag/gw/7RA38RA8/%s/dt", (char *)mqtt_config.client_id);
    sprintf(mqtt_config.mqtt_topic_gw_pub_err, "stag/gw/7RA38RA8/%s/dt/error", (char *)mqtt_config.client_id);
    sprintf(mqtt_config.mqtt_topic_gw_sub_job, "stag/gw/7RA38RA8/%s/cmd/accepted", (char *)mqtt_config.client_id);
    sprintf(mqtt_config.mqtt_topic_gw_pub_accept, "stag/gw/7RA38RA8/%s/cmd/status", (char *)mqtt_config.client_id);
*/


    ESP_LOGI(TAG, "mqtt_cfg.client_id = %s", mqtt_cfg.client_id);
    ESP_LOGI(TAG, "mqtt_topic_pub = %s", mqtt_config.mqtt_topic_pub);
    ESP_LOGI(TAG, "mqtt_topic_pub_err = %s", mqtt_config.mqtt_topic_pub_err);
    ESP_LOGI(TAG, "mqtt_topic_sub_job = %s", mqtt_config.mqtt_topic_sub_job);
    ESP_LOGI(TAG, "mqtt_topic_pub_job_accept = %s", mqtt_config.mqtt_topic_pub_job_accept);
    ESP_LOGI(TAG, "mqtt_topic_gw_add_sub = %s", mqtt_config.mqtt_topic_gw_add_sub);
    ESP_LOGI(TAG, "mqtt_topic_gw_pub_dt = %s", mqtt_config.mqtt_topic_gw_pub_dt);
    ESP_LOGI(TAG, "mqtt_topic_gw_pub_err = %s", mqtt_config.mqtt_topic_gw_pub_err);
    ESP_LOGI(TAG, "mqtt_topic_gw_sub_job = %s", mqtt_config.mqtt_topic_gw_sub_job);
    ESP_LOGI(TAG, "mqtt_topic_gw_pub_accept = %s", mqtt_config.mqtt_topic_gw_pub_accept);

    //esp_mqtt_client_handle_t
    mqtt_handle = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_handle, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_handle);
    esp_mqtt_client_start(mqtt_handle);
    // client->config->task_stack, client, client->config->task_prio, &client->task_handle
    xTaskCreatePinnedToCore(mqtt_send_task, "mqtt_send_task", 8*1024, mqtt_handle, 6| portPRIVILEGE_BIT, NULL, 1);
    // ESP_LOGW(TAG, "[APP] Free memory: %d bytes --", esp_get_free_heap_size());
    ESP_LOGW(TAG, "Free memory: %d bytes %d --", esp_get_free_heap_size(), __LINE__);
    // ESP_ERROR_CHECK( heap_trace_stop() );
    // heap_trace_dump();
}
/***********************************************************************************************************************
 * Function Name:
 * Description  :
 * Arguments    : none
 * Return Value : none
 ***********************************************************************************************************************/

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    int msg_id = 0;
    // your_context_t *context = event->context;
    esp_mqtt_client_handle_t client = event->client;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        if(gateway_data.mqtt_status!= true)
        gateway_data.mqtt_status = true;
        mqtt_config.get_active_start = true;
        msg_id = esp_mqtt_client_subscribe(client, mqtt_config.mqtt_topic_sub_job, 0);
        ESP_LOGI(TAG, "sent subscribe mqtt_topic_sub_job successful, msg_id=%d", msg_id);
        // msg_id = esp_mqtt_client_subscribe(event->client, mqtt_config.mqtt_topic_gw_sub_job, 0);
        // ESP_LOGI(TAG, "sent subscribe mqtt_topic_gw_sub_job successful, msg_id=%d", msg_id);
        if(is_mqtt_first_boot == false)
        {
            data_event_t cmd1 = GW_FEEDBACK_FLAG;
			if (xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS) != pdPASS)
			{
				ESP_LOGE(TAG, "Failed to send %s", __func__);
			}

            if(gateway_data.user_reset)
            {
                data_event_t cmd1 = GW_RESET_SS_STATE;
                if (xQueueSend(data_process, &cmd1, 20 / portTICK_PERIOD_MS) != pdPASS)
                {
                    ESP_LOGE(TAG, "Failed to send %s", __func__);
                }
                ESP_LOGI(TAG, "User would like to reset sensors state");
                gateway_data.user_reset = false;
            }
            is_mqtt_first_boot = true;
        }
		ESP_LOGW(TAG, "Free memory: %d bytes %d ++", esp_get_free_heap_size(), __LINE__);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        gateway_data.mqtt_status = false;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // msg_id = esp_mqtt_client_publish(event->client, topic_pub, "data", 0, 0, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "RECEIVED DATA=%.*s", event->data_len, event->data);
        bool status = json_parser_gw_job((const char *)event->data, event->data_len);
        ESP_LOGI(TAG, "status = %d", status);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Last captured errno : %d (%s)", event->error_handle->esp_transport_sock_errno,
                        strerror(event->error_handle->esp_transport_sock_errno));
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        gateway_data.mqtt_status = false;
        break;
    case MQTT_EVENT_BEFORE_CONNECT:
        ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
        break;
    case MQTT_EVENT_DELETED:
        ESP_LOGI(TAG, "MQTT_EVENT_DELETED");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
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
uint32_t previous_time_start = 0;
uint8_t state_send_data = 0;
bool time_inter_val_send = false;
static void mqtt_send_task(void *pvParameters)
{
    esp_mqtt_client_handle_t user_client = (esp_mqtt_client_handle_t) pvParameters;
    data_event_t msg_cmd_id = 0;
    int msg_id = 0;
    uint8_t c = 0;
    while (1)
    {
        if(gateway_data.mqtt_status)
        {
            if (xQueueReceive(data_process, &msg_cmd_id, portMAX_DELAY == pdPASS))
            {
                switch (msg_cmd_id)
                {
                    case GW_RESET_SS_STATE:
                    {
                        for (c = 1; c <= sensor_data.maxDetector; c++)
                        {
                            if((sensor_data.device_activated[c] == 1) && (sensor_data.dtor_sensor_state[c] == ALARM_ST))
                            {
                                sensor_data.dtor_sensor_state[c] = NORMAL_ST;
                                char *message_packet = (char *)malloc(MQTT_HEAP_SIZE * sizeof(char));
                                memset(message_packet, 0x00, MQTT_HEAP_SIZE * sizeof(char));
                                json_packet_message_sensor_data(message_packet, c);
                                msg_id = esp_mqtt_client_publish(user_client, mqtt_config.mqtt_topic_gw_pub_dt, message_packet, strlen(message_packet), QOS_PKG_IMPORTANT, 0);
                                ESP_LOGI(TAG, "[%d] topic [%s]",msg_id, mqtt_config.mqtt_topic_gw_pub_dt);
                                ESP_LOGI(TAG, "[%d] send : = %s", msg_id,  message_packet);
                                memset(message_packet, 0x00, MQTT_HEAP_SIZE * sizeof(char));
                                free(message_packet);
                                flash_save_sensor_data();// After success register save data
                            }
                            ESP_LOGI(TAG, "[%d] heap size = %d", msg_id, esp_get_free_heap_size());
                            vTaskDelay(20 / portTICK_PERIOD_MS);
                        }
                    }
                    break;
                    case SS_FEEDBACK_FLAG:
                    {
                        ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
                        //update_relay_alarm_state(sensor_data.curDetector);
                        char *message_packet = (char *)malloc(MQTT_HEAP_SIZE * sizeof(char));
                        memset(message_packet, 0x00, MQTT_HEAP_SIZE * sizeof(char));
                        json_packet_message_sensor_data(message_packet, sensor_data.curDetector);
                        // char message_packet[2000] = "\"request_id\":\"d945696ac932472bb204ad61f5b3dd0e\",\"sub_id\":\"010.010.106.029\",\"states\":{\"fire_sensor_state\":\"normal\",\"battery_state\":\"high\"}";
                        msg_id = esp_mqtt_client_publish(user_client, mqtt_config.mqtt_topic_gw_pub_dt, message_packet, strlen(message_packet), QOS_PKG_IMPORTANT, 0);
                        // msg_id = esp_mqtt_client_enqueue(user_client, mqtt_config.mqtt_topic_gw_pub_dt, message_packet, strlen(message_packet), QOS_PKG_IMPORTANT, 0, true);
                        ESP_LOGI(TAG, "[%d] topic [%s]",msg_id, mqtt_config.mqtt_topic_gw_pub_dt);
                        ESP_LOGI(TAG, "[%d] send : = %s", msg_id,  message_packet);
                        memset(message_packet, 0x00, MQTT_HEAP_SIZE * sizeof(char));
                        free(message_packet);
                        flash_save_sensor_data();// After success register save data
                        ESP_LOGI(TAG, "[%d] heap size = %d", msg_id, esp_get_free_heap_size());
                    }
                    break;
                    case GW_FEEDBACK_FLAG:
                    {
                        char *message_packet = (char *)malloc(MQTT_HEAP_SIZE * sizeof(char));
                        memset(message_packet, 0x00, MQTT_HEAP_SIZE * sizeof(char));
                        json_packet_message_gateway_data(message_packet);
                        msg_id = esp_mqtt_client_publish(user_client, mqtt_config.mqtt_topic_pub, message_packet, strlen(message_packet), QOS_PKG_IMPORTANT, 0);
                        // msg_id = esp_mqtt_client_enqueue(user_client, mqtt_config.mqtt_topic_pub, message_packet, strlen(message_packet), QOS_PKG_IMPORTANT, 0, true);
                        ESP_LOGI(TAG, "[%d] topic [%s]",msg_id, mqtt_config.mqtt_topic_pub);
                        ESP_LOGI(TAG, "[%d] send : = %s",msg_id,  message_packet);
                        memset(message_packet, 0x00, MQTT_HEAP_SIZE * sizeof(char));
                        free(message_packet);
                        ESP_LOGI(TAG, "[%d] heap size = %d", msg_id, esp_get_free_heap_size());
                    }
                    break;
                    case JOB_REQUESTED_FEEDBACK_FLAG:
                    {
                        char *message_packet = (char *)malloc(MQTT_HEAP_SIZE * sizeof(char));
                        memset(message_packet, 0x00, MQTT_HEAP_SIZE * sizeof(char));
                        json_packet_message_job_requested_feedback(message_packet);
                        msg_id = esp_mqtt_client_publish(user_client, mqtt_config.mqtt_topic_pub_job_accept, message_packet, strlen(message_packet), QOS_PKG_IMPORTANT, 0);
                        // msg_id = esp_mqtt_client_enqueue(user_client, mqtt_config.mqtt_topic_pub_job_accept, message_packet, strlen(message_packet), QOS_PKG_IMPORTANT, 0, true);
                        ESP_LOGI(TAG, "[%d] topic [%s]", msg_id, mqtt_config.mqtt_topic_pub_job_accept);
                        ESP_LOGI(TAG, "[%d] send : = %s",msg_id,  message_packet);
                        memset(message_packet, 0x00, MQTT_HEAP_SIZE * sizeof(char));
                        free(message_packet);
                        ESP_LOGI(TAG, "[%d] heap size = %d", msg_id, esp_get_free_heap_size());
                    }
                    break;
                    case SS_REGISTER_SERVER:
                    {
                        char *message_packet = (char *)malloc(MQTT_HEAP_SIZE * sizeof(char));
                        memset(message_packet, 0x00, MQTT_HEAP_SIZE * sizeof(char));
                        json_packet_message_fb_ss_process(message_packet, sensor_data.curDetector);
                        msg_id = esp_mqtt_client_publish(user_client, mqtt_config.mqtt_topic_gw_add_sub, message_packet, strlen(message_packet), QOS_PKG_IMPORTANT, 0);
                        // msg_id = esp_mqtt_client_enqueue(user_client, mqtt_config.mqtt_topic_gw_add_sub, message_packet, strlen(message_packet), QOS_PKG_IMPORTANT, 0, true);
                        ESP_LOGI(TAG, "[%d] topic [%s]", msg_id, mqtt_config.mqtt_topic_gw_add_sub);
                        ESP_LOGI(TAG, "[%d] send : = %s",msg_id,  message_packet);
                        memset(message_packet, 0x00, MQTT_HEAP_SIZE * sizeof(char));
                        free(message_packet);
                        ESP_LOGI(TAG, "[%d] heap size = %d", msg_id, esp_get_free_heap_size());
                    }
                    break;
                    case SS_UPLOAD_DB: //reserved
                    {
                        for (c = 1; c <= sensor_data.maxDetector; c++)
                        {
                            if(sensor_data.device_activated[c] == 1)
                            {
                                char *message_packet = (char *)malloc(MQTT_HEAP_SIZE * sizeof(char));
                                memset(message_packet, 0x00, MQTT_HEAP_SIZE * sizeof(char));
                                json_packet_message_sensor_data(message_packet, c);
                                msg_id = esp_mqtt_client_publish(user_client, mqtt_config.mqtt_topic_gw_pub_dt, message_packet, strlen(message_packet), QOS_PKG_IMPORTANT, 0);
                                // msg_id = esp_mqtt_client_enqueue(user_client, mqtt_config.mqtt_topic_gw_pub_dt, message_packet, strlen(message_packet), QOS_PKG_IMPORTANT, 0, true);
                                ESP_LOGI(TAG, "[%d] topic [%s]",msg_id, mqtt_config.mqtt_topic_gw_pub_dt);
                                ESP_LOGI(TAG, "[%d] send : = %s", msg_id,  message_packet);
                                memset(message_packet, 0x00, MQTT_HEAP_SIZE * sizeof(char));
                                free(message_packet);
                                flash_save_sensor_data();// After success register save data
                            }
                            ESP_LOGI(TAG, "[%d] heap size = %d", msg_id, esp_get_free_heap_size());
                        }
                    }
                    break;
                    case JOB_REQUESTED_OTA:
                    {
                        esp_mqtt_client_destroy(user_client);
                        gateway_data.mqtt_status = false;
                        ESP_LOGI(TAG, "Destroy mqtt heap size = %d", esp_get_free_heap_size());
                    }
                    break;
                    case SS_ERROR_FLAG:
                    {
                        char *message_packet = (char *)malloc(MQTT_HEAP_SIZE * sizeof(char));
                        memset(message_packet, 0x00, MQTT_HEAP_SIZE * sizeof(char));
                        json_packet_message_sensor_error(message_packet, sensor_data.curDetector);
                        msg_id = esp_mqtt_client_publish(user_client, mqtt_config.mqtt_topic_gw_pub_err, message_packet, strlen(message_packet), QOS_PKG_IMPORTANT, 0);
                        ESP_LOGI(TAG, "[%d] topic [%s]",msg_id, mqtt_config.mqtt_topic_gw_pub_err);
                        ESP_LOGI(TAG, "[%d] send : = %s",msg_id,  message_packet);
                        memset(message_packet, 0x00, MQTT_HEAP_SIZE * sizeof(char));
                        free(message_packet);
                        ESP_LOGI(TAG, "[%d] heap size = %d", msg_id, esp_get_free_heap_size());
                    }
                    break;
                    default:
                    {
                        ESP_LOGI(TAG, "Unknown msg_cmd_id = %d",msg_cmd_id);
                    }
                    break;
                }
            }
        }
        else if (gateway_data.mqtt_status == false)
        {
            //ESP_LOGE(TAG, "Network connect failed");
        }
        else
        {
            ESP_LOGW(TAG, "Do nothing !!!");
	        ESP_LOGW(TAG, " Free memory: %d bytes func %s line %d ++", esp_get_free_heap_size(), __func__, __LINE__);
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}
/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/
uint32_t previous_time_error = 0;
uint8_t state_send_error = 0;
static void error_message_send(void)
{
    int msg_id;
    switch (state_send_error)
    {
    case 0:
        previous_time_error = usertimer_gettick();
        state_send_error = 1;
        break;
    case 1:
        state_send_error = 0;
        // if (gateway_data.sensor.dust_error)
        // {
        //     if (usertimer_gettick() - previous_time_error > 5000)
        //     {
        //         char *message_packet = (char *)malloc(200 * sizeof(char));
        //         memset(message_packet, 0x00, 200 * sizeof(char));
        //         json_packet_message_error(message_packet, 1);
        //         msg_id = esp_mqtt_client_publish(s_client, mqtt_config.mqtt_topic_pub_err, message_packet, strlen(message_packet), 0, 0);
        //         memset(message_packet, 0x00, 200 * sizeof(char));
        //         free(message_packet);

        //         previous_time_error = usertimer_gettick();
        //         state_send_error = 2;
        //     }
        // }
        // else
        //     state_send_error = 2;
        break;
    default:
        break;
    }
}

static char *wifi_get_mac(void)
{
    char *mac_add;
    mac_add = (char *)malloc(18 + 1);
#if 1
    // Get the derived MAC address for each network interface
    uint8_t derived_mac_addr[6] = {0};
    // Get MAC address for WiFi Station interface
    ESP_ERROR_CHECK(esp_read_mac(derived_mac_addr, ESP_MAC_WIFI_STA));
#else
    uint8_t derived_mac_addr[6] = {0x97, 0xcd, 0xac, 0xbf, 0xa9, 0x97};
#endif
    sprintf(mac_add, "%02x:%02x:%02x:%02x:%02x:%02x", derived_mac_addr[0], derived_mac_addr[1], derived_mac_addr[2], derived_mac_addr[3],
            derived_mac_addr[4], derived_mac_addr[5]);
    ESP_LOGD(TAG, "wifi_get_mac end = %s", mac_add);
    return mac_add;
}

char cer_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0}; // Buffer to store response of http request
char cer_link[MAX_HTTP_OUTPUT_BUFFER] = {0};   // Buffer to store response of http request
uint16_t length = 0;
uint8_t ignore_first_message = false;
esp_err_t _http_event_handle(esp_http_client_event_t *evt)
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
        // printf("HTTP_EVENT_ON_HEADER %.*s\n", evt->data_len, (char *)evt->data);
        // length = sprintf(cer_link + length, "%.*s", evt->data_len, (char *)evt->data);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        printf("%.*s\n", evt->data_len, (char *)evt->data);
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            if (mqtt_get_state == E_MQTT_GET_CER_LINK)
            {
                if (ignore_first_message == true)
                {
                    length += sprintf(cer_link + length, "%.*s", evt->data_len, (char *)evt->data);
                }
                else
                {
                    ignore_first_message = true;
                    // printf("ignore_first %.*s", evt->data_len, (char *)evt->data);
                }
            }
            else if (mqtt_get_state != E_MQTT_GET_CER_LINK)
            {
                length += sprintf(cer_buffer + length, "%.*s", evt->data_len, (char *)evt->data);
            }
        }
        else
        {
            length += sprintf(cer_link + length, "%.*s", evt->data_len, (char *)evt->data);
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");

        // memset(cer_link, 0x00, sizeof(cer_link));
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED = %d", length);
        if (mqtt_get_state == E_MQTT_GET_CER_LINK)
            page_not_found = json_parser_certificate(cer_link, length);
        length = 0;
        break;
    }
    return ESP_OK;
}

bool mqtt_get_cer(void)
{
    bool get_cer_status = true;
    esp_http_client_handle_t client_2;
    ESP_LOGI(TAG, "-------------mqtt_get_cer");
    esp_http_client_config_t config = {
        .url = "https://api.iot.elifeup.com/iot-thing/active",
        .method = HTTP_METHOD_POST,
        .event_handler = _http_event_handle,
        .cert_pem = (const char *)root_ca_cert_pem_start,
        .timeout_ms = 50000,
    };
    client_2 = esp_http_client_init(&config);
    // POST Request
    char *post_data;
    post_data = (char *)malloc(200 + 1);
    // strcpy((char *) wifi_author.mPairToken , "E5ORCC23A5B7VI8SXF2EAQZ98J2FUHW1");//temp
   sprintf(post_data, "{\"product_code\":\"fcomt8001\",\"pair_token\":\"%s\",\"serial_number\":\"%s\",\"hash_firmware\":\"%s\"}",
                                                                                                        wifi_author.mPairToken,
                                                                                                        wifi_get_mac(),
                                                                                                        gateway_data.fw_info);
                                                                                                        /*
sprintf(post_data, "{\"product_code\":\"dev-fcom001\",\"pair_token\":\"%s\",\"serial_number\":\"%s\",\"hash_firmware\":\"%s\"}",
                                                                                                        wifi_author.mPairToken,
                                                                                                        wifi_get_mac(),
                                                                                                        gateway_data.fw_info);*/ 
    ESP_LOGI(TAG, "News [%s]", post_data);
    esp_http_client_set_post_field(client_2, post_data, strlen(post_data));
    esp_http_client_set_header(client_2, "X-Api-Key", "5hNuOgmEKX4UzUwZBoPGl5UOVmwA4LLd");
    esp_http_client_set_header(client_2, "Content-Type", "application/json");
    esp_err_t err = esp_http_client_perform(client_2);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTPS Status = %d", esp_http_client_get_status_code(client_2));
    }
    else
    {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
        get_cer_status = false;
    }

    esp_http_client_cleanup(client_2);

    if (page_not_found == false)
        get_cer_status = false;

    free(post_data);

    if (get_cer_status == true)
    {
        flash_client_id_save();

        mqtt_get_state = E_MQTT_GET_CER_PEM_VALUE;
        get_cer_status = mqtt_get_cer_pem_file();
    }

    if (get_cer_status == false)
        mqtt_get_state = E_MQTT_GET_CER_LINK;
    return get_cer_status;
}

bool mqtt_get_cer_pem_file(void)
{
    bool get_cer_pem_status = false;
    esp_http_client_handle_t client_2;
    ESP_LOGI(TAG, "-------------mqtt_get_cer_file");
    esp_http_client_config_t config = {
        .event_handler = _http_event_handle,
        .url = "https://api.iot.elifeup.com/iot-thing/active",
        .timeout_ms = 50000,
    };
    client_2 = esp_http_client_init(&config);
    if (mqtt_config.certificate_pem_link != NULL)
    {

        esp_http_client_set_url(client_2, mqtt_config.certificate_pem_link);
        printf("%.*s", strlen(mqtt_config.certificate_pem_link), mqtt_config.certificate_pem_link);
    }

    esp_err_t err = esp_http_client_perform(client_2);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "get from server and save certificate_pem");
        memset(mqtt_config.certificate_pem, 0x00, sizeof(mqtt_config.certificate_pem));
        sprintf(mqtt_config.certificate_pem, "%.*s\n", length,
                (char *)cer_buffer);
        flash_client_cert_pem_save();
        esp_http_client_cleanup(client_2);
        mqtt_get_state = E_MQTT_GET_PRIVATE_PEM_VALUE;
    }
    else
    {
        ESP_LOGI(TAG, "unable download file , aborting update firmware.... \n");
        // restart get cer
        get_cer_pem_status = false;
        esp_http_client_cleanup(client_2);
        return get_cer_pem_status;
    }

    get_cer_pem_status = mqtt_get_private_pem_file();
    return get_cer_pem_status;
}

bool mqtt_get_private_pem_file(void)
{
    bool get_private_pem_status = false;
    esp_http_client_handle_t client_2;
    ESP_LOGI(TAG, "-------------mqtt_get_cer_file");
    esp_http_client_config_t config = {
        .event_handler = _http_event_handle,
        .url = "https://api.iot.elifeup.com/iot-thing/active",
        .timeout_ms = 50000,
    };
    client_2 = esp_http_client_init(&config);
    if (mqtt_config.certificate_pem_link != NULL)
    {

        esp_http_client_set_url(client_2, mqtt_config.private_pem_link);
        printf("%.*s", strlen(mqtt_config.private_pem_link), mqtt_config.private_pem_link);
    }

    esp_err_t err = esp_http_client_perform(client_2);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "get from server and save private_pem");
        memset(mqtt_config.private_pem, 0x00, sizeof(mqtt_config.private_pem));
        sprintf(mqtt_config.private_pem, "%.*s\n", length,
                (char *)cer_buffer);
        flash_client_private_pem_save();
        get_private_pem_status = true;
    }
    else
    {
        ESP_LOGI(TAG, "unable download file , aborting update firmware.... \n");
        // restart get cer
        get_private_pem_status = false;
    }
    esp_http_client_cleanup(client_2);
    mqtt_get_state = E_MQTT_GET_CER_LINK;
    return get_private_pem_status;
}




