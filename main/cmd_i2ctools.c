/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* cmd_i2ctools.c

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "argtable3/argtable3.h"
#include "driver/i2c_master.h"
#include "esp_console.h"
#include "esp_log.h"
#include "gp8413_sdc.h"
#include "ssd1306.h"

static const char *TAG = "cmd_i2ctools";

#define I2C_TOOL_TIMEOUT_VALUE_MS (50)
static uint32_t i2c_frequency = 100 * 1000;
i2c_master_bus_handle_t tool_bus_handle;

static esp_err_t i2c_get_port(int port, i2c_port_t *i2c_port)
{
    if (port >= I2C_NUM_MAX)
    {
        ESP_LOGE(TAG, "Wrong port number: %d", port);
        return ESP_FAIL;
    }
    *i2c_port = port;
    return ESP_OK;
}

static struct
{
    struct arg_int *port;
    struct arg_int *freq;
    struct arg_int *sda;
    struct arg_int *scl;
    struct arg_end *end;
} i2cconfig_args;

static int do_i2cconfig_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&i2cconfig_args);
    i2c_port_t i2c_port = I2C_NUM_0;
    int i2c_gpio_sda = 0;
    int i2c_gpio_scl = 0;
    if (nerrors != 0)
    {
        arg_print_errors(stderr, i2cconfig_args.end, argv[0]);
        return 0;
    }

    /* Check "--port" option */
    if (i2cconfig_args.port->count)
    {
        if (i2c_get_port(i2cconfig_args.port->ival[0], &i2c_port) != ESP_OK)
        {
            return 1;
        }
    }
    /* Check "--freq" option */
    if (i2cconfig_args.freq->count)
    {
        i2c_frequency = i2cconfig_args.freq->ival[0];
    }
    /* Check "--sda" option */
    i2c_gpio_sda = i2cconfig_args.sda->ival[0];
    /* Check "--scl" option */
    i2c_gpio_scl = i2cconfig_args.scl->ival[0];

    // re-init the bus
    if (i2c_del_master_bus(tool_bus_handle) != ESP_OK)
    {
        return 1;
    }

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = i2c_port,
        .scl_io_num = i2c_gpio_scl,
        .sda_io_num = i2c_gpio_sda,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    if (i2c_new_master_bus(&i2c_bus_config, &tool_bus_handle) != ESP_OK)
    {
        return 1;
    }

    return 0;
}

static void register_i2cconfig(void)
{
    i2cconfig_args.port = arg_int0(NULL, "port", "<0|1>", "Set the I2C bus port number");
    i2cconfig_args.freq = arg_int0(NULL, "freq", "<Hz>", "Set the frequency(Hz) of I2C bus");
    i2cconfig_args.sda = arg_int1(NULL, "sda", "<gpio>", "Set the gpio for I2C SDA");
    i2cconfig_args.scl = arg_int1(NULL, "scl", "<gpio>", "Set the gpio for I2C SCL");
    i2cconfig_args.end = arg_end(2);
    const esp_console_cmd_t i2cconfig_cmd = {
        .command = "i2cconfig",
        .help = "Config I2C bus",
        .hint = NULL,
        .func = &do_i2cconfig_cmd,
        .argtable = &i2cconfig_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cconfig_cmd));
}

static int do_i2cdetect_cmd(int argc, char **argv)
{
    uint8_t address;
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
    for (int i = 0; i < 128; i += 16)
    {
        printf("%02x: ", i);
        for (int j = 0; j < 16; j++)
        {
            fflush(stdout);
            address = i + j;
            esp_err_t ret = i2c_master_probe(tool_bus_handle, address, I2C_TOOL_TIMEOUT_VALUE_MS);
            if (ret == ESP_OK)
            {
                printf("%02x ", address);
            }
            else if (ret == ESP_ERR_TIMEOUT)
            {
                printf("UU ");
            }
            else
            {
                printf("-- ");
            }
        }
        printf("\r\n");
    }

    return 0;
}

static void register_i2cdetect(void)
{
    const esp_console_cmd_t i2cdetect_cmd = {
        .command = "i2cdetect",
        .help = "Scan I2C bus for devices",
        .hint = NULL,
        .func = &do_i2cdetect_cmd,
        .argtable = NULL};
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cdetect_cmd));
}

static struct
{
    struct arg_int *chip_address;
    struct arg_int *register_address;
    struct arg_int *data_length;
    struct arg_end *end;
} i2cget_args;

