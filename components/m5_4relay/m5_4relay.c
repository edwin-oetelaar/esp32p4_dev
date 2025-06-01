/**
 * @file m5_4relay.c
 * @brief Template implementation for the M5 4-Relay board via I2C on ESP32P4.
 *
 * This file provides functions to initialize, control, and deinitialize the M5 4-Relay board
 * using the ESP-IDF I2C master driver. It includes helper functions for I2C communication,
 * relay and LED control, and mode management.
 *
 * Features:
 * - Initialization and deinitialization of the relay board context and I2C device handle.
 * - Setting and reading the state of individual relays and LEDs.
 * - Setting all relays or LEDs simultaneously.
 * - Reading and writing device registers via I2C.
 * - Managing the board's operation mode (automatic/manual).
 *
 * Usage:
 * 1. Allocate and configure an m54_ctx_t structure.
 * 2. Call m54_init() to initialize the board.
 * 3. Use relay and LED control functions as needed.
 * 4. Call m54_deinit() to clean up.
 *
 * Author: Edwin vd Oetelaar
 * Date: May 2025
 */

#include "m5_4relay.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <string.h> // String manipulation functions
// ESP-IDF includes for error reporting, logging, and system functions
#include "esp_err.h"           // ESP-IDF error codes
#include "esp_log.h"           // Logging functionality
#include "driver/i2c_master.h" // I2C master configuration and communication
// Logging tag for ESP-IDF
static const char *TAG = "M5-4Relay";
////////////////////////////////////////////////////////////////////////////////
// Intern: I2C-write helper
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief  Schrijf één register (1 byte) met één databyte via I2C.
 * @param  handle       Pointer naar het M5-4Relay device-descriptor
 * @param  reg_addr     Registeradres (bijv. M54R_REG_RELAY)
 * @param  value        De waarde die naar dat register geschreven moet worden
 * @return ESP_OK       als de write slaagt, anders foutcode.
 */
static esp_err_t m54_i2c_write_byte(m54_ctx_t *handle, uint8_t reg_addr, uint8_t value)
{
    if (!handle)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Data-buffer: eerst registeradres, dan de waarde
    uint8_t data[2] = {reg_addr, value};

    return i2c_master_transmit(
        handle->dev_handle,  // al aangemaakte I2C-device-handle
        data,                // pointer naar [reg_addr, value]
        sizeof(data),        // lengte = 2 bytes
        pdMS_TO_TICKS(100)); // timeout = 100 ms
}

static esp_err_t m54_i2c_read_registers(m54_ctx_t *handle, uint8_t *modeval, uint8_t *relvalue)
{
    if (!handle)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // send-buffer: registeradres, dan de waarde
    uint8_t data_addr[] = {M54R_REG_MODE}; // We lezen het mode-register
    uint8_t result[2] = {0, 0};            // Buffer voor de ontvangen data
    // result[0] is mode
    // result[1] is LED + RELAY status
    esp_err_t ret = i2c_master_transmit_receive(handle->dev_handle, data_addr, sizeof(data_addr), result, sizeof(result), pdMS_TO_TICKS(100));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read register 0x%02X: %s", data_addr[0], esp_err_to_name(ret));
        return ret; // Return the error code
    }
    // We verwachten 2 bytes: mode en relay+led status
    ESP_LOGI(TAG, "Read mode: 0x%02X, relay+led status: 0x%02X", result[0], result[1]);
    // Zet de mode en relay+led status in de output pointer
    if (modeval && relvalue)
    {
        *modeval = result[0];  // Mode is de eerste byte
        *relvalue = result[1]; // Relay+LED status is de tweede byte
    }
    else if (modeval) // Alleen mode gevraagd
    {
        *modeval = result[0];
    }
    else if (relvalue) // Alleen relay+led status gevraagd
    {
        *relvalue = result[1];
    }
    else
    {
        ESP_LOGE(TAG, "Output pointer is NULL");
        return ESP_ERR_INVALID_ARG; // Return error if output pointer is NULL
    }
    return ESP_OK; // Return success
}
////////////////////////////////////////////////////////////////////////////////
// m54r_init & m54r_deinit
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief  Initialiseer het M5-4Relay board: maak de I2C-devicehandle aan en bewaar instellingen.
 * @param  dev_handle_p  Pointer naar m54_ctx_t struct (caller moet dit alloceren).
 * @param  bus_handle    Het I2C-bus-handle (i2c_master_bus_handle_t) dat al eerder is geconfigureerd.
 * @param  device_addr   7-bit I2C-adres van het relay-board (meestal 0x26).
 * @param  scl_speed_hz  De I2C klokfrequentie (bijv. 100000 of 400000).
 * @return ESP_OK als succes, anders ESP_ERR_xxx.
 */
