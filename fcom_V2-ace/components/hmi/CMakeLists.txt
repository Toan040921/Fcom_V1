set(app_src hmi_service.c)
set(pri_req peripheral user_driver)
idf_component_register(SRCS ${app_src}
                    INCLUDE_DIRS "."
                    REQUIRES ${pri_req})
