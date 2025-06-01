// m5_4relay.h
// This header is designed for use with ESP-IDF I2C master APIs
// Edwin vd Oetelaar, mei 2025
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "driver/i2c_master.h"

#define M54R_ADDR (0X26)
#define M54R_REG_MODE (0X10)
#define M54R_REG_RELAY (0X11)

    // M54R device descriptor
    typedef struct
    {
        uint16_t device_address;            // I2C device address (2 bytes)
        i2c_master_bus_handle_t bus_handle; // I2C bus handle (assumed pointer, 4 bytes)
        i2c_master_dev_handle_t dev_handle; // I2C device handle (assumed pointer, 4 bytes)
        uint32_t scl_speed_hz;              // I2C clock frequency (4 bytes)
        uint8_t initialized;                // Initialization flag (1 byte)
        uint8_t relay_state;                // Relay state (1 byte, bitmask for 4 relays)
        uint8_t led_state;                  // LED state (1 byte, bitmask for 4 LEDs)
        uint8_t mode;                       // Mode (1 byte, true = async, false = sync)
        // Additional fields can be added as needed
    } m54_ctx_t;

    // Initialization/free function
    esp_err_t m54_init(m54_ctx_t *dev);
    void m54_deinit(m54_ctx_t *dev); // Free the device handle and memory

    // Basic commands
    esp_err_t m54_relay_set(m54_ctx_t *dev, uint8_t number, uint8_t state);
    esp_err_t m54_relay_get(m54_ctx_t *dev, uint8_t number, uint8_t *state);
    // bulk relay commands
    esp_err_t m54_relay_set_all(m54_ctx_t *dev, uint8_t state);
    esp_err_t m54_relay_get_all(m54_ctx_t *dev, uint32_t *state);
    // LED commands
    esp_err_t m54_led_set(m54_ctx_t *dev, uint8_t number, uint8_t state);
    esp_err_t m54_led_get(m54_ctx_t *dev, uint8_t number, uint8_t *state);
    // bulk LED commands
    esp_err_t m54_led_set_all(m54_ctx_t *dev, uint8_t state);
    esp_err_t m54_led_get_all(m54_ctx_t *dev, uint32_t *state);
    // m54_mode_set/get
    // Set the mode of the device (e.g., AutoLed or ManualLed)
    esp_err_t m54_mode_set(m54_ctx_t *dev, uint8_t mode);
    esp_err_t m54_mode_get(m54_ctx_t *dev, uint8_t *mode);

#ifdef __cplusplus
}
#endif

// m5_4relay.h