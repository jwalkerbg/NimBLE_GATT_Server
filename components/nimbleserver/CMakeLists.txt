file(GLOB_RECURSE srcs "src/*.c")

idf_component_register(SRCS "${srcs}"
                       PRIV_REQUIRES nvs_flash
                       REQUIRES bt
                       INCLUDE_DIRS "./include")

# Pass the version to the build system
set(NIMBLE_SERVER_VERSION "${CONFIG_NIMBLE_SERVER_VERSION}")

message(STATUS "NIMBLE_SERVER_VERSION: ${NIMBLE_SERVER_VERSION}")