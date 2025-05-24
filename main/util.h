# pragma once
// Edwin vd Oetelaar
// part of esp32 robot control over i2c

// #include <stdint.h>
// #include "esp_system.h"
// #include "esp_log.h"

// Driver includes
// #include <driver/spi_master.h>
// #include <driver/i2c.h>
// #include <driver/i2c_master.h>

// typedef struct {
//     uint8_t device_address; // 8-bit adres van het apparaat
//     const char *name;       // Pointer naar de naam van het apparaat
//     uint8_t found;          // Boolean vlag (1 of 0)
// } device_description_t;

// // math map range to other range
// float map(float x, float in_min, float in_max, float out_min, float out_max);

// // i2c stuff
// void initialize_i2c_pin(gpio_num_t pin, uint32_t level);
// void unfreeze_i2c(void);
// void scan_i2c_bus(i2c_master_bus_handle_t bus_handle);
// int test_i2c_devices_present(i2c_master_bus_handle_t bus_handle, device_description_t device_list[]);

/* smart arrays size macros, from linux kernel */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))
/* &a[0] degrades to a pointer: a different type from an array */
#define __must_be_array(a) BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))
#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:(-!!(e)); }))

#pragma endregion