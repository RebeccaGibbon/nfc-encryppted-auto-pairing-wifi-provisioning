// This is based on esp-idf's SPI Master example code
// https://github.com/espressif/esp-idf/blob/master/examples/peripherals/spi_master/lcd/main/spi_master_example_main.c

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

// Define pins for using SPI2 bus
#define PN532_HOST    HSPI_HOST
#define DMA_CHAN    2

#define PIN_NUM_MISO 12
#define PIN_NUM_MOSI 13
#define PIN_NUM_CLK  14 // SCK
#define PIN_NUM_CS   15 // SS


int main (void)
{
    esp_err_t ret;
    spi_device_handle_t spi;

    // Cnfigure the spi bus
    spi_bus_config_t buscfg = {
            .miso_io_num = PIN_NUM_MISO,
            .mosi_io_num = PIN_NUM_MOSI,
            .sclk_io_num = PIN_NUM_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1
    };
    
    // Configure the spi interface
    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .clock_speed_hz = 1000000,  // Clock out at 1 MHz
        .duty_cycle_pos = 128,
        .mode = 0,                  // spi mode 0
        .spics_io_num = PIN_NUM_CS, // SS pin
        .cs_ena_posttrans = 3,
        .cs_ena_pretrans = 0,
        .queue_size = 3
    };

    // Initialise spi bus
    ret = spi_bus_initialize(PN532_HOST, &buscfg, DMA_CHAN);
    ESP_ERROR_CHECK(ret);

    // Register device
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    // Interact with device

    return 0;
}