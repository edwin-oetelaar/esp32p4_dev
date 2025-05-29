#pragma once

/*
 * GP8413 Device Driver Testing Header
 *
 * Project: SDC2025
 * Author: Edwin van den Oetelaar (e.vandenoetelaar@fonyts.nl / edwin@oetelaar.com)
 * License: MIT
 *
 * Provides testing functions for the GP8413 DAC device.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "gp8413_sdc.h"

/**
 * @brief Run a voltage ramp test on both DAC channels.
 *
 * Channel 0 ramps up from 0V to 10V while channel 1 ramps down from 10V to 0V.
 *
 * @param dac Pointer to initialized GP8413 handle.
 * @return esp_err_t
 */
esp_err_t gp8413_sdc_ramp_test(gp8413_handle_t *dac);

/**
 * @brief Test setting zero voltage on both channels.
 *
 * @param dac Pointer to initialized GP8413 handle.
 * @return esp_err_t
 */
esp_err_t gp8413_sdc_zero_test(gp8413_handle_t *dac);

/**
 * @brief Set a voltage on channel 0 of a newly initialized DAC.
 *
 * @param bus_handle Pointer to I2C bus handle.
 * @param voltage Voltage to set in millivolts.
 * @return esp_err_t
 */
esp_err_t gp8413_sdc_set_output_voltage_ch0(i2c_master_bus_handle_t *bus_handle, uint32_t voltage);

/**
 * @brief Set a voltage on channel 1 of a newly initialized DAC.
 *
 * @param bus_handle Pointer to I2C bus handle.
 * @param voltage Voltage to set in millivolts.
 * @return esp_err_t
 */
esp_err_t gp8413_sdc_set_output_voltage_ch1(i2c_master_bus_handle_t *bus_handle, uint32_t voltage);

#ifdef __cplusplus
}
#endif