esp_err_t m54_init(m54_ctx_t *dev)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG; // Controleer of de handle geldig is
    }

    // Check if the device is already initialized
    if (dev->initialized)
    {
        ESP_LOGI(TAG, "M5-4Relay device already initialized");
        return ESP_OK; // Device is already initialized
    }
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
        return ESP_FAIL; // Return error if device addition fails
    }

    // Set the initialized flag to true
    dev->initialized = true;
    ESP_LOGI(TAG, "M5-4Relay device initialized at address 0x%02X", dev->device_address);
    // 3) Zet de initiële relay- en LED-status op 0 (uit).
    dev->relay_state = 0x00; // Alle relais uit
    dev->led_state = 0x00;   // Alle LEDs uit
    dev->mode = 0x01;        // Standaard modus is sync (0x01)

    ESP_LOGI(TAG, "M5-4Relay device initialized successfully");
    uint8_t initial_mode = 0;
    uint8_t initial_led_relay = 0;
    ret = m54_i2c_read_registers(dev, &initial_mode, &initial_led_relay); // Lees de initiële status van het device
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read initial status: %s", esp_err_to_name(ret));
        return ret; // Return the error code if reading fails
    }
    // Zet de mode en relay+led status in de context
    dev->mode = (initial_mode ? 0x01 : 0x00);      // Mode: 0x01 = sync, 0x00 = async
    dev->relay_state = (initial_led_relay & 0x0F); // Bit0..Bit3 zijn de relais
    dev->led_state = (initial_led_relay & 0xF0);   // Bit4..Bit7 zijn de LEDs
    ESP_LOGI(TAG, "Initial mode: 0x%02X, relay state: 0x%02X, led state: 0x%02X", dev->mode, dev->relay_state, dev->led_state);
    return ESP_OK;
}

/**
 * @brief  De- initialiseert het M5-4Relay board: verwijder de I2C-device en maak struct leeg.
 * @param  dev  Pointer naar eerder geïnitialiseerd m54_ctx_t
 */
