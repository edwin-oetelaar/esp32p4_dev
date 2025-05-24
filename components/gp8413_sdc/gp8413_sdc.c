/*
 * GP8413 Device Driver Header
 *
 * Project: SDC2025
 * Author: Edwin van den Oetelaar (e.vandenoetelaar@fonyts.nl / edwin@oetelaar.com)
 * License: MIT
 *
 * Provides implementation for the GP8413 DAC device.
 */
#include "gp8413_sdc.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

#define TAG "GP8413_SDC"
#define GP8413_CHANNEL_MAX 2

#define I2C_TOOL_TIMEOUT_VALUE_MS (50)
static uint32_t i2c_frequency = 100 * 1000;

// Helper macro for input validation
#define CHECK_HANDLE(h)                 \
    do                                  \
    {                                   \
        if (!(h))                       \
            return ESP_ERR_INVALID_ARG; \
    } while (0)

#define CHECK_CHANNEL(ch)               \
    do                                  \
    {                                   \
        if ((ch) >= GP8413_CHANNEL_MAX) \
            return ESP_ERR_INVALID_ARG; \
    } while (0)

#define CHECK_RANGE(r)                                                       \
    do                                                                       \
    {                                                                        \
        if ((r) != GP8413_OUTPUT_RANGE_5V && (r) != GP8413_OUTPUT_RANGE_10V) \
            return ESP_ERR_INVALID_ARG;                                      \
    } while (0)


// Write data to the GP8413 device over I2C.
// This function creates a temporary I2C device handle for the transaction,
// sends the provided data buffer, and then removes the device handle.
// Returns ESP_OK on success, or an error code on failure.
static esp_err_t write_data_i2c(gp8413_handle_t *handle, uint8_t *data, size_t size)
{
    i2c_device_config_t i2c_dev_conf = {
        .scl_speed_hz = i2c_frequency,
        .device_address = handle->device_addr,
    };

    i2c_master_dev_handle_t dev_handle;

    if (i2c_master_bus_add_device(handle->bus_handle, &i2c_dev_conf, &dev_handle) != ESP_OK)
    {
        return ESP_FAIL;
    }

    esp_err_t ret = i2c_master_transmit(dev_handle, data, size, I2C_TOOL_TIMEOUT_VALUE_MS);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Write OK");
    }
    else if (ret == ESP_ERR_TIMEOUT)
    {
        ESP_LOGW(TAG, "Bus is busy");
    }
    else
    {
        ESP_LOGW(TAG, "Write Failed");
    }

    if (i2c_master_bus_rm_device(dev_handle) != ESP_OK)
    {
        return ESP_FAIL;
    }
    return ESP_OK;
}


// Initialize the GP8413 device and return a handle
gp8413_handle_t *gp8413_init(i2c_master_bus_handle_t bus_handle,
                             uint8_t device_addr,
                             gp8413_output_range_t output_range,
                             uint32_t voltage_ch0,
                             uint32_t voltage_ch1)
{
    // Check if bus_handle is valid
    if (!bus_handle)
    {
        ESP_LOGE(TAG, "No bus handle provided");
        return NULL; // no bus handle
    }

    // Allocate memory for the device handle
    gp8413_handle_t *handle = (gp8413_handle_t *)calloc(1, sizeof(gp8413_handle_t));

    if (!handle)
    {
        ESP_LOGE(TAG, "No memory for gp8413_handle_t");
        return NULL; // no memory
    }

    // Store initialization parameters in the handle
    handle->bus_handle = bus_handle;
    handle->device_addr = device_addr;
    handle->output_range = output_range;
    esp_err_t ret = gp8413_set_output_range(handle, output_range);
    if (ret != ESP_OK)
    {
        // If setting voltages fails, free the handle and return NULL
        free(handle);
        return NULL;
    }
    // Set initial output voltages for both channels
    ret = gp8413_set_output_voltage_dual(handle, voltage_ch0, voltage_ch1);
    if (ret != ESP_OK)
    {
        // If setting voltages fails, free the handle and return NULL
        free(handle);
        return NULL;
    }

    // Return the initialized handle
    return handle;
}