static int do_i2cget_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&i2cget_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, i2cget_args.end, argv[0]);
        return 0;
    }

    /* Check chip address: "-c" option */
    int chip_addr = i2cget_args.chip_address->ival[0];
    /* Check register address: "-r" option */
    int data_addr = -1;
    if (i2cget_args.register_address->count)
    {
        data_addr = i2cget_args.register_address->ival[0];
    }
    /* Check data length: "-l" option */
    int len = 1;
    if (i2cget_args.data_length->count)
    {
        len = i2cget_args.data_length->ival[0];
    }
    uint8_t *data = malloc(len);

    i2c_device_config_t i2c_dev_conf = {
        .scl_speed_hz = i2c_frequency,
        .device_address = chip_addr,
    };

    i2c_master_dev_handle_t dev_handle;
    if (i2c_master_bus_add_device(tool_bus_handle, &i2c_dev_conf, &dev_handle) != ESP_OK)
    {
        return 1;
    }

    esp_err_t ret = i2c_master_transmit_receive(dev_handle, (uint8_t *)&data_addr, 1, data, len, I2C_TOOL_TIMEOUT_VALUE_MS);
    if (ret == ESP_OK)
    {
        for (int i = 0; i < len; i++)
        {
            printf("0x%02x ", data[i]);
            if ((i + 1) % 16 == 0)
            {
                printf("\r\n");
            }
        }
        if (len % 16)
        {
            printf("\r\n");
        }
    }
    else if (ret == ESP_ERR_TIMEOUT)
    {
        ESP_LOGW(TAG, "Bus is busy");
    }
    else
    {
        ESP_LOGW(TAG, "Read failed");
    }
    free(data);
    if (i2c_master_bus_rm_device(dev_handle) != ESP_OK)
    {
        return 1;
    }
    return 0;
}

static void register_i2cget(void)
{
    i2cget_args.chip_address = arg_int1("c", "chip", "<chip_addr>", "Specify the address of the chip on that bus");
    i2cget_args.register_address = arg_int0("r", "register", "<register_addr>", "Specify the address on that chip to read from");
    i2cget_args.data_length = arg_int0("l", "length", "<length>", "Specify the length to read from that data address");
    i2cget_args.end = arg_end(1);
    const esp_console_cmd_t i2cget_cmd = {
        .command = "i2cget",
        .help = "Read registers visible through the I2C bus",
        .hint = NULL,
        .func = &do_i2cget_cmd,
        .argtable = &i2cget_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cget_cmd));
}

static struct
{
    struct arg_int *chip_address;
    struct arg_int *register_address;
    struct arg_int *data;
    struct arg_end *end;
} i2cset_args;

static int do_i2cset_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&i2cset_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, i2cset_args.end, argv[0]);
        return 0;
    }

    /* Check chip address: "-c" option */
    int chip_addr = i2cset_args.chip_address->ival[0];
    /* Check register address: "-r" option */
    int data_addr = 0;
    if (i2cset_args.register_address->count)
    {
        data_addr = i2cset_args.register_address->ival[0];
    }
    /* Check data: "-d" option */
    int len = i2cset_args.data->count;

    i2c_device_config_t i2c_dev_conf = {
        .scl_speed_hz = i2c_frequency,
        .device_address = chip_addr,
    };
    i2c_master_dev_handle_t dev_handle;
    if (i2c_master_bus_add_device(tool_bus_handle, &i2c_dev_conf, &dev_handle) != ESP_OK)
    {
        return 1;
    }

    uint8_t *data = malloc(len + 1);
    data[0] = data_addr;
    for (int i = 0; i < len; i++)
    {
        data[i + 1] = i2cset_args.data->ival[i];
    }
    esp_err_t ret = i2c_master_transmit(dev_handle, data, len + 1, I2C_TOOL_TIMEOUT_VALUE_MS);
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

    free(data);
    if (i2c_master_bus_rm_device(dev_handle) != ESP_OK)
    {
        return 1;
    }
    return 0;
}

static void register_i2cset(void)
{
    i2cset_args.chip_address = arg_int1("c", "chip", "<chip_addr>", "Specify the address of the chip on that bus");
    i2cset_args.register_address = arg_int0("r", "register", "<register_addr>", "Specify the address on that chip to read from");
    i2cset_args.data = arg_intn(NULL, NULL, "<data>", 0, 256, "Specify the data to write to that data address");
    i2cset_args.end = arg_end(2);
    const esp_console_cmd_t i2cset_cmd = {
        .command = "i2cset",
        .help = "Set registers visible through the I2C bus",
        .hint = NULL,
        .func = &do_i2cset_cmd,
        .argtable = &i2cset_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cset_cmd));
}

static struct
{
    struct arg_int *chip_address;
    struct arg_int *size;
    struct arg_end *end;
} i2cdump_args;

