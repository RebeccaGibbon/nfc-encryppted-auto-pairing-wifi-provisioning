cmake_minimum_required(VERSION 3.13)

project(ble_nfc_app)

add_executable(ble_nfc_demo src/main.c
                            src/spi_pn532.c
                            src/mqtt_runner.c
                            src/nfc_tasks.c)

# Tell IDF build to link against this target.
set(IDF_PROJECT_EXECUTABLE ble_nfc_demo)

# As of now there's no offical way to redefine config files outside of FreeRTOS source tree.
# This is a temporary approach to inject an include path so that this takes precedence over the
# config file directory inside FreeRTOS.
include_directories(BEFORE amazon-freertos-config)
include_directories(BEFORE include)

# AFR_BOARD tells which board to target.
set(AFR_BOARD espressif.esp32_devkitc CACHE INTERNAL "")
# Add amazon freertos as a subdirectory.
add_subdirectory(amazon-freertos)

# Link against the mqtt library so that we can use it. Dependencies are transitively
# linked.
target_link_libraries(ble_nfc_demo PRIVATE AFR::demo_core_mqtt
    AFR::ble
    AFR::demo_wifi_provisioning
    AFR::demo_numeric_comparison
    AFR::common_io
    AFR::wifi
)
