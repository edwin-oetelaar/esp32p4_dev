set(component_srcs "ssd1306_fonts.c" "ssd1306.c")
# alternatief set(component_srcs "src/matrix_keyboard.c")
idf_component_register(SRCS "${component_srcs}"
            INCLUDE_DIRS "." # kan ook "include" zijn
		    PRIV_REQUIRES "esp_driver_i2c" # en deze "esp_driver_gpio"
            REQUIRES "")
