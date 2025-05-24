#pragma once
/*
 * GP8413 Device Driver Header
 *
 * Project: SDC2025
 * Author: Edwin van den Oetelaar (e.vandenoetelaar@fonyts.nl / edwin@oetelaar.com)
 * License: MIT
 *
 * Provides interface definitions for the GP8413 DAC device.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "driver/i2c_master.h"

#define GP8413_REG_CONFIG_CURRENT (0x02) // Default register in GP8413
#define GP8413_I2C_ADDRESS (0x59)        // Default I2C address

    typedef enum
    {
        GP8413_OUTPUT_RANGE_5V = 5000,   // in millivolts
        GP8413_OUTPUT_RANGE_10V = 10000, // in millivolts
    } gp8413_output_range_t;

    typedef struct
    {
        i2c_master_bus_handle_t bus_handle;
        uint8_t device_addr; // I2C device address (0x59)
        gp8413_output_range_t output_range;
    } gp8413_handle_t;

    // private functions
    // [[maybe_unused]] static esp_err_t write_data_i2c(gp8413_handle_t *handle, uint8_t *data, size_t size);

    // public API functions

    /**
     * @brief Initialize the GP8413 device and return a handle.
     *
     * @param dev_handle   I2C device handle.
     * @param device_addr  I2C device address.
     * @param output_range Initial output voltage range.
     * @param voltage_ch0  Initial output voltage for channel 0 in millivolts.
     * @param voltage_ch1  Initial output voltage for channel 1 in millivolts.
     * @return Pointer to gp8413_handle_t on success, NULL on failure.
     */
    gp8413_handle_t *gp8413_init(i2c_master_bus_handle_t bus_handle, uint8_t device_addr, gp8413_output_range_t output_range, uint32_t voltage_ch0, uint32_t voltage_ch1);

    /**
     * @brief Deinitialize the GP8413 device and free its handle.
     *
     * @param handle Double pointer to the GP8413 handle to be deinitialized and set to NULL.
     */
    void gp8413_deinit(gp8413_handle_t **handle);

    /**
     * @brief Set the output voltage range for the GP8413 DAC.
     *
     * @param handle Pointer to the GP8413 handle.
     * @param range  Output voltage range.
     * @return esp_err_t
     */
    esp_err_t gp8413_set_output_range(gp8413_handle_t *handle, gp8413_output_range_t range);

    /**
     * @brief Set the output voltage for a specific channel.
     *
     * @param handle  Pointer to the GP8413 handle.
     * @param voltage Output voltage in millivolts.
     * @param channel Channel number (0 or 1).
     * @return esp_err_t
     */
    esp_err_t gp8413_set_output_voltage(gp8413_handle_t *handle, uint32_t voltage, uint32_t channel);

    /**
     * @brief Set the output voltage for both channels.
     *
     * @param handle      Pointer to the GP8413 handle.
     * @param voltage_ch0 Output voltage for channel 0 in millivolts.
     * @param voltage_ch1 Output voltage for channel 1 in millivolts.
     * @return esp_err_t
     */
    esp_err_t gp8413_set_output_voltage_dual(gp8413_handle_t *handle, uint32_t voltage_ch0, uint32_t voltage_ch1);

    /**
     * @brief Store the current settings to the GP8413 EEPROM.
     *
     * @param handle Pointer to the GP8413 handle.
     * @return esp_err_t
     *
     * @note Be careful: storing non-zero values may cause the device to start up with outputs enabled.
     */
    esp_err_t gp8413_store_settings(gp8413_handle_t *handle); // not implemented yet

#ifdef __cplusplus
}
#endif