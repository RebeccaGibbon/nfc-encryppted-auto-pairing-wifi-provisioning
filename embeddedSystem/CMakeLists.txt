cmake_minimum_required(VERSION 3.13)

set(esp_idf_dir "${CMAKE_CURRENT_LIST_DIR}/amazon-freertos/vendors/espressif/esp-idf")
include(${esp_idf_dir}/tools/cmake/idf.cmake)
string(FIND "${CMAKE_TOOLCHAIN_FILE}" "esp32" SOC_TOOLCHAIN_ESP32)

project(ble_nfc_app)

# Tell IDF build to link against this target.
set(IDF_PROJECT_EXECUTABLE ble_nfc_demo)
get_filename_component(
    IDF_EXECUTABLE_SRCS
    "src/spi_pn532.c" ABSOLUTE
    )

# As of now there's no offical way to redefine config files outside of FreeRTOS source tree.
# This is a temporary approach to inject an include path so that this takes precedence over the
# config file directory inside FreeRTOS.
include_directories(BEFORE config)


# AFR_BOARD tells which board to target.
set(AFR_BOARD espressif.esp32_devkitc CACHE INTERNAL "")
# Add amazon freertos as a subdirectory.
add_subdirectory(amazon-freertos)

# Link against the mqtt library so that we can use it. Dependencies are transitively
# linked.
target_link_libraries(
    ble_nfc_demo
    PRIVATE
    AFR::demo_core_mqtt
    AFR::ble
    AFR::ble_wifi_provisioning
    # idf::foo
)