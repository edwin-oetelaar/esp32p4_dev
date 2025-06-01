// ssd1306.h
// This header is designed for use with ESP-IDF I2C master APIs
// Edwin vd Oetelaar, december 2024
#pragma once

#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C"
{
#endif

// /* smart arrays size macros, from linux kernel */
// #define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))
// /* &a[0] degrades to a pointer: a different type from an array */
// #define __must_be_array(a) BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))
// #define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
// #define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int : (-!!(e)); }))

#include "driver/i2c_master.h"
#define SSD1306_I2C_ADDRESS (0x3c) // Default I2C address

    // SSD1306 device descriptor
    typedef struct
    {
        uint16_t device_address;            // I2C device address (2 bytes)
        i2c_master_bus_handle_t bus_handle; // I2C bus handle (assumed pointer, 4 bytes)
        // i2c_master_dev_handle_t i2c_port;    // I2C port handle (assumed pointer, 4 bytes)
        // i2c_master_dev_handle_t *dev_handle; // Pointer to device on I2C bus (4 bytes)
        i2c_master_dev_handle_t dev_handle;
        uint32_t scl_speed_hz; // I2C clock frequency (4 bytes)
        uint8_t width;         // Display width in pixels (1 byte)
        uint8_t height;        // Display height in pixels (1 byte)
        uint8_t pages;         // Total pages (1 byte)
        uint8_t external_vcc;  // External VCC flag (1 byte)

        uint8_t buffer[1024]; // Pixel buffer (1024 bytes)
    } ssd1306_handle_t;

    // Initialization/free function
    void ssd1306_init(ssd1306_handle_t *dev, uint8_t width, uint8_t height, uint8_t external_vcc);
    void ssd1306_deinit(ssd1306_handle_t *dev); // Free the device handle and memory

    // Basic commands
    void ssd1306_poweroff(ssd1306_handle_t *dev);
    void ssd1306_poweron(ssd1306_handle_t *dev);
    void ssd1306_contrast(ssd1306_handle_t *dev, uint8_t contrast);
    void ssd1306_invert(ssd1306_handle_t *dev, uint8_t invert);

    // Drawing / updating
    void ssd1306_fill(ssd1306_handle_t *dev, uint8_t color);
    void ssd1306_set_pixel(ssd1306_handle_t *dev, unsigned int x, unsigned int y, uint8_t color);

    // Copy bitmap from buffer to device (blit)
    void ssd1306_show(ssd1306_handle_t *dev);
    uint8_t ssd1306_printFixed6(ssd1306_handle_t *dev, uint8_t xpos, uint8_t y, uint8_t color, const char *str);
    uint8_t ssd1306_printFixed8(ssd1306_handle_t *dev, uint8_t xpos, uint8_t ypos, uint8_t color, const char *str);
    uint8_t ssd1306_printFixed16(ssd1306_handle_t *dev, uint8_t xpos, uint8_t ypos, uint8_t color, const char *str);
    // Low-level I2C write
    // static void ssd1306_write_cmd(ssd1306_handle_t *dev, uint8_t cmd);
    // static void ssd1306_write_data(ssd1306_handle_t *dev, const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif
