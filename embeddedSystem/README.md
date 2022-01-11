
build:
```
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=amazon-freertos/tools/cmake/toolchains/xtensa-esp32.cmake -GNinja
```

flash:
```
cmake --build build --target flash
```

monitor output:
```
export ESPPORT <replace-with-serial-port-of-ESP32>
amazon-freertos/vendors/espressif/esp-idf/tools/idf.py monitor
```