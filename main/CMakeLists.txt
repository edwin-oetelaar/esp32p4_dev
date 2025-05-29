set(srcs "i2ctools_example_main.c" "cmd_i2ctools.c")

idf_component_register(SRCS ${srcs}
     PRIV_REQUIRES fatfs esp_driver_i2c ssd1306 gp8413_sdc esp_timer
     INCLUDE_DIRS ".")