static int do_i2cdump_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&i2cdump_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, i2cdump_args.end, argv[0]);
        return 0;
    }

    /* Check chip address: "-c" option */
    int chip_addr = i2cdump_args.chip_address->ival[0];
    /* Check read size: "-s" option */
    int size = 1;
    if (i2cdump_args.size->count)
    {
        size = i2cdump_args.size->ival[0];
    }
    if (size != 1 && size != 2 && size != 4)
    {
        ESP_LOGE(TAG, "Wrong read size. Only support 1,2,4");
        return 1;
    }

    i2c_device_config_t i2c_dev_conf = {
        .scl_speed_hz = i2c_frequency,
        .device_address = chip_addr,
    };
    i2c_master_dev_handle_t dev_handle;
    if (i2c_master_bus_add_device(tool_bus_handle, &i2c_dev_conf, &dev_handle) != ESP_OK)
    {
        return 1;
    }

    uint8_t data_addr;
    uint8_t data[4];
    int32_t block[16];
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f"
           "    0123456789abcdef\r\n");
    for (int i = 0; i < 128; i += 16)
    {
        printf("%02x: ", i);
        for (int j = 0; j < 16; j += size)
        {
            fflush(stdout);
            data_addr = i + j;
            esp_err_t ret = i2c_master_transmit_receive(dev_handle, &data_addr, 1, data, size, I2C_TOOL_TIMEOUT_VALUE_MS);
            if (ret == ESP_OK)
            {
                for (int k = 0; k < size; k++)
                {
                    printf("%02x ", data[k]);
                    block[j + k] = data[k];
                }
            }
            else
            {
                for (int k = 0; k < size; k++)
                {
                    printf("XX ");
                    block[j + k] = -1;
                }
            }
        }
        printf("   ");
        for (int k = 0; k < 16; k++)
        {
            if (block[k] < 0)
            {
                printf("X");
            }
            if ((block[k] & 0xff) == 0x00 || (block[k] & 0xff) == 0xff)
            {
                printf(".");
            }
            else if ((block[k] & 0xff) < 32 || (block[k] & 0xff) >= 127)
            {
                printf("?");
            }
            else
            {
                printf("%c", (char)(block[k] & 0xff));
            }
        }
        printf("\r\n");
    }
    if (i2c_master_bus_rm_device(dev_handle) != ESP_OK)
    {
        return 1;
    }
    return 0;
}

static void register_i2cdump(void)
{
    i2cdump_args.chip_address = arg_int1("c", "chip", "<chip_addr>", "Specify the address of the chip on that bus");
    i2cdump_args.size = arg_int0("s", "size", "<size>", "Specify the size of each read");
    i2cdump_args.end = arg_end(1);
    const esp_console_cmd_t i2cdump_cmd = {
        .command = "i2cdump",
        .help = "Examine registers visible through the I2C bus",
        .hint = NULL,
        .func = &do_i2cdump_cmd,
        .argtable = &i2cdump_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cdump_cmd));
}

static struct
{
    struct arg_int *ch0_val;
    struct arg_int *ch1_val;
    struct arg_end *end;
} dacset_args;

static int do_dacset_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&dacset_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, dacset_args.end, argv[0]);
        return 0;
    }

    uint32_t ch0_val = 0;
    uint32_t ch1_val = 0;
    gp8413_channel_t channel_type = GP8413_CH_NONE;
    if (dacset_args.ch0_val->count == 0 && dacset_args.ch1_val->count == 0)
    {
        ESP_LOGE(TAG, "No output value specified for channel 1 or channel 2");
        return 1;
    }
    if (dacset_args.ch0_val->count == 1 && dacset_args.ch1_val->count == 1)
    {
        channel_type = GP8413_CH0_CH1;
        ch0_val = dacset_args.ch0_val->ival[0];
        ch1_val = dacset_args.ch1_val->ival[0];
    }
    else if (dacset_args.ch0_val->count == 1)
    {
        channel_type = GP8413_CH0;
        ch0_val = dacset_args.ch0_val->ival[0];
    }
    else if (dacset_args.ch1_val->count == 1)
    {
        channel_type = GP8413_CH1;
        ch1_val = dacset_args.ch1_val->ival[0];
    }

    if (ch0_val > 10000 || ch1_val > 10000)
    {
        ESP_LOGE(TAG, "Output voltage must be between 0 and 10000 mV");
        return 1;
    }

    ESP_LOGI(TAG, "Setting output voltage to %d mV on channel 1", ch0_val);
    ESP_LOGI(TAG, "Setting output voltage to %d mV on channel 2", ch1_val);

    gp8413_init_params_t init_params = {
        .bus_handle = tool_bus_handle,
        .device_addr = GP8413_I2C_ADDRESS,
        .output_range = GP8413_OUTPUT_RANGE_10V,
        .voltage_ch0 = ch0_val,
        .voltage_ch1 = ch1_val,
        .channel_type = channel_type};

    ESP_LOGI(TAG, "Initializing DAC with parameters: "
                  "Device Address: 0x%02x, "
                  "Output Range: %d mV, "
                  "Channel Type: %d, "
                  "Channel 0 Voltage: %d mV, "
                  "Channel 1 Voltage: %d mV",
             init_params.device_addr,
             init_params.output_range,
             init_params.channel_type,
             init_params.voltage_ch0,
             init_params.voltage_ch1);

    gp8413_handle_t *dac = gp8413_init(&init_params);
    if (dac == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize DAC");
        return 1;
    }
    ESP_LOGI(TAG, "DAC initialized successfully");
    esp_err_t ret = gp8413_set_output_voltage(dac, ch0_val, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set output voltage for channel 0: %s", esp_err_to_name(ret));
        gp8413_deinit(&dac);
        return 1;
    }
    ret = gp8413_set_output_voltage(dac, ch1_val, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set output voltage for channel 1: %s", esp_err_to_name(ret));
        gp8413_deinit(&dac);
        return 1;
    }
    ESP_LOGI(TAG, "Output voltage set successfully");
    ESP_LOGI(TAG, "Setting output voltage to %d mV on channel 0", ch0_val);
    ESP_LOGI(TAG, "Setting output voltage to %d mV on channel 1", ch1_val);
    gp8413_deinit(&dac);
    return 0;
}

