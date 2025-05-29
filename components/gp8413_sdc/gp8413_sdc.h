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
#include "esp_err.h"

// Default I2C address for the GP8413 device
#define GP8413_I2C_ADDRESS (0x59) // Default I2C address for GP8413

// GP8413 specific error codes
#define ESP_ERR_GP8413_NOT_INITIALIZED (ESP_ERR_INVALID_STATE) // Device not initialized
#define ESP_ERR_GP8413_INVALID_CHANNEL (ESP_ERR_INVALID_ARG)   // Invalid channel specified
#define ESP_ERR_GP8413_INVALID_VOLTAGE (ESP_ERR_INVALID_ARG)   // Voltage outside valid range
#define ESP_ERR_GP8413_COMMUNICATION (ESP_FAIL)                // Communication error with device

      typedef enum
    {
        GP8413_OUTPUT_RANGE_5V = 5000,   // in millivolts
        GP8413_OUTPUT_RANGE_10V = 10000, // in millivolts
    } gp8413_output_range_t;

    // Handle for the GP8413 device

    typedef struct
    {
        i2c_master_bus_handle_t bus_handle;
        uint8_t device_addr; // I2C device address (0x59)
        gp8413_output_range_t output_range;
        uint32_t current_voltage_ch0; // Current voltage for channel 0 in millivolts
        uint32_t current_voltage_ch1; // Current voltage for channel 1 in millivolts
        bool initialized;             // Flag to track if the device is properly initialized
    } gp8413_handle_t;

    // Configuration struct for GP8413 initialization
    typedef struct
    {
        i2c_master_bus_handle_t bus_handle;
        uint8_t device_addr;                // I2C device address (default: GP8413_I2C_ADDRESS)
        gp8413_output_range_t output_range; // Output voltage range (5V or 10V)
        struct
        {
            uint32_t voltage; // Initial voltage for channel 0 in millivolts
            bool enable;      // Whether to enable channel 0 during initialization
        } channel0;
        struct
        {
            uint32_t voltage; // Initial voltage for channel 1 in millivolts
            bool enable;      // Whether to enable channel 1 during initialization
        } channel1;
    } gp8413_config_t;

    // public API functions

    /**
     * @brief Initialize the GP8413 device and return a handle.
     *
     * @param config Pointer to initialization configuration.
     * @return Pointer to gp8413_handle_t on success, NULL on failure.
     */
    gp8413_handle_t *gp8413_init(const gp8413_config_t *config);

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

    // /**
    //  * @brief Get the current output voltage for a specific channel.
    //  *
    //  * @param handle Pointer to the GP8413 handle.
    //  * @param channel Channel number (0 or 1).
    //  * @param voltage Pointer to store the current voltage value.
    //  * @return esp_err_t
    //  */
    // // esp_err_t gp8413_get_output_voltage(gp8413_handle_t *handle, uint32_t channel, uint32_t *voltage);

    // /**
    //  * @brief Reset the GP8413 device to default settings.
    //  *
    //  * @param handle Pointer to the GP8413 handle.
    //  * @return esp_err_t
    //  */
    // esp_err_t gp8413_reset(gp8413_handle_t *handle);

    // /**
    //  * @brief Enable or disable a specific channel.
    //  *
    //  * @param handle Pointer to the GP8413 handle.
    //  * @param channel Channel number (0 or 1).
    //  * @param enable True to enable, false to disable.
    //  * @return esp_err_t
    //  */
    // esp_err_t gp8413_set_channel_enable(gp8413_handle_t *handle, uint32_t channel, bool enable);

#ifdef __cplusplus
}
#endif