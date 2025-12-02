# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(PROJECT_VER "1.0.3")
set(PROJECT_VERU ${PROJECT_VER})
set(PROJECT_PREF "gateway1")

string (REPLACE "\." "_" PROJECT_VERU ${PROJECT_VERU})
set(PROJECT_BIN "${PROJECT_PREF}_${PROJECT_VERU}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# (Not part of the boilerplate)
# This example uses an extra component for common functions such as Wi-Fi and Ethernet connection.
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/components)

project(gateway_rf_mqtt)
