#!/bin/bash

source amazon-freertos/vendors/espressif/esp-idf/export.sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=amazon-freertos/tools/cmake/toolchains/xtensa-esp32.cmake -GNinja
idf.py build
# idf.py erase_flash
# idf.py flash monitor