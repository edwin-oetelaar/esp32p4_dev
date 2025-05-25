#include "gp8413_sdc.h"
#include "esp_log.h"
#include "esp_console.h"

esp_err_t gp8413_sdc_testing1(gp8413_handle_t *dac)
{
    uint32_t voltages1[] = {0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};
    uint32_t voltages2[] = {10000, 9000, 8000, 7000, 6000, 5000, 4000, 3000, 2000, 1000, 0};

    for (int i = 0; i < sizeof(voltages1) / sizeof(voltages1[0]); i++)
    {
        ESP_LOGI(TAG, "Setting output voltage to %d mV on channel 0", voltages1[i]);
        ESP_LOGI(TAG, "Setting output voltage to %d mV on channel 1", voltages2[i]);

        ret = gp8413_set_output_voltage(dac, voltages1[i], 0);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set output voltage: %s", esp_err_to_name(ret));
            break;
        }
        ret = gp8413_set_output_voltage(dac, voltages2[i], 1);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set output voltage: %s", esp_err_to_name(ret));
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "Testing completed successfully");
    return ESP_OK;
}

esp_err_t gp8413_sdc_testing1(gp8413_handle_t *dac)
{

    esp_err_t ret = gp8413_set_output_voltage(dac, 0, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set output voltage: %s", esp_err_to_name(ret));
        gp8413_deinit(&dac);
        return ESP_ERR_INVALID_ARG;
    }

    ret = gp8413_set_output_voltage(dac, 0, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set output voltage: %s", esp_err_to_name(ret));
        gp8413_deinit(&dac);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Output voltage set successfully");
    ESP_LOGI(TAG, "Setting output voltage to 0V on channel 1");
    ESP_LOGE(TAG, "OK initialize DAC");
    return ESP_OK;
}

esp_err_t gp8413_dgp8413_sdc_set_output_voltage1(tool_bus_handle_t *tool_bus_handle)
{
    gp8413_handle_t *dac = gp8413_init(tool_bus_handle, GP8413_I2C_ADDRESS, GP8413_OUTPUT_RANGE_10V, 0, 0);
    if (dac == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize DAC");
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "DAC initialized successfully");
    ret = gp8413_set_output_voltage(dac, voltage, 0);
    gp8413_deinit(&dac);
}

esp_err_t gp8413_sdc_set_output_voltage2(tool_bus_handle_t *tool_bus_handle)
{
    gp8413_handle_t *dac = gp8413_init(tool_bus_handle, GP8413_I2C_ADDRESS, GP8413_OUTPUT_RANGE_10V, 0, 0);
    if (dac == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize DAC");
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "DAC initialized successfully");
    ret = gp8413_set_output_voltage(dac, voltages, 1);
    gp8413_deinit(&dac);
}