static void register_dac_set(void)
{
    dacset_args.ch0_val = arg_int0("s", "ch0", "<ch0 speed in mv>", "Output value for channel 0 in millivolts");
    dacset_args.ch1_val = arg_int0("b", "ch1", "<ch1 brake_force in mv>", "Output value for channel 1 in millivolts");
    dacset_args.end = arg_end(2);
    const esp_console_cmd_t dacset_cmd = {
        .command = "dac_set_output",
        .help = "Set value of DAC output",
        .hint = NULL,
        .func = &do_dacset_cmd,
        .argtable = &dacset_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&dacset_cmd));
}

static struct
{
    struct arg_int *ch0_val;
    struct arg_end *end;
} ssdset_args;

static int do_ssd1306_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&ssdset_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, ssdset_args.end, argv[0]);
        return 0;
    }

    uint32_t ch0_val = ssdset_args.ch0_val->ival[0];
    ESP_LOGI(TAG, "Setting SSD1306 display text to: %d", ch0_val);
    // Here you would typically call a function to set the text on the SSD1306 display
    // For example: ssd1306_set_text(ch0_val);

    // ssd1306_handle_t ssd1306_handle = {0};
    // ssd1306_handle.tool_bus_handle = tool_bus_handle;
    // ssd1306_handle.device_address = SSD1306_I2C_ADDRESS;
    ssd1306_handle_t dev = {
        .bus_handle = tool_bus_handle,
        // Assuming tool_bus_handle is already initialized and points to the I2C bus
        .device_address = SSD1306_I2C_ADDRESS,
        .scl_speed_hz = i2c_frequency,
        .external_vcc = 0, // Set to 1 if using external VCC
        .width = 128,
        .height = 64,
        .pages = 8,         // For a 128x64 display, there are 8 pages
        .dev_handle = NULL, // This will be set when the device is initialized
        .buffer = {0},      // clear the buffer
    };

    ssd1306_init(&dev, 128, 64, 0); // Initialize the SSD1306 display
    if (dev.dev_handle == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize SSD1306 display");
        return 1;
    }
    ESP_LOGI(TAG, "SSD1306 display initialized successfully");
    // Fill the display with white color (1)

    char buf[16];
    snprintf(buf, sizeof(buf), "Value: %ld", ch0_val);
    ssd1306_printFixed16(&dev, 0, 0, 1, buf); // Draw the string at (0, 0) with white color
    ssd1306_show(&dev);                       // Show the drawn string on the display
    ESP_LOGI(TAG, "SSD1306 display updated with text: %s", buf);

    //    ssd1306_fill(&dev, 1);
    //    ssd1306_show(&dev); // Show the filled display
    //    ESP_LOGI(TAG, "SSD1306 display filled with white color");

    // vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second to see the filled display
    ssd1306_deinit(&dev); // Deinitialize the display after use
    ESP_LOGI(TAG, "SSD1306 display deinitialized successfully");
    return 0;
}

static void register_ssd1306(void)
{
    ssdset_args.ch0_val = arg_int0("s", "txt", "display integer", "some value");
    ssdset_args.end = arg_end(2);
    const esp_console_cmd_t ssdset_cmd = {
        .command = "ssd1306",
        .help = "Set text",
        .hint = NULL,
        .func = &do_ssd1306_cmd,
        .argtable = &ssdset_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&ssdset_cmd));
}

void register_i2ctools(void)
{
    register_i2cconfig();
    register_i2cdetect();
    register_i2cget();
    register_i2cset();
    register_i2cdump();
    register_dac_set();
    register_ssd1306();
}
