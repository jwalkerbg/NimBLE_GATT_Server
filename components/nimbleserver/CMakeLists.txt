file(GLOB_RECURSE srcs "src/*.c")

idf_component_register(SRCS "${srcs}"
                       PRIV_REQUIRES nvs_flash
                       REQUIRES bt
                       INCLUDE_DIRS "./include")
