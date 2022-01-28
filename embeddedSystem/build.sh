#!/bin/bash

source amazon-freertos/vendors/espressif/esp-idf/export.sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=amazon-freertos/tools/cmake/toolchains/xtensa-esp32.cmake -GNinja