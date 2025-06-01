// ssd1306.c
// for use with ESP-IDF I2C master APIs
// Edwin vd Oetelaar, december 2024

#include <stdint.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <string.h> // String manipulation functions
// ESP-IDF includes for error reporting, logging, and system functions
#include "esp_err.h"           // ESP-IDF error codes
#include "esp_log.h"           // Logging functionality
#include "driver/i2c_master.h" // I2C master configuration and communication
#include "ssd1306.h"           // SSD1306 OLED display driver
#include <ssd1306_fonts.h>

#include "font_petme128_8x8.h"
// Logging tag for ESP-IDF
static const char *TAG = "SSD1306";

// Register definitions and SSD1306 commands as an enum
typedef enum
{
    SET_CONTRAST = 0x81,        // Set display contrast, 2 byte command
    SET_ENTIRE_ON = 0xA4,       // Set entire display ON
    SET_NORM_INV = 0xA6,        // Set normal 0xA6 or inverse 0xA7 display
    SET_DISP = 0xAE,            // Set display to ON (0xAF) or OFF (0xAE)
    SET_MEM_ADDR = 0x20,        // Set memory addressing mode (2 byte command)
    SET_COL_ADDR = 0x21,        // Set column address (2 byte command)
    SET_PAGE_ADDR = 0x22,       // Set page address (3 byte command)
    SET_DISP_START_LINE = 0x40, // Set display start line
    SET_SEG_REMAP = 0xA0,       // Set segment re-map
    SET_MUX_RATIO = 0xA8,       // Set multiplex ratio (2 byte command)
    SET_COM_OUT_DIR = 0xC0,     // Set COM output scan direction
    SET_DISP_OFFSET = 0xD3,     // Set display offset (2 byte command)
    SET_COM_PIN_CFG = 0xDA,     // Set COM pin configuration (2 byte command)
    SET_DISP_CLK_DIV = 0xD5,    // Set display clock divide ratio (2 byte command)
    SET_PRECHARGE = 0xD9,       // Set pre-charge period (2 byte command)
    SET_VCOM_DESEL = 0xDB,      // Set VCOM deselect level (2 byte command)
    SET_CHARGE_PUMP = 0x8D      // Set charge pump setting
} SSD1306_Command;

// LOW-LEVEL I2C WRITES
static void ssd1306_write_cmd(ssd1306_handle_t *handle, uint8_t cmd)
{
    // Control byte: Co=1 (bit 7), D/C#=0 (bit 6)
    // 0x80 is 1000 0000
    uint8_t data[2];
    data[0] = 0x80; // Indicate we're sending a command
    data[1] = cmd;
    esp_err_t ret = i2c_master_transmit(handle->dev_handle, data, sizeof(data), pdMS_TO_TICKS(1000));

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error writing command 0x%02X", cmd);
    }
}

#define MAX_CHUNK_SIZE 32 // Maximum size of each data chunk

static esp_err_t write_data_chunk(ssd1306_handle_t *handle, const uint8_t *data, size_t chunk_size)
{
    uint8_t buf[MAX_CHUNK_SIZE + 1];   // 32 bytes of data + 1 byte for the control byte
    buf[0] = 0x40;                     // Indicate we're sending data
    memcpy(buf + 1, data, chunk_size); // Copy chunk data into buffer

    // Transmit data over I2C
    return i2c_master_transmit(handle->dev_handle, buf, chunk_size + 1, pdMS_TO_TICKS(1000));
}

static void ssd1306_write_data(ssd1306_handle_t *handle, const uint8_t *data, size_t length)
{
    size_t remaining_length = length; // Number of bytes left to write
    size_t written = 0;

    while (remaining_length > 0)
    {
        size_t chunk_size = (remaining_length > MAX_CHUNK_SIZE) ? MAX_CHUNK_SIZE : remaining_length;

        // Write the current chunk
        esp_err_t ret = write_data_chunk(handle, data + written, chunk_size);
        //   ESP_LOGE(TAG, "Writing chunk of size %d, remaining %d bytes", chunk_size, remaining_length);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Error writing data block");
            break;
        }

        written += chunk_size;
        remaining_length -= chunk_size;
    }
}

