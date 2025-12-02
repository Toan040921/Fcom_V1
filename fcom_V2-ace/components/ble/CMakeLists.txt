set(app_src ble_service.c)
set(pri_req ble bt)
idf_component_register(SRCS ${app_src}
                    INCLUDE_DIRS "."
                    REQUIRES ${pri_req})
