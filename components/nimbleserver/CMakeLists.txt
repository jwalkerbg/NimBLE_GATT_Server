file(GLOB_RECURSE srcs "src/*.c")

idf_component_register(SRCS "${srcs}"
                       PRIV_REQUIRES nvs_flash esp_driver_gpio espressif__led_strip
                       REQUIRES bt
                       INCLUDE_DIRS "./include")