void m54_deinit(m54_ctx_t *dev)
{
    assert(dev != NULL);

    // Free the device handle if needed (not shown here)
    if (i2c_master_bus_rm_device(dev->dev_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to remove M5-4Relay I2C device");
    }
    // Reset the device context
    dev->dev_handle = NULL;  // Set the device handle to NULL
    dev->initialized = 0;    // Mark as uninitialized
    dev->relay_state = 0x00; // Reset relay state
    dev->led_state = 0x00;   // Reset LED state
    dev->mode = 0;           // Reset mode
                             // Note: We do not free the m54_ctx_t struct itself, as it is expected to be allocated by the caller.
                             // Optionally, you can log the deinitialization
    ESP_LOGI(TAG, "M5-4Relay device deinitialized");
}

////////////////////////////////////////////////////////////////////////////////
// Basisrelay-commando's
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief  Zet één relay aan of uit.
 * @param  handle   Pointer naar geïnitialiseerd m54_ctx_t.
 * @param  number   Relay-nummer [0..3] (er zijn 4 relais op het board).
 * @param  state    true = aan ( gesloten ), false = uit ( open ).
 */
esp_err_t m54_relay_set(m54_ctx_t *handle, uint8_t number, uint8_t state)
{
    if (!handle || number > 3)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Relay-register (één byte) bestaat uit bit-mask: bit0 = relay0, bit1 = relay1, etc.
    // Lees eerst de huidige waarde (optioneel), of houd lokaal bij wat je hebt gezet.
    // Hier doen we simpel: we lezen niet; we maken een klein bitmask en schrijven het hele register.
    uint32_t s = handle->relay_state; // Huidige relay-status
    if (state == 0)
    {
        s &= ~(0x01 << number);
    }
    else
    {
        s |= (0x01 << number);
    }
    handle->relay_state = s;

    // Schrijf naar register M54R_REG_RELAY
    return m54_i2c_write_byte(handle, M54R_REG_RELAY, handle->led_state | handle->relay_state); // LED's in bit4..bit7, relais in bit0..bit3
}

/**
 * @brief  Schakel alle relais tegelijk aan/uit.
 * @param  handle   Pointer naar geïnitialiseerd m54_ctx_t.
 * @param  state    true = alle aan, false = alle uit.
 */
esp_err_t m54_relay_set_all(m54_ctx_t *handle, uint8_t state)
{
    if (!handle)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t mask = state ? 0x0F : 0x00; // bit0..bit3 high = alle 4 relais aan
    // TODO: Als je de huidige status wilt behouden, lees dan eerst het register:
    // uint8_t current_state = 0;
    // m54_i2c_read_byte(handle, M54R_REG_RELAY, &current_state);
    handle->relay_state = mask; // Bewaar de nieuwe relay-status in de context
    // Schrijf naar register M54R_REG_RELAY
    // LED's in bit4..bit7, relais in bit0..bit3
    return m54_i2c_write_byte(handle, M54R_REG_RELAY, handle->led_state | mask); // LED's in bit4..bit7, relais in bit0..bit3
    //     return m54_i2c_write_byte(handle, M54R_REG_RELAY, mask);
}

////////////////////////////////////////////////////////////////////////////////
// Basis-LED-commando's (per board bevatten sommige LED's dezelfde registers)
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief  Zet één LED aan of uit.
 * @param  handle   Pointer naar geïnitialiseerd m54_ctx_t.
 * @param  number   LED-nummer [0..3] (er zijn 4 LEDs op het board).
 * @param  state    true = aan, false = uit.
 */
esp_err_t m54_led_set(m54_ctx_t *handle, uint8_t number, uint8_t state)
{
    if (!handle || number > 3)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t s = handle->led_state; // Huidige relay-status
    if (state == 0)
    {
        s &= ~(0x10 << number);
    }
    else
    {
        s |= (0x10 << number);
    }
    handle->led_state = s;
    if (handle->mode == 0x01) // Als we in sync mode zijn, dan gaan de LEDs niet reageren
    {
        ESP_LOGI(TAG, "LED %d set to %s (no effect in Sync mode)", number, state ? "ON" : "OFF");
    }
    // Schrijf naar register M54R_REG_RELAY
    return m54_i2c_write_byte(handle, M54R_REG_RELAY, handle->led_state | handle->relay_state); // LED's in bit4..bit7, relais in bit0..bit3
}

esp_err_t m54_led_get(m54_ctx_t *handle, uint8_t number, uint8_t *state)
{
    if (!handle || number > 3)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t mystate = handle->led_state;  // Huidige led-status, we moeten deze uit het device halen: TODO: m54_i2c_read_byte(handle, M54R_REG_RELAY, &mystate);
    if ((mystate & (0x10 << number)) == 0) // check
    {
        *state = 0; // LED is uit
    }
    else
    {
        *state = 1; // LED is aan
    }

    return ESP_OK;
}

esp_err_t m54_relay_get(m54_ctx_t *handle, uint8_t number, uint8_t *state)
{
    if (!handle || number > 3)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t mystate = handle->relay_state; // Huidige relay-state, we moeten deze uit het device halen: TODO
    if ((mystate & (0x1 << number)) == 0)   // check
    {
        *state = 0; // relay is uit
    }
    else
    {
        *state = 1; // relay is aan
    }
    return ESP_OK;
}
/**
 * @brief  Schakel alle LEDs tegelijk aan/uit.
 * @param  handle   Pointer naar geïnitialiseerd m54_ctx_t.
 * @param  state    true = alle aan, false = alle uit.
 */
esp_err_t m54_led_set_all(m54_ctx_t *handle, uint8_t state)
{
    if (!handle)
    {
        return ESP_ERR_INVALID_ARG;
    }

    handle->led_state = state ? 0xF0 : 0x00;                                                    // bit4..bit7 high = alle 4 LEDs aan
    return m54_i2c_write_byte(handle, M54R_REG_RELAY, handle->led_state | handle->relay_state); // LED's in bit4..bit7, relais in bit0..bit3
}

////////////////////////////////////////////////////////////////////////////////
// Besturingsmodus
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief  Zet de led status (bijvoorbeeld “Auto” of “Manual”).
 * @param  handle   Pointer naar geïnitialiseerd m54_ctx_t.
 * @param  mode     true = modus Automatisch, false = modus Handmatig.
 */
esp_err_t m54_mode_set(m54_ctx_t *handle, uint8_t mode)
{
    if (!handle)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Zet de modus van het board: bit0 in register M54R_REG_MODE.
    // Dit bepaalt of de leds automatisch aan/uit gaan bij het inschakelen van de relais.
    // In deze template zetten we bit0=mode, zonder eerst te lezen (alle andere bits gaan dan naar 0).

    uint8_t reg_value = (mode ? 0x01 : 0x00);
    return m54_i2c_write_byte(handle, M54R_REG_MODE, reg_value);
}

// end