void gp8413_deinit(gp8413_handle_t **handle)
{
    if (handle && *handle)
    {
        // Optionally, store settings to device here
        // Example: gp8413_store_settings(*handle);

        // Free the handle memory
        free(*handle);
        *handle = NULL; // Set pointer to NULL to avoid dangling pointer
    }
}

esp_err_t gp8413_set_output_range(gp8413_handle_t *handle, gp8413_output_range_t range)
{
    CHECK_HANDLE(handle);
    CHECK_RANGE(range);

    handle->output_range = range;
    uint8_t reg = 0x01;        // register address for output range
    uint8_t range_code = 0x00; // default range code

    if (handle->output_range == GP8413_OUTPUT_RANGE_5V)
    {
        ESP_LOGI(TAG, "Output range set to 5V");
        range_code = 0x55; // 5V range code, not in datasheet!!
    }
    else if (handle->output_range == GP8413_OUTPUT_RANGE_10V)
    {
        ESP_LOGI(TAG, "Output range set to 10V");
        range_code = 0x77; // 10V range code, not in datasheet!!
    }
    else
    {
        ESP_LOGE(TAG, "Invalid output range");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2] = {reg, (uint8_t)range_code};
    ESP_LOGI(TAG, "Data to write: %02x %02x", data[0], data[1]);

    esp_err_t err = write_data_i2c(handle, data, sizeof(data));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set output range");
        return err;
    }
    else
    {
        ESP_LOGI(TAG, "Output range set to %d mV", handle->output_range);
    }

    return ESP_OK;
}

esp_err_t gp8413_set_output_voltage(gp8413_handle_t *handle, uint32_t voltage, uint32_t channel)
{
    CHECK_HANDLE(handle);
    CHECK_CHANNEL(channel);

    // Clamp voltage to range
    uint32_t max_mv = (uint32_t)handle->output_range;
    if (voltage > max_mv)
        voltage = max_mv;

    // Select register address for channel 0 or 1
    uint8_t data_addr = (channel == 0) ? 0x02 : 0x04;

    // Convert voltage to 16-bit word (0-32767)
    uint16_t word = (uint16_t)(32767 * voltage / max_mv);

    uint8_t data[3] = {
        data_addr,
        (uint8_t)(word & 0xFF),       // low byte
        (uint8_t)((word >> 8) & 0xFF) // high byte
    };

    ESP_LOGI(TAG, "Set output voltage to %d mV on channel %d", voltage, channel);
    ESP_LOGI(TAG, "Data to write: %02x %02x %02x", data[0], data[1], data[2]);

    return write_data_i2c(handle, data, sizeof(data));
}

esp_err_t gp8413_set_output_voltage_dual(gp8413_handle_t *handle, uint32_t voltage_ch0, uint32_t voltage_ch1)
{
    CHECK_HANDLE(handle);

    // Clamp voltages
    uint32_t max_mv = (uint32_t)handle->output_range;
    if (voltage_ch0 > max_mv)
        voltage_ch0 = max_mv;
    if (voltage_ch1 > max_mv)
        voltage_ch1 = max_mv;

    // Convert voltages to 15-bit words (0-32767)
    int data_addr = 0x02; // register start for 0 and 1 channel (16 bits each)

    uint16_t word0 = 32767 * voltage_ch0 / max_mv;
    uint16_t word1 = 32767 * voltage_ch1 / max_mv;

    uint8_t data[5] = {
        data_addr,
        (uint8_t)(word0 & 0xFF) /*low0 */, (uint8_t)(word0 >> 8) /*high0 */,
        (uint8_t)(word1 & 0xFF) /* low1 */, (uint8_t)(word1 >> 8) /* high1 */
    };

    ESP_LOGI(TAG, "Set output voltage to %d mV on channel 0 and %d mV on channel 1", voltage_ch0, voltage_ch1);
    ESP_LOGI(TAG, "Data to write: %02x %02x %02x %02x %02x", data[0], data[1], data[2], data[3], data[4]);

    return write_data_i2c(handle, data, sizeof(data));
}

esp_err_t gp8413_store_settings(gp8413_handle_t *handle)
{
    CHECK_HANDLE(handle);
    ESP_LOGE(TAG, "gp8413_store_settings() not implemented yet");
    // TODO: Write store command to device
    // Example: i2c_master_transmit(handle->dev_handle, ...);

    return ESP_OK;
}
