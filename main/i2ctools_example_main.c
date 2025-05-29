/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include <esp_timer.h>
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_fat.h"
#include "cmd_system.h"
#include "cmd_i2ctools.h"
#include "driver/i2c_master.h"
#include "gp8413_sdc.h"

static const char *TAG = "i2c-tools";

static gpio_num_t i2c_gpio_sda = CONFIG_EXAMPLE_I2C_MASTER_SDA; /* 8 */
static gpio_num_t i2c_gpio_scl = CONFIG_EXAMPLE_I2C_MASTER_SCL; /* 7 */

static i2c_port_t i2c_port = I2C_NUM_0;

#if CONFIG_EXAMPLE_STORE_HISTORY

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

static void initialize_filesystem(void)
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 4,
        .format_if_mount_failed = true};
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
}
#endif // CONFIG_EXAMPLE_STORE_HISTORY

void app_main(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();

#if CONFIG_EXAMPLE_STORE_HISTORY
    initialize_filesystem();
    repl_config.history_save_path = HISTORY_PATH;
#endif
    repl_config.prompt = "i2c-tools>";

    // install console REPL environment
#if CONFIG_ESP_CONSOLE_UART
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
#elif CONFIG_ESP_CONSOLE_USB_CDC
    esp_console_dev_usb_cdc_config_t cdc_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&cdc_config, &repl_config, &repl));
#elif CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t usbjtag_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&usbjtag_config, &repl_config, &repl));
#endif

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = i2c_port,
        .scl_io_num = i2c_gpio_scl,
        .sda_io_num = i2c_gpio_sda,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    // i2c-tools> i2cconfig --port=0 --sda=8 --scl=7
    // i2c-tools> i2cdetect
    //      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
    // 00: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
    // 10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
    // 20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
    // 30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
    // 40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
    // 50: -- -- -- -- -- -- -- -- -- 59 -- -- -- -- -- --
    // 60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
    // 70: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
    ESP_LOGW(TAG, "I2C bus configuration: port=%d, SDA=%d, SCL=%d", i2c_port, i2c_gpio_sda, i2c_gpio_scl);

    ESP_LOGW(TAG, "I2C bus glitch ignore count: %d", i2c_bus_config.glitch_ignore_cnt);
    ESP_LOGW(TAG, "I2C bus internal pull-up enabled: %s", i2c_bus_config.flags.enable_internal_pullup ? "true" : "false");
    ESP_LOGW(TAG, "I2C bus clock source: %d", i2c_bus_config.clk_source);
    ESP_LOGW(TAG, "I2C bus SCL IO number: %d", i2c_bus_config.scl_io_num);
    ESP_LOGW(TAG, "I2C bus SDA IO number: %d", i2c_bus_config.sda_io_num);
    ESP_LOGW(TAG, "I2C bus port: %d", i2c_bus_config.i2c_port);
    ESP_LOGW(TAG, "I2C bus handle: %p", tool_bus_handle);

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &tool_bus_handle));

    register_i2ctools();

    printf("\n ==============================================================\n");
    printf(" |             Steps to Use i2c-tools                         |\n");
    printf(" |                                                            |\n");
    printf(" |  1. Try 'help', check all supported commands               |\n");
    printf(" |  2. Try 'i2cconfig' to configure your I2C bus              |\n");
    printf(" |  3. Try 'i2cdetect' to scan devices on the bus             |\n");
    printf(" |  4. Try 'i2cget' to get the content of specific register   |\n");
    printf(" |  5. Try 'i2cset' to set the value of specific register     |\n");
    printf(" |  6. Try 'i2cdump' to dump all the register (Experiment)    |\n");
    printf(" |  7. Try 'dac_set_output' to set DAC voltages               |\n");
    printf(" |                                                            |\n");
    printf(" ==============================================================\n\n");

    #if CONFIG_EXAMPLE_GP8413_SDC
    
    gp8413_config_t config = {
        .bus_handle = tool_bus_handle,
        .device_addr = GP8413_I2C_ADDRESS,
        .output_range = GP8413_OUTPUT_RANGE_10V,
        .channel0 = {
            .voltage = 0,
            .enable = true
        },
        .channel1 = {
            .voltage = 0,
            .enable = true
        }
    };

    gp8413_handle_t *dac = gp8413_init(&config);
    if (dac == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize DAC");
        return;
    }

    ESP_LOGI(TAG, "DAC initialized successfully");
    ESP_LOGI(TAG, "Setting output voltage to 0V on channel 0");
    esp_err_t ret = gp8413_set_output_voltage(dac, 0, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set output voltage: %s", esp_err_to_name(ret));
        gp8413_deinit(&dac);
        return;
    }

    ret = gp8413_set_output_voltage(dac, 0, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set output voltage: %s", esp_err_to_name(ret));
        gp8413_deinit(&dac);
        return;
    }
    ESP_LOGI(TAG, "Output voltage set successfully");
    ESP_LOGI(TAG, "Setting output voltage to 0V on channel 1");
    ESP_LOGE(TAG, "OK initialize DAC");

    gp8413_deinit(&dac);
#endif
    // Register system commands
    printf("timer : %lld\r\n",esp_timer_get_time());
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
