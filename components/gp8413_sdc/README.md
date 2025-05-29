# GP8413 DAC Driver

## Overview

This component provides a driver for the GP8413 Digital-to-Analog Converter (DAC) device. 
The GP8413 is a dual-channel, high-precision DAC that can be controlled via I2C interface.

## Features

- Support for dual-channel DAC control
- 5V and 10V output voltage range options
- Simple API for initialization and configuration
- Individual channel voltage control
- Optional EEPROM storage for settings

## Hardware Connection

The GP8413 communicates via I2C. Connect the device to your ESP32 as follows:

- SDA: Connect to the SDA pin of your ESP32 I2C bus
- SCL: Connect to the SCL pin of your ESP32 I2C bus
- VCC: Connect to 3.3V
- GND: Connect to GND

## Usage Example

```c
#include "gp8413_sdc.h"
#include "esp_log.h"

static const char *TAG = "GP8413_EXAMPLE";

void app_main(void)
{
    // Initialize I2C bus
    i2c_master_bus_handle_t bus_handle;
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = GPIO_NUM_22,
        .sda_io_num = GPIO_NUM_21,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    // Configure DAC
    gp8413_config_t dac_config = {
        .bus_handle = bus_handle,
        .device_addr = GP8413_I2C_ADDRESS,
        .output_range = GP8413_OUTPUT_RANGE_10V,
        .channel0 = {
            .voltage = 0,   // Start with 0V
            .enable = true, // Enable channel 0
        },
        .channel1 = {
            .voltage = 0,   // Start with 0V
            .enable = true, // Enable channel 1
        }
    };

    // Initialize DAC
    gp8413_handle_t *dac = gp8413_init(&dac_config);
    if (dac == NULL) {
        ESP_LOGE(TAG, "Failed to initialize GP8413 DAC");
        return;
    }

    ESP_LOGI(TAG, "GP8413 DAC initialized successfully");

    // Set voltages
    ESP_ERROR_CHECK(gp8413_set_output_voltage(dac, 5000, 0)); // 5V on channel 0
    ESP_ERROR_CHECK(gp8413_set_output_voltage(dac, 2500, 1)); // 2.5V on channel 1

    // Cleanup when done
    gp8413_deinit(&dac);
}
```

## API Reference

See the header file `gp8413_sdc.h` for the complete API documentation.

## License

MIT License. See the LICENSE file for details.