// BASIC INITIALIZATION
void ssd1306_init(ssd1306_handle_t *dev, uint8_t width, uint8_t height, uint8_t external_vcc)
{
    assert(dev != NULL);

    dev->width = width;
    dev->height = height;
    dev->pages = height / 8;

    dev->external_vcc = external_vcc;

    // attach the I2C device handle
    i2c_device_config_t i2c_dev_conf = {
        .scl_speed_hz = dev->scl_speed_hz,     // Set the I2C clock frequency
        .device_address = dev->device_address, // Set the device address
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, // Use 7-bit addressing
    };
    // Add the device to the I2C bus
    esp_err_t ret = i2c_master_bus_add_device(dev->bus_handle, &i2c_dev_conf, &dev->dev_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(ret));
        return;
    }
    // Initialize the buffer to zero
    memset(dev->buffer, 0, sizeof(dev->buffer));
    // Initialize the SSD1306 display

    ssd1306_write_cmd(dev, SET_DISP | 0x00); // display off
    ssd1306_write_cmd(dev, SET_MEM_ADDR);
    ssd1306_write_cmd(dev, 0x00); // horizontal addressing mode

    ssd1306_write_cmd(dev, SET_DISP_START_LINE | 0x00);
    ssd1306_write_cmd(dev, SET_SEG_REMAP | 0x01);
    ssd1306_write_cmd(dev, SET_MUX_RATIO);
    ssd1306_write_cmd(dev, dev->height - 1);

    ssd1306_write_cmd(dev, SET_COM_OUT_DIR | 0x08);
    ssd1306_write_cmd(dev, SET_DISP_OFFSET);
    ssd1306_write_cmd(dev, 0x00);
    ssd1306_write_cmd(dev, SET_COM_PIN_CFG);
    ssd1306_write_cmd(dev, (width > 2 * height) ? 0x02 : 0x12);

    // timing & driving
    ssd1306_write_cmd(dev, SET_DISP_CLK_DIV);
    ssd1306_write_cmd(dev, 0x80);
    ssd1306_write_cmd(dev, SET_PRECHARGE);
    ssd1306_write_cmd(dev, external_vcc ? 0x22 : 0xF1);
    ssd1306_write_cmd(dev, SET_VCOM_DESEL);
    ssd1306_write_cmd(dev, 0x30);

    // display
    ssd1306_write_cmd(dev, SET_CONTRAST);
    ssd1306_write_cmd(dev, 0x7F);
    ssd1306_write_cmd(dev, SET_ENTIRE_ON);
    ssd1306_write_cmd(dev, SET_NORM_INV);

    // charge pump
    ssd1306_write_cmd(dev, SET_CHARGE_PUMP);
    ssd1306_write_cmd(dev, external_vcc ? 0x10 : 0x14);

    // turn on display
    ssd1306_write_cmd(dev, SET_DISP | 0x01);

    // Clear the screen
    ssd1306_fill(dev, 0x00);
    ssd1306_show(dev);
    ESP_LOGI(TAG, "SSD1306 initialized, W=%d, H=%d", width, height);
}

void ssd1306_deinit(ssd1306_handle_t *dev)
{
    assert(dev != NULL);
    // Power off the display
    // ssd1306_poweroff(dev);
    // Clear the buffer
    memset(dev->buffer, 0, sizeof(dev->buffer));
    // Free the device handle if needed (not shown here)
    if (i2c_master_bus_rm_device(dev->dev_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to remove I2C device");
    }
    ESP_LOGI(TAG, "SSD1306 deinitialized");
}

// POWER CONTROL
void ssd1306_poweroff(ssd1306_handle_t *dev)
{
    assert(dev != 0);
    ssd1306_write_cmd(dev, SET_DISP | 0x00);
}

void ssd1306_poweron(ssd1306_handle_t *dev)
{
    assert(dev != 0);
    ssd1306_write_cmd(dev, SET_DISP | 0x01);
}

// CONTRAST AND INVERSION
void ssd1306_contrast(ssd1306_handle_t *dev, uint8_t contrast)
{
    assert(dev != 0);
    ssd1306_write_cmd(dev, SET_CONTRAST);
    ssd1306_write_cmd(dev, contrast);
}

void ssd1306_invert(ssd1306_handle_t *dev, uint8_t invert)
{
    assert(dev != 0);
    // invert can be 0 or 1
    ssd1306_write_cmd(dev, SET_NORM_INV | (invert & 0x01));
}

// BUFFER FILL
void ssd1306_fill(ssd1306_handle_t *dev, uint8_t color)
{
    assert(dev != 0);
    // Fill buffer with 0x00 or 0xFF (or pattern)
    memset(dev->buffer, (color ? 0xFF : 0x00), dev->pages * dev->width);
}

// set pixel on/off in buffer memory
void ssd1306_set_pixel(ssd1306_handle_t *dev, unsigned int x, unsigned int y, uint8_t color)
{
    assert(dev != NULL);
    /* Horizontal addressing mode maps to linear framebuffer */
    x &= 0x7f;
    y &= 0x3f;
    if (color)
        dev->buffer[((y & 0xf8) << 4) + x] |= 1 << (y & 7); // set bit
    else
        dev->buffer[((y & 0xf8) << 4) + x] &= ~(1 << (y & 7)); // unset bit
}

// copy buffer to i2c display (SHOW)
void ssd1306_show(ssd1306_handle_t *dev)
{
    assert(dev != NULL);

    uint8_t x0 = 0;
    uint8_t x1 = dev->width - 1;

    // Set column address
    ssd1306_write_cmd(dev, SET_COL_ADDR);
    ssd1306_write_cmd(dev, x0);
    ssd1306_write_cmd(dev, x1);

    // Set page address
    ssd1306_write_cmd(dev, SET_PAGE_ADDR);
    ssd1306_write_cmd(dev, 0);
    ssd1306_write_cmd(dev, dev->pages - 1);

    ssd1306_write_data(dev, dev->buffer, sizeof(dev->buffer)); // full buffer copy 1KByte data over i2c
}

// FONTS

#define FONT_CHAR_START 32
#define FONT_CHAR_OFFSET 4

static void draw_character(ssd1306_handle_t *dev, uint32_t xpos, uint32_t ypos, uint32_t color,
                           const uint8_t *char_data, uint32_t height, uint32_t width)
{
    for (uint32_t x_offset = 0; x_offset < width; x_offset++)
    {
        uint32_t column_data = (height > 8)
                                   ? (char_data[x_offset] | (char_data[x_offset + 8] << 8))
                                   : char_data[x_offset];
        uint32_t inverted_column = color ? column_data : ~column_data;

        for (uint32_t bit = 0; bit < height; bit++)
        {
            ssd1306_set_pixel(dev, xpos + x_offset, ypos + bit, (inverted_column >> bit) & 1);
        }
    }
}

uint8_t ssd1306_printFixed6(ssd1306_handle_t *dev, uint8_t xpos, uint8_t ypos, uint8_t color, const char *str)
{
    size_t str_len = strlen(str);  // Avoid recomputing strlen in the loop
    const uint32_t fontwidth = 6;  // 6 bytes per character
    const uint32_t fontheight = 8; // 8 bits per column
    for (size_t i = 0; i < str_len; i++)
    {
        char current_char = str[i];
        int32_t font_table_index = FONT_CHAR_OFFSET + (current_char - FONT_CHAR_START) * fontwidth;
        draw_character(dev, xpos + (i * fontwidth), ypos, color, &ssd1306xled_font6x8[font_table_index], fontheight, fontwidth);
    }
    return 0;
}

uint8_t ssd1306_printFixed16(ssd1306_handle_t *dev, uint8_t xpos, uint8_t ypos, uint8_t color, const char *str)
{
    size_t str_len = strlen(str);   // Avoid recomputing strlen in the loop
    const uint32_t fontwidth = 8;   // 8 bits per column
    const uint32_t fontheight = 16; // 16 bits per row, 16 bytes per character
    for (size_t i = 0; i < str_len; i++)
    {
        char current_char = str[i];
        int32_t font_table_index = FONT_CHAR_OFFSET + (current_char - FONT_CHAR_START) * fontheight;
        draw_character(dev, xpos + (i * fontwidth), ypos, color, &ssd1306xled_font8x16[font_table_index], fontheight, fontwidth);
    }
    return 0;
}

uint8_t ssd1306_printFixed8(ssd1306_handle_t *dev, uint8_t xpos, uint8_t ypos, uint8_t color, const char *str)
{
    const uint32_t fontwidth = 8;
    const uint32_t fontheight = 8;

    const int FONT_CHAR_MAX = 127; // Assuming ASCII
    int x_offset = xpos;

    size_t str_len = strlen(str);
    if (!str || str_len == 0)
    {
        return -1; // Error: Invalid input
    }

    for (size_t i = 0; i < str_len; i++)
    {
        char current_char = str[i];

        // Validate character is in valid range
        if (current_char < FONT_CHAR_START || current_char > FONT_CHAR_MAX)
        {
            continue; // Skip invalid characters
        }

        int32_t font_table_index = (current_char - FONT_CHAR_START) * fontheight;
        // Validate font table index
        if (font_table_index < 0 || font_table_index >= (sizeof(font_petme128_8x8)/sizeof(font_petme128_8x8[0])))
        {
            continue; // Skip if index out of bounds
        }

        draw_character(dev, x_offset, ypos, color,
                       &font_petme128_8x8[font_table_index],
                       fontheight, fontwidth);

        x_offset += fontwidth; // Increment x position more efficiently
    }
    return 0;
}

/* end */
